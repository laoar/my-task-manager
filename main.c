#include <stdio.h>
#include <stdlib.h>
#include "task.h"


extern int proc_pid(struct allpid *pids);
extern int get_cpu_num(void);

int main(int argc, char *argv[])
{
	// usage : ./a.out process_name percentage(1~100) 
	assert(argc == 3);
	
	double percentage = atoi(argv[2]) / 100.0;
	char *task_name = (char *)malloc(strlen(argv[1]) + 1);
	if (!task_name) {
		perror("main malloc");
		return -1;
	}
	memcpy(task_name, argv[1], strlen(argv[1]) + 1);

	// actually it isn't used now. 
	int ncpu = get_cpu_num();
	printf("cpu num : %d\n", ncpu);

	if (get_allproc(&pids) == -1) {
		return -1;
	}

	if (pid_hit_list_init(task_name) == -1) {
		return -1;
	}

	/* set this control thread to a high priority */
	inc_nice();

	/* main loop */
	while(1) {
		if (task_manage(percentage) == -1) {
			return -1;
		}
	}

	return 0;
}
