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
#include <stdio.h>

/************************************
*      definitions                 *
************************************/
#define THREAD_TIMEOUT	INFINITE
#define TERMINATE_ALL_THREADS_EXITCODE 0x55

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

bool wait_for_threads(HANDLE* handles, int number_of_active_handles, bool wait_for_all, DWORD timeout, bool terminate)
{
	DWORD threads_status = WaitForMultipleObjects(
		number_of_active_handles,		// number of arguments in handles array
		handles,						// pointer to handles array
		wait_for_all,					// rather wait for all handles to finish or not
		timeout);						// how much time in msec to wait for first handle to finish

	if (threads_status >= WAIT_OBJECT_0 && threads_status < WAIT_OBJECT_0 + number_of_active_handles)
		return true;
	switch (threads_status)
	{
	case WAIT_TIMEOUT:
	{
		if (terminate)
		{
			// printf("Terminating all thereads\n");
			// Terminate all running threads
			for (int i = 0; i < number_of_active_handles; i++)
			{
				TerminateThread(handles[i], TERMINATE_ALL_THREADS_EXITCODE);
				// close all handles in the end of main()
			}

			// Wait a few milliseconds for the process to terminate.
			Sleep(10);
		}
		return false;
	}
	default:
	{
		printf("WaitForMultipleObject has returned with code %d. The program will exit\n", threads_status);
		return false;
	}
	}
}

HANDLE create_mutex(bool signal)
{
	/* Create the mutex that will be used to synchronize access to critical section */
	HANDLE mutex = CreateMutex(
		NULL,		/* default security attributes */
		!signal,	/* ownership */
		NULL);		/* unnamed mutex */

	return mutex;
}

HANDLE create_semaphore(int init_count, int max_count)
{
	/* Create the mutex that will be used to synchronize access to critical section */
	HANDLE semaphore = CreateSemaphore(
		NULL,		/* semaphore attributes */
		init_count,	/* initial count */
		max_count,	/* max count */
		NULL);		/* name semaphore */

	return semaphore;
}

HANDLE create_event_handle(bool manual_reset)
{
	HANDLE event = NULL;

	event = CreateEvent(
		NULL,				// default security attributes
		manual_reset,		// manual reset event
		false,				// initail state
		NULL);				// object name

	if (event == NULL)
	{
		printf("Error: CreateEvent failed (%d)\n", GetLastError());
		return NULL;
	}

	return event;
}

bool wait_for_event(HANDLE event)
{
	DWORD code = WaitForSingleObject(event, INFINITE);
	if (code != WAIT_OBJECT_0)
	{
		printf("Error: waiting to event failed (%d)\n", GetLastError());
		return false;
	}

	return true;
}

void close_handles(HANDLE* handles, uint32_t number_of_active_handles)
{
	for (uint32_t i = 0; i < number_of_active_handles; i++)
		if (handles[i] != NULL)
			CloseHandle(handles[i]);
}

/************************************
* static implementation             *
************************************/

