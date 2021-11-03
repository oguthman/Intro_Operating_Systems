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
static void call_son();




static void close_files(void);

/************************************
*       API implementation          *
************************************/
int main(int argc, char *argv[])
{
	parse_arguments(argc, argv);
	run_processes();
	printf("finished!");
	
	
	close_files();
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

	printf("The size of the file is %d, so we call son process %d times", size, size / 16);
	
	for (uint32_t offset = 0; offset < size; offset += 16)
	{
		PROCESS_INFORMATION procinfo;
		call_son(&procinfo, offset);

		// Wait until child process exits.
		WaitForSingleObject(procinfo.hProcess, WAIT_FOR_PROCESS);

		// Close process and thread handles. 
		CloseHandle(procinfo.hThread);
		CloseHandle(procinfo.hProcess);
	}
}

static void call_son(PROCESS_INFORMATION *p_procinfo, uint32_t offset)
{
	// get cmd str lenght
	//int length = snprintf(NULL, 0, "/.Son.exe \"%s\" %d \"%s\"", g_message_file_name, offset, g_key_file_name);
	//char *cmd = malloc(length + 1);
	//if (NULL == cmd)
	//{
	//	printf("Error: failed allocating command \n");
	//	exit(1);
	//}

	//// write string to cmd buffer
	//snprintf(cmd, length + 1, "/.Son.exe \"%s\" %d \"%s\"", g_message_file_name, offset, g_key_file_name);
	//
	LPWSTR cmd = "notepad.exe ../log.txt";

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

static void close_files(void)
{
	fclose(pg_message_file);
	fclose(pg_key_file);
}