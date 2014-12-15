#include <stdio.h>
#include <stdlib.h>
#include <sys/sysctl.h>
#include <sys/resource.h>
#include "task.h"

struct allpid pids;
TAILQ_HEAD(, pid_hit_list) hit_header;

struct task_control ctask;

int get_allproc(struct allpid *pids)
{
	int cnt;
	int *pid_buffer;
	// get the pid count in our Mac system.
	if ((cnt = proc_listallpids(NULL, 0)) <= 0) {
		perror("get_listpids");
		return -1;
	}

	// apply mamory to store all these pids
   if ((pid_buffer = (int *)malloc(cnt * sizeof(pid_t))) == NULL) {
   		perror("proc_pid malloc");
		return -1;
   }	

   // store all these pids to the buffer
   if ((cnt = proc_listallpids(pid_buffer, cnt * sizeof(pid_t))) <= 0) {
   		perror("alloc pid buffer");
		return -1;
   }

   pids->pid_list = pid_buffer;
   pids->cnt = cnt;

   return 0;
}

/*
	get total cpu number in your system. 
*/
int get_cpu_num(void)
{
	int ncpu = -1;
	int mib[2] = {CTL_HW, HW_NCPU};
	size_t len = sizeof(ncpu);

	sysctl(mib, 2, &ncpu, &len, NULL, 0);

	return ncpu;
}

/*
	get the task infomation with PID. 
*/
int get_pidinfo(pid_t pid, struct proc_taskallinfo *task_info)
{
	int retval;

	retval = proc_pidinfo(pid, PROC_PIDTASKALLINFO, 0, task_info, sizeof(struct proc_taskallinfo));
	if (retval <= 0) {
		perror("proc_pidinfo");
		retval =  -1;
	} else if(retval < sizeof(task_info)){
		printf("retval %d less than expected size %lu\n", retval, sizeof(struct proc_taskallinfo));
		retval =  -1;
	}	

	return retval;
}

/*
   init the pid hit list. 
*/
int pid_hit_list_init(char *task_name)
{
	int i;
	int name_match = 0;
	int name_len;
	struct pid_hit_list pid_hit[20]; // why 20? I think it's enough.
	char *name = (char *)malloc(NAME_LEN);
	if (name == NULL) {
		perror("malloc");
		return -1;
	}

	TAILQ_INIT(&hit_header);
	for (i = 0; i < pids.cnt; i++) {
		if (!(name_len = proc_name(*(pids.pid_list + i), name, NAME_LEN))) {
			continue;
		}
		
		if (strncmp(task_name, name, MIN(name_len, strlen(task_name)))) {
			continue;
		}
		pid_hit[name_match].pid = *(pids.pid_list + i);
		TAILQ_INSERT_TAIL(&hit_header, &pid_hit[name_match], pid_list);
		name_match += 1;
	}
	free(name);
	// we can't find the process you type.
	if (!name_match) {
		printf("come on, little man. what the fuck did you type?\n");
		return -1;
	}

	struct pid_hit_list *var;
	TAILQ_FOREACH(var, &hit_header, pid_list){
		get_pidinfo(var->pid, &var->ti);
		var->ppid = var->ti.pbsd.pbi_ppid;
		var->start_time = var->ti.pbsd.pbi_start_tvsec;
		/* convert to ms*/
		/* user means user level; system means kernel level.*/
		var->cputime = (var->ti.ptinfo.pti_total_user + var->ti.ptinfo.pti_total_system) / 1000000; 
		ctask.task_time += var->cputime;
		//var->last_update.tv_sec = 0;
		//var->last_update.tv_usec = 0;
	}
	gettimeofday(&ctask.cputime, NULL);

	return 0;
}

int task_manage(double limit)
{
	assert(limit > 0 && limit < 1);

	struct timeval now; 
	gettimeofday(&now, NULL);
	uint64_t cpu_time = ((now.tv_sec - ctask.cputime.tv_sec) * 1000000
					 + (now.tv_usec - ctask.cputime.tv_usec)) / 1000;  // ms.

	if (cpu_time < MIN_CPU_TIME) // waste so less cpu, don't need control it. 
		return 0;	

	struct pid_hit_list *var;
	uint64_t task_time = 0;
	TAILQ_FOREACH(var, &hit_header, pid_list) {
		get_pidinfo(var->pid, &var->ti); // actually, only need update cpu_time.
		var->cputime = (var->ti.ptinfo.pti_total_user + var->ti.ptinfo.pti_total_system) / 1000000;	
		task_time += var->cputime; //caculate total task run time. 
	}

	ctask.cpu_usage = (task_time - ctask.task_time) / cpu_time * 1.0;

	/* don't need control*/
	if (ctask.cpu_usage < limit)
			return 0;

	/* Now we make the process sleep */
	struct timespec tsleep = {0, 0};
	struct timespec twork = {0, 0};

	twork.tv_nsec = TIME_SLOT * 1000*limit;	//TIME_SLOT : 100000
	tsleep.tv_nsec = TIME_SLOT * 1000 - twork.tv_nsec;

	/* let all the process sleep */
	TAILQ_FOREACH(var, &hit_header, pid_list) {
		if (kill(var->pid,SIGSTOP)!=0) {
			perror("kill stop");
			return -1;
		}
	}

	nanosleep(&tsleep,NULL);

	/* then wakeup them and update the data*/
	TAILQ_FOREACH(var, &hit_header, pid_list) {
		if (kill(var->pid,SIGCONT)!=0) {
			perror("kill cont");
			return -1;
		}
	}

	ctask.task_time = task_time;
	memcpy(&ctask.cputime, &now, sizeof(struct timeval));
	
	return 0;
}


void inc_nice(void)
{
#define MIN_NICE -20
	int old_nice = getpriority(PRIO_PROCESS, 0);
	int nice = old_nice - 1;

	while (setpriority(PRIO_PROCESS, 0, nice) == 0  && nice > MIN_NICE) {
			nice--;
	}

	if (nice != old_nice - 1) {
		printf("control process priority : %d old:%d\n", getpriority(PRIO_PROCESS, 0), old_nice);
	} else {
		printf("pls. use root to change the priority!\n");
	}
}
