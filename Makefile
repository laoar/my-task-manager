all:
	gcc task.c main.c -o task_manager --std=c99

clear:
	rm task_manager 
