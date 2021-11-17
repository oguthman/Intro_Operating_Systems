/*!
******************************************************************************
\file thread.h
\date 24 October 2021
\author Shahar Dorit Morag & Ofir Guthman
\brief

\details

\par Copyright
(c) Copyright 2021 Ofir & Shahar
\par
ALL RIGHTS RESERVED
*****************************************************************************/

#ifndef __THREAD_H__
#define __THREAD_H__

/************************************
*      include                      *
************************************/
#include <stdio.h>

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
typedef struct
{
	int school_number;
	int real_grade_weight;
	int human_grade_weight;
	int eng_grade_weight;
	int eval_grade_weight;
} s_thread_inputs;

/************************************
*       API                         *
************************************/

/*!
******************************************************************************
\brief
Initialize func

\details
Must be called only once

\param
 [in] counter_val - reset counter value
 [out] out_val    -

\return none
*****************************************************************************/
HANDLE OpenNewThread(s_thread_inputs *input);

#endif //__THREAD_H__