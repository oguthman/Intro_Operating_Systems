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



#endif //__THREADS_H__