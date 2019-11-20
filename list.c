#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "list.h"


info_p LIST_create()
{
	info_p linfo;
	linfo = malloc(sizeof(info));
	linfo->size = 0;
	linfo->head = NULL;
	
	return linfo;
}


void LIST_destroy(info_p *linfo)
{
		list_node_p temp, todel;
		
		if(LIST_empty(*linfo))
		{
			temp = ((*linfo)->head)->next;
			while(temp != (*linfo)->head)
			{
				todel = temp;
				temp = temp->next;
				free(todel);
		}
		free((*linfo)->head);
		(*linfo)->head = NULL;
    	}	
		free(*linfo);
		(*linfo) = NULL;
}

void LIST_traverse(const info_p  linfo, int fd)	//traverses whole list and prints the elements to file
{
	list_node_p temp;
	temp = linfo->head;
	
	if(LIST_empty(linfo))
	{   
		fprintf(stderr, "List is empty\n");
		return;
	}
	
	do
	{   
		write_value(fd, temp->data);
		temp = temp->next;
	}while(temp != linfo->head);
}

int LIST_empty(info_p linfo)
{
	return (linfo->head == NULL);
	//return (linfo->size);
}


void LIST_content(const info_p  linfo, list_node_p p, process *proc, int *error) 
{
	*error = 0;
	if(p != NULL)
		set_value(proc, p->data);
	else
		*error = 1;
	
}

void LIST_insert(info_p *const linfo, process proc, list_node_p cur_node, int *error)
{
	list_node_p last;

	if(cur_node == NULL && LIST_empty(*linfo))
		insert_begin(linfo, proc, error);

	else 
	{
		LIST_last(*linfo, &last, error);
		insert_after(linfo, proc, last, error);
	}
	 (*linfo)->size++;
}

void insert_begin(info_p * const linfo, process proc, int *error)
{
	list_node_p temp1, temp2; 

	temp1 = malloc(sizeof(list_node));	//New node to be inserted
	if (temp1 == NULL)
	{   *error = 1;
		return;
	}
	set_value(&(temp1->data), proc);
	if(LIST_empty(*linfo))		//List is empty
	{
		temp1->next = temp1;	//Only node, shows at itself
		temp1->previous = temp1;
	
	}
	else	//Not empty list
	{   
		LIST_last(*linfo, &temp2, error);	
		temp1->next = (*linfo)->head;
		temp1->previous = temp2;
		temp2->next = temp1;
	}
	(*linfo)->head = temp1;	
	//(*linfo)->size ++; 
}

void insert_after(info_p * const linfo, process proc, list_node_p cur_node, int *error)
{
	list_node_p temp; //New node

	temp = malloc(sizeof(list_node));
	if (temp == NULL)
	{   *error = 1;
		return;
	}
	
	set_value(&(temp->data), proc);
	temp->next = cur_node->next;
	temp->previous = cur_node;
	if(temp->next != NULL)
		temp->next->previous = temp;
	cur_node->next = temp;

	//(*linfo)->size ++;
}

void LIST_delete(info_p * const linfo, list_node_p *p, int * const error)
{
	list_node_p temp, previous;
	/*if ((*p)->data.command != NULL)
	{
		int i = 0, j;
		while ((*p)->data.command[i] != NULL)
				i++;
		for (j = 0; j < i; j++)
			free((*p)->data.command[j]);
		free((*p)->data.command);
	}*/
	int error2 = 0;
	temp = *p;
	*error = 0;
	if(LIST_empty(*linfo) || (*p == NULL))
	{   
		*error = 1;
		return;
	}
	if(((*linfo)->head)->next != (*linfo)->head) //more than one nodes in list
	{  
		previous = temp;                                          
		LIST_previous(*linfo, &previous, &error2);
	 	if(error2 == 2)
		{ 
			*error = 1;
			  return;
		}                                              
		if((*linfo)->head == *p) //first node is to be deleted
			(*linfo)->head = (*p)->next;	
		*p = temp->next;
		previous->next = temp->next;
		temp->next->previous = previous;
		free(temp);
		temp = NULL;
   	}
	else //only one node in list
	{ 
		free((*linfo)->head);
		(*linfo)->head = NULL;
		*p = NULL;                
	}                      
	(*linfo)->size--;                                                                  	
}


void LIST_next(const info_p  linfo, list_node_p * const p, int * const error)
{   
	*error = 0;
	if((*p) != NULL)
	 { 
		if((*p)->next != NULL) 
			*p = (*p)->next;
		else
			*error = 1;               
	}  
	else
	   *error = 2; 
}

void LIST_previous (const info_p linfo, list_node_p * const p, int * const error)
{
	list_node_p temp;
	int error2;
	*error = 0;
	if((*p) != NULL)
	{   
		if((*p) == linfo->head)	//p points to first node
		{
			LIST_last(linfo, &temp, &error2);     
			if(error2 == 1)
			{ 
				*error = 1;
				  return;
			}
			*p = temp;
		}   
		else if((*p) == (linfo->head)->next)  //p points to second node
			*p = linfo->head;   
		else
			*p = (*p)->previous;
	}
	else
		*error = 2;    
}

void LIST_first(const info_p  linfo, list_node_p * const first, int * const error)
{
	*error = 0;
	*first = linfo->head;
	if (LIST_empty(linfo))
		*error = 1;
}

void LIST_last (const info_p linfo, list_node_p *const last, int *const error)
{
	list_node_p temp;
	*error = 0;
	*last = NULL;
	if (LIST_empty(linfo))          /* list is empty */
		*error = 1;                 
	else                       /* not empty*/
	{   
		temp = linfo->head;
		while (temp->next != linfo->head)
			temp = temp->next;
		*last = temp;
	 } 
}

pid_t LIST_search_jobID(const info_p  linfo, int job_id, process *proc, list_node_p *p, int *found)
{
	process temp;
	list_node_p current;
	int error;

	error = 0;
	current = linfo->head;
	LIST_last(linfo, p, &error);
	error = 0;
	*found = 0;

	if(!LIST_empty(linfo))
	 {	do
		{   
			LIST_content(linfo, current, &temp, &error);
			if (temp.jobID == job_id)
			{
				*found = 1;
				set_value(proc, temp);
				return proc->pid;
			}
			else
			{   
				*p = current;
				LIST_next(linfo, &current, &error);
			}
		}
		while ( (!(*found)) && (current != linfo->head));
	}
	return 0;
}

void LIST_search_pid(const info_p  linfo, int pid, process *proc, list_node_p *p, int *found)
{
	process temp;
	list_node_p current;
	int error;

	error = 0;
	current = linfo->head;
	LIST_last(linfo, p, &error);
	error = 0;
	*found = 0;

	if(!LIST_empty(linfo))
	 {	do
		{   
			LIST_content(linfo, current, &temp, &error);
			if (temp.jobID == pid)
			{
				*found = 1;
				set_value(proc, temp);
				//return proc->pid;
			}
			else
			{   
				*p = current;
				LIST_next(linfo, &current, &error);
			}
		}
		while ( (!(*found)) && (current != linfo->head));
	}
}
