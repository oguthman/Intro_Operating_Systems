#define _CRT_SECURE_NO_WARNINGS
/*!
******************************************************************************
\file thread.c
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
#include <stdio.h>

#include "thread.h"

/************************************
*      definitions                 *
************************************/
#define ERROR_CODE ((int)(-1))

/************************************
*       types                       *
************************************/

/************************************
*      variables                    *
************************************/

/************************************
*      static functions             *
************************************/
static HANDLE create_new_thread(LPTHREAD_START_ROUTINE p_start_routine, LPVOID p_thread_parameters, LPDWORD p_thread_id);
static DWORD WINAPI routine_func(LPVOID lpParam);
static FILE* open_file(char * file_path, int school_number, char *mode);

/************************************
*       API implementation          *
************************************/
HANDLE OpenNewThread(s_thread_inputs *input)
{
	// Create Thread, pass the routine function
	LPDWORD thread_id;
	return create_new_thread(routine_func, input, &thread_id);
}

/************************************
* static implementation             *
************************************/

/// Description: Create new thread.  
/// Parameters: 
///		p_start_routine - thread function. 
///		p_thread_parameters - parametes for thread function.
///		p_thread_id - id of the new thread.
/// Return: thread_handle - handle for the new thread.
static HANDLE create_new_thread(LPTHREAD_START_ROUTINE p_start_routine,	LPVOID p_thread_parameters,	LPDWORD p_thread_id)
{
	HANDLE thread_handle;

	ASSERT(p_start_routine != NULL, "Error: failed creating a thread\nReceived routine NULL pointer\n");
	ASSERT(p_thread_id != NULL, "Error: failed creating a thread\nReceived thread NULL pointer\n");

	thread_handle = CreateThread(
		NULL,                /*  default security attributes */
		0,                   /*  use default stack size */
		p_start_routine,     /*  thread function */
		p_thread_parameters, /*  argument to thread function */
		0,                   /*  use default creation flags */
		p_thread_id);        /*  returns the thread identifier */
	
	return thread_handle;
}

/// Description: Open files of desired school, calculate average for each student and write to a result file.  
/// Parameters: 
///		lpParam - input of all the required information for calculating student average. 
/// Return: indicator if running failed.
static DWORD WINAPI routine_func(LPVOID lpParam)
{
	s_thread_inputs *input = (s_thread_inputs *)lpParam;
	printf("Starting the routine for school number %d\n", input->school_number);
	
	// Open Files
	FILE* p_real_file = open_file("Real\\Real", input->school_number, "r");
	FILE* p_human_file = open_file("Human\\Human", input->school_number, "r");
	FILE* p_eng_file = open_file("Eng\\Eng", input->school_number, "r");
	FILE* p_eval_file = open_file("Eval\\Eval", input->school_number, "r");

	// if information of one file is missing, don't open results file and exit function
	if (p_real_file == NULL || p_human_file == NULL || p_eng_file == NULL || p_eval_file == NULL)
		return 1;

	FILE* p_result_file = open_file("Results\\Results", input->school_number, "w");
	if (p_result_file == NULL) return 1;

	// Calculate avarage grades for all students
	int real_grade = 0, human_grade = 0, eng_grade = 0, eval_grade = 0, result_grade = 0;
	
	while ((fscanf(p_real_file, "%d", &real_grade) != EOF) &&
		(fscanf(p_human_file, "%d", &human_grade) != EOF) &&
		(fscanf(p_eng_file, "%d", &eng_grade) != EOF) &&
		(fscanf(p_eval_file, "%d", &eval_grade) != EOF))
	{
		// Calculate the avarage
		result_grade =	((float)input->real_grade_weight / 100) * real_grade +
						((float)input->human_grade_weight / 100) * human_grade +
						((float)input->eng_grade_weight / 100) * eng_grade +
						((float)input->eval_grade_weight / 100) * eval_grade;

		// Print to result file
		fprintf(p_result_file, "%d\n", result_grade);
	}
	
	// Close files
	fclose(p_real_file);
	fclose(p_human_file);
	fclose(p_eng_file);
	fclose(p_eval_file);
	fclose(p_result_file);
	return 0;
}

/// Description: Open file.  
/// Parameters: 
///		file_path 
///		school_number
///		mode - mode for read/write/append.
/// Return: p_file - file pointer for opened file.
static FILE *open_file(char *file_path, int school_number, char *mode)
{
	char file_name[50];
	sprintf(file_name, "%s%d.txt", file_path, school_number);	// file_name is without extension (.txt)

	FILE *p_file = fopen(file_name, mode);
	if (p_file == NULL)
	{
		printf("Error: failed open file %s\nThread number %d failed\n", file_name, school_number);
	}
	return p_file;
}