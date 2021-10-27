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

/************************************
*       types                       *
************************************/

/************************************
*      variables                    *
************************************/
static FILE *gMessageFile = NULL;
static FILE *gKeyFile = NULL;

static char* gMessageFileName = NULL;
static char* gKeyFileName = NULL;

/************************************
*      static functions             *
************************************/
static void parse_arguments(int argc, char* argv[]);
static uint32_t get_file_length(FILE *file);
static void run_processes();
static void call_Son();




static void close_files(void);

/************************************
*       API implementation          *
************************************/
int main(int argc, char *argv[])
{
	parse_arguments(argc, argv[]);
	run_processes();
		
	
	
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
	if ((gMessageFile = fopen(argv[1], "r")) == NULL)
	{
		printf("Error: failed opening file. \n");
		exit(1);
	}

	gMessageFileName = argv[1];
	gKeyFileName = argv[2];
}

static uint32_t get_file_length(FILE* file)
{
	fseek(file, 0, SEEK_END);
	return ftell(file);
}

static void run_processes()
{
	uint32_t size = get_file_length(gMessageFile);
	fclose(gMessageFile);
	
	for (uint32_t offset = 0; offset < size; offset += 16)
	{
		PROCESS_INFORMATION procinfo;
		call_Son(&procinfo, offset);

	}
}

static void call_Son(PROCESS_INFORMATION *procinfo, uint32_t offset)
{
	// get cmd str lenght
	int length = snprintf(NULL, 0, "Son.exe %s %d %s", gMessageFileName, offset, gKeyFileName);
	char *cmd = malloc(length + 1);
	if (NULL == cmd)
	{
		printf("Error: failed allocating command \n");
		exit(1);
	}

	// write string to cmd buffer
	snprintf(cmd, length + 1, "Son.exe %s %d %s", gMessageFileName, offset, gKeyFileName);
	
	// create new process
	STARTUPINFO satartinfo = { sizeof(STARTUPINFO), NULL, 0 };

	bool success = CreateProcess(
		NULL,			// No module name (use command line)
		cmd,			// Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&satartinfo,     // Pointer to STARTUPINFO structure
		procinfo);     // Pointer to PROCESS_INFORMATION structure



	// free alocation
	free(cmd);
}

if (!CreateProcess(NULL,   // No module name (use command line)
    command,        // Command line
    NULL,           // Process handle not inheritable
    NULL,           // Thread handle not inheritable
    FALSE,          // Set handle inheritance to FALSE
    0,              // No creation flags
    NULL,           // Use parent's environment block
    NULL,           // Use parent's starting directory 
    &atartinfo,            // Pointer to STARTUPINFO structure
    &procinfo)           // Pointer to PROCESS_INFORMATION structure
    )
{
    printf("CreateProcess failed (%d).\n", GetLastError());
    return;
}

// Wait until child process exits.
WaitForSingleObject(pi.hProcess, INFINITE);

// Close process and thread handles. 
CloseHandle(pi.hProcess);
CloseHandle(pi.hThread);

static void close_files(void)
{
	fclose(gMessageFile);
	fclose(gKeyFile);
}