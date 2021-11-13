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

static DWORD WINAPI routine_func(LPVOID lpParam)
{
	s_thread_inputs *input = (s_thread_inputs *)lpParam;
	printf("Starting the routine for school number %d\n", input->school_number);
	
	// Open Files
	FILE* real_file = open_file("Real\\Real", input->school_number, "r");
	FILE* human_file = open_file("Human\\Human", input->school_number, "r");
	FILE* eng_file = open_file("Eng\\Eng", input->school_number, "r");
	FILE* eval_file = open_file("Eval\\Eval", input->school_number, "r");

	// if information one file if missing, dont open results file
	if (real_file == NULL || human_file == NULL || eng_file == NULL || eval_file == NULL)
		return 1;

	FILE* result_file = open_file("Results\\Results", input->school_number, "w");
	if (result_file == NULL) return 1;

	// Calculate avarage grades for all students
	int real_grade = 0, human_grade = 0, eng_grade = 0, eval_grade = 0, result_grade = 0;
	
	while ((fscanf(real_file, "%d", &real_grade) != EOF) &&
		(fscanf(human_file, "%d", &human_grade) != EOF) &&
		(fscanf(eng_file, "%d", &eng_grade) != EOF) &&
		(fscanf(eval_file, "%d", &eval_grade) != EOF))
	{

		// calculate the avarage
		result_grade =	((float)input->real_grade_weight / 100) * real_grade +
						((float)input->human_grade_weight / 100) * human_grade +
						((float)input->eng_grade_weight / 100) * eng_grade +
						((float)input->eval_grade_weight / 100) * eval_grade;

		// print to result file
		fprintf(result_file, "%d\n", result_grade);
	}
	
	// Close files
	fclose(real_file);
	fclose(human_file);
	fclose(eng_file);
	fclose(eval_file);
	fclose(result_file);
	return 0;
}

static FILE *open_file(char *file_path, int school_number, char *mode)
{
	char file_name[50];
	sprintf(file_name, "%s%d.txt", file_path, school_number);	// file_name is without extension (.txt)

	FILE *file = fopen(file_name, mode);
	if (file == NULL)
	{
		printf("Error: failed open file %s\nThread number %d failed\n", file_name, school_number);
	}
	return file;
}

