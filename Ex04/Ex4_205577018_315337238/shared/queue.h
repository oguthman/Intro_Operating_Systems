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

/************************************
*      definitions                 *
************************************/
	
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
/// Description: push items to queue (FIFO/FILO). 
/// Parameters: 
///		[in] head - pointer to queue. 
///		[in] item - stored item. 
///		[in] head - if true push new item to the head, otherwise push to the tail.
/// Return: none.
void queue_push(s_node** head, void* item, bool push_to_head);	//stack
/// Description: push items to queue by priority - inc_order/dec_order. 
/// Parameters: 
///		[in] head - pointer to queue. 
///		[in] item - stored item. 
///		[in] priority - weight of the item. 
///		[in] inc_order - true if inc_order, false if dec_order. 
/// Return: none.
void queue_priority_push(s_node** head, void* item, uint32_t priority, bool inc_order);

/// Description: push new items to the end LRU queue. If exits, move item to the end of the queue.  
/// Parameters: 
///		[in] head - pointer to queue. 
///		[in] page_number - stored item. 
///		[in] time_of_use - stored item. 
/// Return: none.
void queue_lru_push(s_node_lru** head, uint32_t page_number, uint32_t time_of_use);
/// Description: search for available page to clear for frame.  
/// Parameters: 
///		[in] head - pointer to queue. 
///		[in] time - global time of the process. 
///		[in] page_to_clear - pointer to revevant page to clear from frame. 
/// Return: true if page was found, false otherwise.
bool queue_lru_pop_available_page(s_node_lru** head, uint32_t time, uint32_t* page_to_clear);
void* queue_lru_pop(s_node_lru** head);
bool queue_lru_is_empty(s_node_lru** head);

#endif //__QUEUE_H__
