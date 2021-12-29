/*!
******************************************************************************
\file threads.h
\date 20 December 2021
\author Shahar Dorit Morag & Ofir Guthman
\project #4
\brief 

\details

\par Copyright
(c) Copyright 2021 Ofir & Shahar
\par
ALL RIGHTS RESERVED
*****************************************************************************/

#ifndef __THREADS_H__
#define __THREADS_H__

/************************************
*      include                      *
************************************/
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <stdbool.h>

/************************************
*      definitions                 *
************************************/

/************************************
*       types                       *
************************************/


/************************************
*       API                         *
************************************/
/// Description: create new thread.  
/// Parameters: 
///		[in] p_start_routine - thread function. 
///		[in] p_thread_parameters - parametes for thread function.
/// Return: thread_handle - handle for the new thread.
HANDLE create_new_thread(LPTHREAD_START_ROUTINE p_function, LPVOID p_thread_parameters);

/// Description: wait for running threads to finish.  
/// Parameters: 
///		[in] handles - handles array of running threads. 
///		[in] number_of_active_handles - count of running threads. 
/// Return: true - thread ended successfully, false - otherwise.
bool wait_for_thread(HANDLE* handles, int number_of_active_handles);
HANDLE create_mutex(bool signal);
HANDLE create_semaphore(int init_count, int max_count);

#endif //__THREADS_H__