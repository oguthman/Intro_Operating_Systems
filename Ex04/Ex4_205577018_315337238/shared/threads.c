/*!
******************************************************************************
\file threads.c
\date 20 December 2021
\authors Shahar Dorit Morag 315337238 & Ofir Guthman 205577018
\project #4
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
#include "threads.h"

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

/************************************
*       API implementation          *
************************************/
HANDLE create_new_thread(LPTHREAD_START_ROUTINE p_function, LPVOID p_thread_parameters)
{
	HANDLE thread_handle;
	DWORD thread_id;

	if (p_function == NULL)
	{
		printf("Error: failed creating a thread\nReceived NULL pointer\n");
		return NULL;
	}

	thread_handle = CreateThread(
		NULL,                /*  default security attributes */
		0,                   /*  use default stack size */
		p_function,			 /*  thread function */
		p_thread_parameters, /*  argument to thread function */
		0,                   /*  use default creation flags */
		&thread_id);         /*  returns the thread identifier */

	return thread_handle;
}

/************************************
* static implementation             *
************************************/

