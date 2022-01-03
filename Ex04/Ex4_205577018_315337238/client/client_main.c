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
#include <ctype.h>
//
#include "..\shared\message_defs.h"
#include "..\shared\socket_handle.h"
#include "..\shared\threads.h"
#include "..\shared\FileApi\file.h"
//
#include "client_send_recv.h"

/************************************
*      definitions                 *
************************************/
#define ASSERT(cond, msg, ...)																\
	do {																					\
		if (!(cond)) {																		\
			printf("Assertion failed at file %s line %d: \n", __FILE__, __LINE__);			\
			printf(msg, __VA_ARGS__);														\
			exit (1);																		\
		}																					\
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

#define LOG_PRINTF(msg, ...)																\
	do {																					\
		printf(msg, __VA_ARGS__);															\
		if (g_client_log_file != NULL)														\
			File_Printf(g_client_log_file, msg, __VA_ARGS__);								\
	} while (0);

/************************************
*       types                       *
************************************/
typedef enum {
	connection_idle,
	connection_succeed,
	connection_denied
} e_connection_state;

/************************************
*      variables                    *
************************************/
static struct {
	char* server_ip;
	uint16_t server_port;
	char* username;
} gs_inputs;

static SOCKET g_client_socket;
static e_connection_state g_connection_state;
static bool g_exit_flag;
static bool g_soft_exit_flag;
static File g_client_log_file;

/************************************
*      static functions             *
************************************/
static void parse_arguments(int argc, char* argv[]);
static void open_log_file(void);
static void handle_connection_menu(void);
static bool wait_for_connection(void);
static int validate_menu_input(char* acceptable_str[], int array_length, char* message);
static char* string_to_lower(char* str);
static void vaildate_user_move(char* accepatble_move, char* message);
static void data_received_handle(s_message_params params);

/************************************
*       API implementation          *
************************************/
// TODO: CHECK FREE IN GENERAL(SOCKETS AND ALLOCATIONS)
// TODO: ASSERTS

/// Description: initiate values and modules, connect to server, create client threads. 
/// Parameters: 
///		[in] argc - number of arguments. 
///		[in] argv - arguments list. 
/// Return: true if succeeded or false otherwise.
int main(int argc, char* argv[])
{
	// parse arguments
	parse_arguments(argc, argv);
	open_log_file();

	g_exit_flag = false;
	g_soft_exit_flag = false;
	g_connection_state = connection_idle;
	HANDLE thread_handels[2] = { NULL, NULL };

	// init client send receive module
	client_init_send_recv(&g_client_socket, &g_soft_exit_flag);
	client_bind_callback(data_received_handle);

	// run loop
	while (g_exit_flag == false)
	{
		// try to connect to server
		g_client_socket = Socket_Init(socket_client, gs_inputs.server_ip, gs_inputs.server_port);

		// connection failed
		if (g_client_socket == INVALID_SOCKET)
		{
			LOG_PRINTF("Failed connecting to server on %s:%d\n", gs_inputs.server_ip, gs_inputs.server_port);

			handle_connection_menu();
			continue;
		}

		// connection succeeded
		LOG_PRINTF("Connected to server on %s:%d\n", gs_inputs.server_ip, gs_inputs.server_port);
		// send client name to the server
		s_message_params message_params = { .message_type = MESSAGE_TYPE_CLIENT_REQUEST, .params_count = 1 };
		message_params.params[0] = gs_inputs.username;
		client_add_transaction(message_params);

		// initiate send and receive threads
		thread_handels[0] = create_new_thread(client_send_routine, NULL);
		thread_handels[1] = create_new_thread(client_receive_routine, NULL);
		if (thread_handels[0] == NULL || thread_handels[1] == NULL)
		{
			LOG_PRINTF("Error: failed creating a thread\n");
			close_handles(thread_handels, 2);
			break;
		}

		// wait for connection succeed
		if (!wait_for_connection())
		{
			// access denied, start again
			Socket_TearDown(g_client_socket, true);
			handle_connection_menu();
			continue;
		}

		// wait for threads to finish
		if (!wait_for_threads(thread_handels, 2, false, INFINITE, false))
		{
			LOG_PRINTF("Error: wait for threads return error\n");
			close_handles(thread_handels, 2);
			break;
		}

		// one thread finished action, close program (softly) 
		g_soft_exit_flag = true;
		wait_for_threads(thread_handels, 2, false, 5000, true);
		break;
	}
	
	// check receive thread exit code to determine if disconnected
	DWORD exitcode;
	if (!GetExitCodeThread(thread_handels[1], &exitcode))
		LOG_PRINTF("Error: GetExitCodeThread return error\n");

	if (exitcode == transfer_disconnected)
		LOG_PRINTF("Server disconnected. Exiting.\n");

	// close all handles
	close_handles(thread_handels, 2);
	File_Close(g_client_log_file);

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

static void open_log_file(void)
{
	int filename_length = snprintf(NULL, 0, "Client_log_%s.txt\n", gs_inputs.username);
	char* file_name = malloc((filename_length) * sizeof(char));
	ASSERT(file_name != NULL, "Error: failed allocating buffer for message\n")

	snprintf(file_name, filename_length, "Client_log_%s.txt\n", gs_inputs.username);
	g_client_log_file = File_Open(file_name, "w");
	free(file_name);
	ASSERT(g_client_log_file != NULL, "Error: failed opening output file\n");
}

/// Description: check for exit request from client.  
/// Parameters: none.
/// Return: none.
static void handle_connection_menu(void)
{
	char* message = "Choose what to do next:\n1.Try to reconnect\n2.Exit\n";
	char* acceptable_chars[] = { "1", "2" };

	// check if need to exit
	if (validate_menu_input(acceptable_chars, 2, message) == 2)
		g_exit_flag = true;
}

/// Description: try to connect to server.
/// Parameters: none.
/// Return: true if connection succeed and false if connection denied.
static bool wait_for_connection(void)
{
	while (g_connection_state != connection_succeed)
	{
		if (g_connection_state == connection_denied)
			return false;
	}
	return true;
}

/// Description: check user's response to main menu. if invalid value - resend the main menu and wait for valid response. 
/// Parameters: 
///		[in] acceptable_str - valid user responses. 
///		[in] array_length - number of valid user responses. 
///		[in] message - main menu message. 
/// Return: none.
static int validate_menu_input(char* acceptable_str[], int array_length, char* message)
{
	do
	{
		// print require message
		printf("%s", message);

		// get user input
		char input[256];
		gets_s(input, sizeof(input));

		// check if input is in the acceptable array
		for (int i = 0; i < array_length; i++)
			if (!strcmp(input, acceptable_str[i]))
				return strtol(input, NULL, 10);
		
		LOG_PRINTF("Error: Illegal command\n");
	} while (1);
}

/// Description: check user's game move validity. if invalid value - resend the next move message and wait for valid response. 
/// Parameters: 
///		[in] accepatble_move - valid user game move. 
///		[in] message - next move message. 
/// Return: none.
static void vaildate_user_move(char* accepatble_move, char* message)
{
	static char boom[] = "boom";
	do
	{
		// print require message
		printf("%s", message);

		// get user input
		char input[256];
		gets_s(input, sizeof(input));

		// check if input is 'boom'
		if (!strcmp(string_to_lower(input), boom) || strspn(input, "0123456789") == strlen(input))
		{
			strcpy(accepatble_move, string_to_lower(input));
			return;
		}

		LOG_PRINTF("Error: Illegal command\n");
	} while (1);
}

/// Description: change string from upper case to lower case.
/// Parameters: 
///		[in] str - upper case string. 
/// Return: lower case string.
static char* string_to_lower(char* str)
{
	for (int i = 0; i < (int)strlen(str); i++)
		str[i] = tolower(str[i]);

	return str;
}

static void data_received_handle(s_message_params message_params)
{
	// TODO: change according to instructions

	// print to console
	switch (message_params.message_type)
	{
		case MESSAGE_TYPE_SERVER_APPROVED:
			g_connection_state = connection_succeed;
			break;
		case MESSAGE_TYPE_SERVER_DENIED:
			g_connection_state = connection_denied;
			printf("Server on %s:%d denied the connection request.\n", gs_inputs.server_ip, gs_inputs.server_port);
			break;
		// TODO: check if needed, becuase I think the server always send MESSAGE_TYPE_SERVER_NO_OPPONENTS and than MESSAGE_TYPE_SERVER_MAIN_MENU
		// case MESSAGE_TYPE_SERVER_NO_OPPONENTS: 
		case MESSAGE_TYPE_SERVER_MAIN_MENU:
		{
			char* message = "Choose what to do next:\n1. Play against another client\n2. Quit\n";
			char* acceptable_chars[] = { "1", "2" };
			switch (validate_menu_input(acceptable_chars, 2, message))
			{
				case 1:
				{
					s_message_params send_message_params = { .message_type = MESSAGE_TYPE_CLIENT_VERSUS };
					client_add_transaction(send_message_params);
					break;
				}
				case 2:
					// quit the program
					g_soft_exit_flag = true;
					break;
			}
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
			char* message = "Enter the next number or boom:\n";
			char send_string[256] = "";
			vaildate_user_move(send_string, message);
			
			// send user move
			s_message_params send_message_params = { .message_type = MESSAGE_TYPE_CLIENT_PLAYER_MOVE, .params_count = 1 };
			send_message_params.params[0] = send_string;
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

/// Description: write message to log file.
/// Parameters: 
///		[in] msg - message to write to log file. 
/// Return: none.
static void log_printf(char* msg)
{
	printf(msg);
	File_Write(g_client_log_file, msg, strlen(msg));
}
