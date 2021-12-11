/*!
******************************************************************************
\file queue.c
\date 26 November 2021
\authors Shahar Dorit Morag 315337238 & Ofir Guthman 205577018
\project #3
\brief

\details

\par Copyright
(c) Copyright 2021 Ofir & Shahar
\par
ALL RIGHTS RESERVED
*****************************************************************************/

/************************************
*      include                      *
************************************/
#include "queue.h"
#include <string.h>
#include <stdlib.h>

/************************************
*      definitions                 *
************************************/

/************************************
*       types                       *
************************************/


/************************************
*      variables                    *
************************************/

/************************************
*      static functions             *
************************************/
static s_node* create_new_node(void* item);
static s_node_lru* create_new_node_lru(uint32_t page_number, uint32_t time_of_use);

/************************************
*       API implementation          *
************************************/
bool queue_is_empty(s_node** head)
{
	return (*head == NULL);
}

void* queue_pop(s_node** head)
{
	if (queue_is_empty(head))
		return NULL;

	s_node* current = *head;
	(*head) = current->next;

	void* item = current->item;
	free(current);
	return item;
}

void queue_push(s_node **head, void* item)
{
	s_node* node = create_new_node(item);
	
	if (queue_is_empty(head))
		*head = node;
	
	else
	{
		node->next = *head;
		(*head) = node;
	}
}

void queue_priority_push(s_node** head, void* item, uint32_t priority, bool inc_order)
{
	s_node* node = create_new_node(item);
	node->priority = priority;

	if (*head == NULL)
	{
		*head = node;
	}

	else if (inc_order ? (*head)->priority > node->priority : (*head)->priority < node->priority)
	{
		node->next = *head;
		(*head) = node;
	}

	else
	{
		s_node* current = *head;
		while (current->next != NULL &&
			(inc_order ? current->next->priority < node->priority : current->next->priority < node->priority))
		{
			current = current->next;
		}

		node->next = current->next;
		current->next = node;
	}
}

void queue_lru_push(s_node_lru** head, uint32_t page_number, uint32_t time_of_use) {
	s_node_lru* new_node = create_new_node_lru(page_number, time_of_use);

	if (*head == NULL)
	{
		*head = new_node;
		return;
	}
	
	s_node_lru* current = *head;
	while (current->next != NULL)
	{
		if (current->next->page_number == new_node->page_number)
		{
			//put first
			new_node = *head;
			current->next = current->next->next;
			return;
		}			
		current = current->next;
	}
	current = new_node;	
}


bool check_if_available(s_node_lru** head, uint32_t time, int32_t* page_to_clear)
{
	s_node_lru* current = *head;
	while (current->next != NULL) 
	{
		// clear node from LRU linked list
		if (current->next->time_of_use <= time)
		{
			*page_to_clear = (int32_t)current->next->page_number;
			current->next = current->next->next;
			return 1;
		}
	}
	return 0;
}

/************************************
* static implementation             *
************************************/
static s_node* create_new_node(void* item)
{
	s_node* node = malloc(sizeof(s_node));
	ASSERT(node != NULL, "failed creating new node\n");	
	
	node->item = item;
	node->next = NULL;

	return node;
}

static s_node_lru* create_new_node_lru(uint32_t page_number, uint32_t time_of_use)
{
	s_node_lru* node = malloc(sizeof(s_node_lru));
	ASSERT(node != NULL, "failed creating new node\n");

	node->page_number = page_number;
	node->time_of_use = time_of_use;
	node->next = NULL;

	return node;
}