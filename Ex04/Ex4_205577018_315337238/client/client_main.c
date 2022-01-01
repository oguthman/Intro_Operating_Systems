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
// TODO: CHECK FREE IN GENERAL(SOCKETS AND ALLOCATIONS)
// TODO: ASSERTS

int main(int argc, char* argv[])
{
	// parse arguments
	parse_arguments(argc, argv);
	
	// init modules
	// init client send receive module
	client_init_send_recv(&g_client_socket);
	client_bind_callback(data_received_handle);
		
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
			// send my name to the server
			char* params[] = { gs_inputs.username };
			s_client_message_params message_params = { MESSAGE_TYPE_CLIENT_REQUEST, 1, params };
			client_add_transaction(message_params);

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
static void parse_arguments(int argc, char* argv[])
{
	// check if there are enough arguments
	ASSERT(argc == 4, "Error: not enough arguments.\n");

	// parse arguments
	gs_inputs.server_ip = argv[1];
	gs_inputs.server_port = (uint16_t)strtol(argv[2], NULL, 10);
	gs_inputs.username = argv[3];
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

static void data_received_handle(s_client_message_params message_params)
{
	// TODO: change according to instructions
	printf("Client: message received '[%d] %s'", message_params.message_type, get_message_str(message_params.message_type));
	for (uint8_t i = 0; i < message_params.params_count; i++)
		printf("params[%d] = %s\n", i, message_params.params[i]);

	// print to console
	switch (message_params.message_type)
	{
		// case MESSAGE_TYPE_SERVER_APPROVED:
		case MESSAGE_TYPE_SERVER_DENIED:
			printf("Server on %s:%d denied the connection request.\n", gs_inputs.server_ip, gs_inputs.server_port);
			break;
		case MESSAGE_TYPE_SERVER_MAIN_MENU:
		case MESSAGE_TYPE_SERVER_NO_OPPONENTS:
		{
			printf("Choose what to do next:\n1. Play against another client\n2. Quit\n");
			char send_string[256];
			// TODO: check if need validation
			gets_s(send_string, sizeof(send_string));

			if (!strcmp(send_string, "1"))
			{
				s_client_message_params send_message_params = { MESSAGE_TYPE_CLIENT_VERSUS, 0, NULL };
				client_add_transaction(send_message_params);
			}

			// TODO: flag for close the client process

			break;
		}
		case MESSAGE_TYPE_GAME_STARTED:
			printf("Game is on!\n");
			break;
		case MESSAGE_TYPE_TURN_SWITCH:
			// TODO: maybe shouhld check the num_of_params
			if (!strcmp(message_params.params[0], gs_inputs.username))
				printf("Your turn!\n");
			else
				printf("%s's turn!\n", message_params.params[0]);
			break;
		case MESSAGE_TYPE_SERVER_MOVE_REQUEST:
		{
			printf("Enter the next number or boom:\n");
			char send_string[256];
			// TODO: check if need validation
			gets_s(send_string, sizeof(send_string));

			char* params[1] = { send_string };
			s_client_message_params send_message_params = { MESSAGE_TYPE_CLIENT_PLAYER_MOVE, 1, params };
			client_add_transaction(send_message_params);
			break;
		}
		case MESSAGE_TYPE_GAME_ENDED:
			printf("%s won!\n", message_params.params[0]);
			break;
		case MESSAGE_TYPE_GAME_VIEW:
			// TODO: maybe shouhld check the num_of_params
			printf("%s move was %s\n", message_params.params[0], message_params.params[1]);
			if (!strcmp(message_params.params[2], "END"))
				printf("END\n");
			else
			{
				//TODO: CHECK IF NEEDED TO RETURN PRINT
				printf("CONT\n");
			}
			break;
		case MESSAGE_TYPE_SERVER_OPPONENT_QUIT:
			printf("Opponent quit.\n");	//TODO: MAKE SURE THE PRINT IS CORRECT "Opponent quit .\n"
			break;
	}
}

static void send_user_response(e_message_type message_type)
{
	char send_string[256];
	gets_s(send_string, sizeof(send_string));

	// TODO: check if need validation
	char* params[1] = { send_string };
	s_client_message_params send_message_params = { message_type, 1, params };
	client_add_transaction(send_message_params);
}
