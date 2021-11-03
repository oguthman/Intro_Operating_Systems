#define _CRT_SECURE_NO_WARNINGS
/*!
******************************************************************************
\file main.c
\date 23 October 2021
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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <stdbool.h>

//

/************************************
*      definitions                 *
************************************/
#define NUMBER_OF_BYTES		16
#define WAIT_FOR_PROCESS	5000	// 5 seconds

#define BRUTAL_TERMINATION_CODE 0x55

/************************************
*       types                       *
************************************/

/************************************
*      variables                    *
************************************/
static FILE *pg_message_file = NULL;
static FILE *pg_key_file = NULL;

static char* g_message_file_name = NULL;
static char* g_key_file_name = NULL;

/************************************
*      static functions             *
************************************/
static void parse_arguments(int argc, char *argv[]);
static uint32_t get_file_length(FILE *p_file);
static void run_processes();
static void call_son(PROCESS_INFORMATION* p_procinfo, uint32_t offset);
static bool wait_for_process(PROCESS_INFORMATION* p_procinfo);
static void close_handles(PROCESS_INFORMATION* procinfo);

static void close_files(void);

/************************************
*       API implementation          *
************************************/
int main(int argc, char *argv[])
{
	parse_arguments(argc, argv);
	run_processes();
	printf("Father program has finished!");
	
	return 0;
}

/************************************
* static implementation             *
************************************/
static void parse_arguments(int argc, char* argv[])
{
	// check if there is exact args.
	if (argc != 3)
	{
		printf("Error: not exact args number. \n");
		exit(1);
	}
	
	// open message file
	if ((pg_message_file = fopen(argv[1], "r")) == NULL)
	{
		printf("Error: failed opening file. \n");
		exit(1);
	}

	g_message_file_name = argv[1];
	g_key_file_name = argv[2];
}

static uint32_t get_file_length(FILE *p_file)
{
	fseek(p_file, 0, SEEK_END);
	return ftell(p_file);
}

static void run_processes()
{
	uint32_t size = get_file_length(pg_message_file);
	fclose(pg_message_file);

	printf("Father: The size of the file is %d, so we call son's process %d times\n", size, size / 16);
	
	for (uint32_t offset = 0; offset < size; offset += 16)
	{
		PROCESS_INFORMATION procinfo;
		DWORD exitcode;

		call_son(&procinfo, offset);

		// Wait until child process exits.
		bool process_state = wait_for_process(&procinfo);
		
		// Close process and thread handles. 
		close_handles(&procinfo);

		// If process not finished successfully, exit the program.
		if (!process_state)
		{
			printf("Error: Son process didn't finish successfully. \n");
			exit(1);
		}
	}
}

static void call_son(PROCESS_INFORMATION *p_procinfo, uint32_t offset)
{
	// get cmd str lenght
	int length = snprintf(NULL, 0, "Son.exe \"%s\" %d \"%s\"", g_message_file_name, offset, g_key_file_name);
	wchar_t *cmd = malloc((length + 1)*sizeof(wchar_t));
	if (NULL == cmd)
	{
		printf("Error: failed allocating command \n");
		exit(1);
	}

	// write string to cmd buffer https://stackoverflow.com/questions/4826189/convert-char-to-wchar-in-c
	swprintf(cmd, length + 1, L"Son.exe \"%hs\" %d \"%hs\"", g_message_file_name, offset, g_key_file_name);
	//swprintf(cmd, length + 1, L"%hs", "notepad.exe");
	
	// create new process
	STARTUPINFO startinfo = { sizeof(STARTUPINFO), NULL, 0 };
	bool success = CreateProcess(
		NULL,			// No module name (use command line)
		cmd,			// Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&startinfo,     // Pointer to STARTUPINFO structure
		p_procinfo);    // Pointer to PROCESS_INFORMATION structure

	// free allocation
	free(cmd);

	// check if process created succesfully
	if (!success)
	{
		printf("Error: process creation failure \n");
		exit(1);
	}

}

static bool wait_for_process(PROCESS_INFORMATION* p_procinfo)
{
	DWORD exitcode;
	// Wait until child process exits.
	DWORD waitcode = WaitForSingleObject(p_procinfo->hProcess, WAIT_FOR_PROCESS);


	switch (waitcode)
	{
		case WAIT_TIMEOUT:
		{
			printf("Father: Process not terminated after %d ms, Father will terminate the son.\n", WAIT_FOR_PROCESS);
			
			// Terminating process with an exit code of 55h
			TerminateProcess(p_procinfo->hProcess, BRUTAL_TERMINATION_CODE);
			
			// Wait a few milliseconds for the process to terminate,
			// the above command may also fail, so another WaitForSingleObject is required.
			// We skip this for brevity
			Sleep(10); 
			return false;
		}
		case WAIT_OBJECT_0:
		{
			printf("Father: Process has finished.\n");
			GetExitCodeProcess(p_procinfo->hProcess, &exitcode);
			printf("Father: The exit code for the process is 0x%x\n", exitcode);
			return (exitcode == 0) ? true : false;
		}
		default:
		{
			printf("Father: WaitForSingleObject has returned with code %d. The program will exit\n", waitcode);
			return false;
		}
	}
}

static void close_handles(PROCESS_INFORMATION *procinfo)
{
	CloseHandle(procinfo->hThread);
	CloseHandle(procinfo->hProcess);
}


static void close_files(void)
{
	fclose(pg_message_file);
	fclose(pg_key_file);
}

