/*
 *These routines provides the interfaces used by the scheduler to modify the list
 */
 
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

/*for structure definations*/
#include "scheduler.h"


/*
 *Adds a job to the list
 *the position of addition will be corresponding to the exec time
 */

struct node * add_job (struct node *job_list, 
		       struct job a)
{
	if (job_list == NULL) {
		job_list = malloc (sizeof(struct node));
		job_list->next = job_list->prev = NULL;
		job_list->jb = a;
		return job_list;
	}
	else {
		struct node *tmp=job_list,*t2;
		struct job *tmp1;
		
		
		while (1) {
			tmp1 = &(tmp->jb);
			
			if (tmp1->exec_time > a.exec_time) {
				t2 = (struct node *)malloc (sizeof (struct node));
				t2->jb = a;
				
				t2->prev = tmp->prev;
				t2->next = tmp;
				tmp->prev = t2;
				
				if (t2->prev != NULL){	//inserted somewhere in middle of list
					(t2->prev)->next = t2;
					return job_list;
				}
				else 			//inserted in the beginning of the list
					return t2;
			}
			if (tmp->next == NULL)
				break;
			else
				tmp =tmp->next;
		}
		
		if (tmp->next == NULL) {		//inserted @ the end of the list
			t2=malloc (sizeof (struct node));
			t2->jb = a;
			t2->prev = tmp;
			t2->next = NULL;
			tmp->next = t2;
			return job_list;
		}
	}
	return job_list;
}



/*
 *Removes the first job on the list.
 */
struct node * remove_job (struct node *p)
{
	if (p == NULL || p->prev != NULL) {
		fprintf (stderr, "Empty LInked list");
		return p;
	}
	
	if (p->next == NULL) {	//this is the last node
		free (p);
		p=NULL;
		return p;
	}
	else {			//remove the first node of lot
		p=p->next;
		free (p->prev);
		p->prev = NULL;
		return p;
	}
}


/*
 *Prints the list (used for debugging)
 */

void print_list(struct node *p)
{
	fprintf (stderr,"\n");
	while (p!=NULL) {
		fprintf (stderr, "%p %u %p\n",p->prev,(p->jb).exec_time,p->next);
		p=p->next;
	}
}



/*
int main()
{
	struct node *job_list = NULL;
	unsigned int tmp;
	struct job a;
	
	a.repete_interval=0;
	a.code[0]='\0';
	
	while (1) {
		printf ("Enter additional time");
		scanf ("%u",&tmp);
		if (tmp == 0)
			break;
		a.exec_time = time() + tmp;
		printf ("exec?");
		scanf("%u",&tmp);
		if (tmp)
			job_list =remove_job(job_list);
		else
			job_list = add_job (job_list,a);
		print_list(job_list);
	}
	return 0;
}*/


