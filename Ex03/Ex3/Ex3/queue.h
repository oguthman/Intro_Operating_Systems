/*!
******************************************************************************
\file queue.h
\date 26 November 2021
\author Shahar Dorit Morag & Ofir Guthman
\brief

\details

\par Copyright
(c) Copyright 2021 Ofir & Shahar
\par
ALL RIGHTS RESERVED
*****************************************************************************/

#ifndef __QUEUE_H__
#define __QUEUE_H__

/************************************
*      include                      *
************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "db.h"

/************************************
*      definitions                 *
************************************/
#define ASSERT(cond, msg, ...)														\
	do {																			\
		if (!(cond)) {																\
			printf("Assertion failed at file %s line %d: \n", __FILE__, __LINE__);	\
			printf(msg, __VA_ARGS__);												\
			exit (1);																\
		}																			\
	} while (0);	
/************************************
*       types                       *
************************************/
typedef struct node {
	void* item;
	uint32_t priority;
	struct node* next;
} s_node;

typedef struct node_lru {
	uint32_t page_number;
	uint32_t time_of_use;
	struct node_lru* next;
} s_node_lru;

/************************************
*       API                         *
************************************/

bool queue_is_empty(s_node** head);

void* queue_pop(s_node** head);
void queue_push(s_node** head, void* item);	//stack

void queue_priority_push(s_node** head, void* item, uint32_t priority, bool inc_order);
void queue_lru_push(s_node_lru** head, uint32_t page_number, uint32_t time_of_use);
bool check_if_available(s_node_lru** head, uint32_t time, int32_t* page_to_clear);

#endif //__QUEUE_H__
