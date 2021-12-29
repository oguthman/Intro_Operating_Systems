#define _WINSOCK_DEPRECATED_NO_WARNINGS
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
#include "client_send_recv.h"
#include "..\shared\threads.h"

/************************************
*      definitions                 *
************************************/
#define ASSERT(cond, msg, ...)														\
	do {																			\
		if (!(cond)) {																\
			printf("Assertion failed at file %s line %d: \n", __FILE__, __LINE__);	\
			printf(msg, __VA_ARGS__);												\
			exit (1);																\
		}																			\
	} while (0);

// TODO: check return/exit condition
#define THREAD_ASSERT(cond, msg, ...)														\
	do {																					\
		if (!(cond)) {																		\
			printf("Thread Assertion failed at file %s line %d: \n", __FILE__, __LINE__);	\
			printf(msg, __VA_ARGS__);														\
			return 1;																		\
		}																					\
	} while (0);

/************************************
*       types                       *
************************************/

/************************************
*      variables                    *
************************************/
static struct {
	char* server_ip;
	uint16_t server_port;
	char* username;
} gs_inputs;

static SOCKET g_client_socket;

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
static void data_received_handle(s_client_message_params params);

/************************************
*       API implementation          *
************************************/
int main(int argc, char* argv[])
{
	// parse arguments
	parse_arguments(argc, argv);

	HANDLE handles[2];

	int answer_to_reconnect = 0;

	do
	{
		// connect to server
		g_client_socket = Socket_Init(socket_client, gs_inputs.server_ip, gs_inputs.server_port);
		// if succeeded
		if (g_client_socket != INVALID_SOCKET)
		{
			printf("Connected to server on %s:%d\n", gs_inputs.server_ip, gs_inputs.server_port);
			// write to log: ("Connected to server on %s:%s\n", gs_inputs.server_ip, gs_inputs.server_port);
			break;
		}
		// if not succeeded
		printf("Failed connecting to server on %s:%d\n", gs_inputs.server_ip, gs_inputs.server_port);
		printf("Choose what to do next:\n1.Try to reconnect\n2.Exit\n");
		// write to log: ("Failed connecting to server on %s:%s\n", gs_inputs.server_ip, gs_inputs.server_port);
		scanf_s("%d", &answer_to_reconnect);
		// TODO: fix the function
		validate_menu_input(&answer_to_reconnect, 1, 2, "Choose what to do next:\n1. Try to reconnect\n2. Exit\n");

	} while (answer_to_reconnect == 1);

	
	// init client send receive module
	client_init_send_recv(g_client_socket);
	client_bind_callback(data_received_handle);

	handles[0] = create_new_thread(client_send_routine, NULL);
	THREAD_ASSERT(handles[0] != NULL, "Error: failed creating a thread\n");
	
	handles[1] = create_new_thread(client_receive_routine, NULL);
	THREAD_ASSERT(handles[1] != NULL, "Error: failed creating a thread\n");

	// temp
	WaitForMultipleObjects(2, handles, false, INFINITE);

	CloseHandle(handles[0]);
	CloseHandle(handles[1]);

	// on exit
	client_teardown();
}



/************************************
* static implementation             *
************************************/
/// Description: parse arguments and open files.  
/// Parameters: 
///		[in] argc - number of arguments. 
///		[in] argv - arguments list. 
/// Return: none.
static void parse_arguments(int argc, char* argv[]) // TODO: add username
{
	// check if there are enough arguments
	ASSERT(argc == 3, "Error: not enough arguments.\n");

	// parse arguments
	gs_inputs.server_ip = argv[1];
	gs_inputs.server_port = strtol(argv[2], NULL, 10);
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
		scanf("%s", input);
		*value = strtol(input, NULL, 10);
	}
}

static void data_received_handle(s_client_message_params params)
{
	printf("Client: message received '[%d] %s'", params.message_type, get_message_str(params.message_type));
}

