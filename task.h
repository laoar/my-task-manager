#ifndef _CPU_LIMIT_TASK_
#define _CPU_LIMIT_TASK_

#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <libproc.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/time.h>

/*
   refer to struct proc_bsdinfo, which define in bsd/sys/proc_info.h.
   The max len of exe name.	
*/

#define NAME_LEN 2*MAXCOMLEN 
#define MIN_CPU_TIME 20
#define TIME_SLOT 100000  // 1ms.

struct task_control {
	uint64_t task_time;
	struct timeval cputime;
	double cpu_usage; /* value int range <0, 1>*/
};

struct allpid {
	int cnt;
	int *pid_list;
};

extern struct allpid pids;

struct pid_hit_list {
	TAILQ_ENTRY(pid_hit_list) pid_list;
	pid_t pid;
	struct proc_taskallinfo ti;
	/* the following 3 members are for managment, although they're already exist in ti.*/
	pid_t ppid;
	uint64_t start_time;
	uint64_t cputime;
	struct timeval last_update;
}; 


int get_allproc(struct allpid *pids);
int get_pidinfo(pid_t pid, struct proc_taskallinfo *task_info);
int pid_hit_list_init(char *task_name);
int task_manage(double limit);
void inc_nice(void);
#endif 
