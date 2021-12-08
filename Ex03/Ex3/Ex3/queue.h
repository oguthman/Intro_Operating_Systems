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
	uint32_t time;
	struct node *next;
} s_node;

/************************************
*       API                         *
************************************/

uint32_t queue_pop(s_node** head);
void queue_push(s_node** head, uint32_t time);
bool queue_is_empty(s_node** head);


#endif //__QUEUE_H__
