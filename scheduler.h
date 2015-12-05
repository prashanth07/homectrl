/*
 *Header related to the definations in the scheduler.c file
 */

#ifndef IRMS_SCHEDULER
#define IRMS_SCHEDULER

struct job{
	time_t exec_time;
	time_t repete_interval;
	char code[7];
};

struct node {
	struct job jb;
	struct node *next;
	struct node *prev;
};

struct node * add_job (struct node *job_list, struct job a);

struct node * remove_job (struct node *p);

void print_list(struct node *p);

#endif

