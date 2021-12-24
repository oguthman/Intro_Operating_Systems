#define _CRT_SECURE_NO_WARNINGS
/*!
******************************************************************************
\file main.c
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
#include <stdio.h>
#include <string.h>
//
#include "..\shared\message_defs.h"
#include "..\shared\socket_handle.h"

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
	char* server_ip;
	char* server_port;
	char* username;
} gs_inputs;

/************************************
*      static functions             *
************************************/
static void parse_arguments(int argc, char* argv[]);
static void validate_menu_input(int* value, int min_arg, int max_arg, char* message);
static void clear_buffer();

/************************************
*       API implementation          *
************************************/
int main(int argc, char* argv[])
{
	// parse arguments
	parse_arguments(argc, argv);
	
	int answer_to_reconnect = 0;
	
	do
	{
		// connect to server
		// if succeeded
		{
			printf("Connected to server on <ip>:<port>\n");
			//break;
		}
		// if not succeeded
		printf("Failed connecting to server on <ip>:<port>\nChoose what to do next:\n1. Try to reconnect\n2. Exit\n");
		scanf_s("%d", &answer_to_reconnect);
		validate_menu_input(&answer_to_reconnect, 1, 2, "Choose what to do next:\n1. Try to reconnect\n2. Exit\n");

	} while (answer_to_reconnect == 1);
	

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
	//// check if there are enough arguments
	//ASSERT(argc == 4, "Error: not enough arguments.\n");

	//// parse arguments
	//gs_argument_inputs.virtual_memory_size = 1 << (strtol(argv[1], NULL, 10) - PAGE_SIZE);
	//gs_argument_inputs.physical_memory_size = 1 << (strtol(argv[2], NULL, 10) - PAGE_SIZE);
	//gs_argument_inputs.input_file = File_Open(argv[3], "r");
	//ASSERT(gs_argument_inputs.input_file != NULL, "Error: Can't open input file\n");
}

// TODO: fix the string as an input
static void validate_menu_input(int* value, int min_arg, int max_arg, char* message)
{
	while (*value > max_arg || *value < min_arg)
	{
		char input[20];
		printf("Error: Illegal command\n");
		//clear_buffer();
		printf("%s", message);
		gets(input);
		*value = strtol(input, NULL, 10);
	}
}

static void clear_buffer()
{
	fseek(stdin, 0, SEEK_END);
}
