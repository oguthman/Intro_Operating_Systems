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
static s_node* create_new_node(uint32_t time);


/************************************
*       API implementation          *
************************************/
uint32_t queue_pop(s_node **head)
{
	if (*head == NULL)
		return -1;

	s_node *current = *head;
	(*head) = current->next;

	uint32_t time = current->time;
	free(current);
	return time;
}

// check double **
void queue_push(s_node **head, uint32_t time)
{
	s_node *node = create_new_node(time);
	
	if (*head == NULL)
	{
		*head = node;
	}

	else if ((*head)->time > node->time)
	{
		node->next = *head;
		(*head) = node;
	}

	else
	{
		s_node* current = *head;
		while (current->next != NULL && current->next->time < node->time)
		{
			current = current->next;
		}

		node->next = current->next;
		current->next = node;
	}
}

bool queue_is_empty(s_node** head)
{
	return (*head == NULL);
}

/************************************
* static implementation             *
************************************/
static s_node* create_new_node(uint32_t time)
{
	s_node* node = malloc(sizeof(s_node));
	ASSERT(node != NULL, "failed creating new node\n");	
	
	node->time = time;
	node->next = NULL;

	return node;
}