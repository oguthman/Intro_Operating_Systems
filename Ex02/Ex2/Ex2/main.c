#define _CRT_SECURE_NO_WARNINGS
/*!
******************************************************************************
\file main.c
\date 9 November 2021
\author Shahr Dorit Morag & Ofir Guthman
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
#include <windows.h>
#include <fileapi.h>

#include "thread.h"

/************************************
*      definitions                 *
************************************/

/************************************
*       types                       *
************************************/

/************************************
*      variables                    *
************************************/
static struct {
	int number_of_schools;
	int real_weight;
	int human_weight;
	int eng_weight;
	int eval_weight;
} gs_argument_inputs;

/************************************
*      static functions             *
************************************/
static void parse_arguments(int argc, char* argv[]);
static void close_handles(HANDLE* handles, int number_of_active_handles);

/************************************
*       API implementation          *
************************************/
int main(int argc, char* argv[])
{
	// Open new directory 'Results'
	ASSERT((CreateDirectoryA("Results", NULL) != 0) || (GetLastError() == ERROR_ALREADY_EXISTS), "Error: failed opening new directory\n");
	
	parse_arguments(argc, argv);
	HANDLE handles[10];
	s_thread_inputs inputs[10];
	int number_of_active_handles = 0;
	DWORD finished_thread_index = -1;		// init to unvalid value

	// Loop over number of schools and open new thread for each school
	for (int i = 0; i < gs_argument_inputs.number_of_schools; i++)
	{
		int index = finished_thread_index != -1 ? finished_thread_index : number_of_active_handles;
		s_thread_inputs input = {i, gs_argument_inputs.real_weight, gs_argument_inputs.human_weight, gs_argument_inputs.eng_weight, gs_argument_inputs.eval_weight};
		inputs[index] = input;
		handles[index] = OpenNewThread(&inputs[index]);
		number_of_active_handles++;
		
		// Create limitation for max 10 threads running in parallel
		if (number_of_active_handles < 10 || i == gs_argument_inputs.number_of_schools - 1) continue;
		
		DWORD single_thread_status = WaitForMultipleObjects(
			number_of_active_handles,		// number of arguments in handles array
			handles,						// pointer to handles array
			FALSE,							// rather wait for all handles to finish or not
			INFINITE);						// how much time in msec to wait for first handle to finish

		// 10 threads are runnuing and one finished. Save it's index and decrease number_of_active_handles
		finished_thread_index = single_thread_status - WAIT_OBJECT_0;
		CloseHandle(handles[finished_thread_index]);
		number_of_active_handles--;
	}

	WaitForMultipleObjects(
		number_of_active_handles,		// number of arguments in handles array
		handles,						// pointer to handles array
		TRUE,							// rather wait for all handles to finish or not
		INFINITE);						// how much time in msec to wait for first handle to finish

	close_handles(handles, number_of_active_handles);
}

/************************************
* static implementation             *
************************************/

/// Description: parse arguments and open files.  
/// Parameters: 
///		[in] argc - number of arguments. 
///		[in] argv - arguments list. 
/// Return: none.
static void parse_arguments(int argc, char* argv[])
{
	// Check if there are enough arguments
	ASSERT(argc == 6, "Error: not enough arguments.\n");

	// Parse arguments
	gs_argument_inputs.number_of_schools = strtol(argv[1], NULL, 10);
	gs_argument_inputs.real_weight = strtol(argv[2], NULL, 10);
	gs_argument_inputs.human_weight = strtol(argv[3], NULL, 10);
	gs_argument_inputs.eng_weight = strtol(argv[4], NULL, 10);
	gs_argument_inputs.eval_weight = strtol(argv[5], NULL, 10);
}

static void close_handles(HANDLE* handles, int number_of_active_handles)
{
	for (int i = 0; i < number_of_active_handles; i++)	
		CloseHandle(handles[i]);
}