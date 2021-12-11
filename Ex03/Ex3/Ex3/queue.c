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


/************************************
*       API implementation          *
************************************/
bool queue_is_empty(s_node** head)
{
	return (*head == NULL);
}

void* queue_pop(s_node **head)
{
	if (queue_is_empty(head))
		return NULL;

	s_node *current = *head;
	(*head) = current->next;

	void* item = current->item;
	free(current);
	return item;
}

void queue_push(s_node **head, void* item)
{
	s_node *node = create_new_node(item);
	
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