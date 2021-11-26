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
#include <string.h>

#include "thread.h"
#include "FileApi\file.h"

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
static HANDLE g_mutex_handle;

/************************************
*      static functions             *
************************************/
static HANDLE create_new_thread(LPTHREAD_START_ROUTINE p_start_routine, LPVOID p_thread_parameters, LPDWORD p_thread_id);
static DWORD WINAPI routine_func(LPVOID lpParam);
static File* open_file(char * file_path, int school_number, char *mode);
bool read_file_grade(File* file, int* grade);

/************************************
*       API implementation          *
************************************/
bool InitiateMutex(void)
{
	/* Create the mutex that will be used to synchronize access to count */
	g_mutex_handle = CreateMutex(
		NULL,	/* default security attributes */
		FALSE,	/* initially not owned */
		NULL);	/* unnamed mutex */
	if (NULL == g_mutex_handle)
	{
		printf("Error when creating mutex: %d\n", GetLastError());
		return false;
	}
	return true;
}

HANDLE OpenNewThread(s_thread_inputs *input)
{
	// Create Thread, pass the routine function
	DWORD thread_id;
	return create_new_thread(routine_func, input, &thread_id);
}

void CloseMutex(void)
{
	CloseHandle(g_mutex_handle);
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
	File* p_real_file = open_file("Real\\Real", input->school_number, "r");
	File* p_human_file = open_file("Human\\Human", input->school_number, "r");
	File* p_eng_file = open_file("Eng\\Eng", input->school_number, "r");
	File* p_eval_file = open_file("Eval\\Eval", input->school_number, "r");

	// if information of one file is missing, don't open results file and exit function
	if (p_real_file == NULL || p_human_file == NULL || p_eng_file == NULL || p_eval_file == NULL)
		return 1;

	File* p_result_file = open_file("Results\\Results", input->school_number, "w");
	if (p_result_file == NULL) return 1;

	// Calculate avarage grades for all students
	int real_grade = 0, human_grade = 0, eng_grade = 0, eval_grade = 0, result_grade = 0;
	
	while ((read_file_grade(p_real_file, &real_grade) == true) &&
		(read_file_grade(p_human_file, &human_grade) == true) &&
		(read_file_grade(p_eng_file, &eng_grade) == true) &&
		(read_file_grade(p_eval_file, &eval_grade) == true))
	{
		// Calculate the avarage
		result_grade = (input->real_grade_weight * real_grade +
			input->human_grade_weight * human_grade +
			input->eng_grade_weight * eng_grade +
			input->eval_grade_weight * eval_grade) / 100;

		// Print to result file
		char line[10];
		sprintf(line, "%d\n", result_grade);
		File_Write(p_result_file, line, (int)strlen(line));
	}
	
	// Close files
	File_Close(p_real_file);
	File_Close(p_human_file);
	File_Close(p_eng_file);
	File_Close(p_eval_file);
	File_Close(p_result_file);
	return 0;
}

/// Description: Open file.  
/// Parameters: 
///		file_path 
///		school_number
///		mode - mode for read/write/append.
/// Return: p_file - file pointer for opened file.
static File *open_file(char *file_path, int school_number, char *mode)
{
	char file_name[50];
	sprintf(file_name, "%s%d.txt", file_path, school_number);	// file_name is without extension (.txt)

	File *p_file = File_Open(file_name, mode);
	if (p_file == NULL)
	{
		printf("Error: failed open file %s\nThread number %d failed\n", file_name, school_number);
	}
	return p_file;
}

/// Description: Open file.  
/// Parameters: 
///		file - pointer to a file 
///		grade - pointer to grade
/// Return: bool if read grade succeeded or false otherwise
bool read_file_grade(File* file, int *grade)
{
	// Wait for the mutex to become available, then take ownership.
	DWORD wait_code = WaitForSingleObject(g_mutex_handle, INFINITE);
	if (WAIT_OBJECT_0 != wait_code)
	{
		printf("Error when waiting for mutex\n");
		return false;
	}
	
	int max_length = 10;
	char *line = (char*)malloc((max_length) * sizeof(char));

	bool file_res = false;
	if (line != NULL)
	{
		bool file_res = (File_ReadLine(file, line, max_length) && (sscanf(line, "%d", grade) != 0));
		free(line);

		if (file_res == false)
		{
			ReleaseMutex(g_mutex_handle);
			return false;
		}
	}

	// Releasing the mutex.
	DWORD ret_val = ReleaseMutex(g_mutex_handle);
	if (FALSE == ret_val)
	{
		printf("Error when releasing the mutex\n");
		return false;
	}

	return true;
}