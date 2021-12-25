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

static struct {
	e_socket_type type;
	e_message_type message_type;
	int params_count;
	char* params[];
} gs_send_recieve_params;

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

	HANDLE handles[3];

	int answer_to_reconnect = 0;

	do
	{
		// connect to server
		SOCKET socket = Socket_Init(socket_client, gs_inputs.server_ip, gs_inputs.server_port);
		if (socket != NULL)
			// if succeeded
		{
			printf("Connected to server on %s:%s\n", gs_inputs.server_ip, gs_inputs.server_port);
			// write to log: ("Connected to server on %s:%s\n", gs_inputs.server_ip, gs_inputs.server_port);
			break;
		}
		// if not succeeded
		printf("Failed connecting to server on %s:%s\n", gs_inputs.server_ip, gs_inputs.server_port);
		printf("Choose what to do next:\n1.Try to reconnect\n2.Exit\n");
		// write to log: ("Failed connecting to server on %s:%s\n", gs_inputs.server_ip, gs_inputs.server_port);
		scanf_s("%d", &answer_to_reconnect);
		// TODO: fix the function
		validate_menu_input(&answer_to_reconnect, 1, 2, "Choose what to do next:\n1. Try to reconnect\n2. Exit\n");

	} while (answer_to_reconnect == 1);

	handles[0] = create_new_thread(Socket_Send, gs_send_recieve_params);
	handles[1] = create_new_thread(Socket_Receive, gs_send_recieve_params);
	handles[1] = create_new_thread(UI, PARMAS);

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

/// Description: create new thread.  
/// Parameters: 
///		[in] p_start_routine - thread function. 
///		[in] p_thread_parameters - parametes for thread function.
/// Return: thread_handle - handle for the new thread.
static HANDLE create_new_thread(LPTHREAD_START_ROUTINE p_function, LPVOID p_thread_parameters)
{
	HANDLE thread_handle;
	DWORD thread_id;

	if (p_function == NULL)
	{
		printf("Error: failed creating a thread\nReceived NULL pointer\n");
		return NULL;
	}

	thread_handle = CreateThread(
		NULL,                /*  default security attributes */
		0,                   /*  use default stack size */
		p_function,			 /*  thread function */
		p_thread_parameters, /*  argument to thread function */
		0,                   /*  use default creation flags */
		&thread_id);         /*  returns the thread identifier */

	return thread_handle;
}