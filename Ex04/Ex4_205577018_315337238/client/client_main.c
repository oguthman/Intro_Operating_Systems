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
#include "client_UI.h"

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

#define LOG_PRINTF(msg, ...)																\
	do {																					\
		printf(msg, __VA_ARGS__);															\
		if (g_client_log_file != NULL)														\
			File_Printf(g_client_log_file, msg, __VA_ARGS__);								\
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
static void data_received_handle(s_message_params params);
static void data_send_handle(s_message_params* params, uint32_t timeout);

/************************************
*       API implementation          *
************************************/
// TODO: CHECK FREE IN GENERAL(SOCKETS AND ALLOCATIONS)
// TODO: ASSERTS
// TODO: ADD SEND/RECV BUFFER TO LOG
// TODO: VERIFY PRINTS TO LOG

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
	HANDLE thread_handels[3] = { NULL, NULL, NULL };

	// init client send receive module
	ASSERT(client_init_send_recv(&g_client_socket, &g_soft_exit_flag), "Error: failed initiate client_send_recv module\n");
	client_bind_callback(data_received_handle);
	client_set_receive_event(true, 15 * 1000);
	//
	ASSERT(clientUI_init(&g_soft_exit_flag, &g_connection_state, (s_server_data*)&gs_inputs, &g_client_log_file), "Error: failed initiate clientUI module\n");
	clientUI_bind_send_callback(data_send_handle);
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
		thread_handels[2] = create_new_thread(clientUI_routine, NULL);
		if (thread_handels[0] == NULL || thread_handels[1] == NULL || thread_handels[2] == NULL)
		{
			LOG_PRINTF("Error: failed creating a thread\n");
			close_handles(thread_handels, 3);
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
		if (!wait_for_threads(thread_handels, 3, false, INFINITE, false))
		{
			LOG_PRINTF("Error: wait for threads return error\n");
			close_handles(thread_handels, 3);
			break;
		}

		// check reconnection


		// one thread finished action, close program (softly) 
		g_soft_exit_flag = true;
		wait_for_threads(thread_handels, 3, false, 5000, true);
		break;
	}
	
	// check receive thread exit code to determine if disconnected
	DWORD exitcode;
	if (!GetExitCodeThread(thread_handels[1], &exitcode))
		LOG_PRINTF("Error: GetExitCodeThread return error\n");

	if (exitcode == transfer_disconnected)
		LOG_PRINTF("Server disconnected. Exiting.\n");

	// close all handles
	close_handles(thread_handels, 3);
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
	if (argc == 1)
	{
		static char username[20] = "";
		sprintf(username, "ofir_%d", rand());
		// parse arguments
		gs_inputs.server_ip = "127.0.0.1";
		gs_inputs.server_port = 8888;
		gs_inputs.username = username;
		return;
	}
	
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
	if (clientUI_validate_menu_input(acceptable_chars, 2, message) == 2)
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

static void data_received_handle(s_message_params params)
{
	if (!clientUI_add_message(params))
	{
		LOG_PRINTF("Error: failed add message to UI. Exiting.\n");
		g_soft_exit_flag = true;
	}
	//
	// reset the receive event until timeout update
	client_set_receive_event(false, 15 * 1000);
}

static void data_send_handle(s_message_params* params, uint32_t timeout)
{
	if (params != NULL)
	{
		if (!client_add_transaction(*params))
		{
			LOG_PRINTF("Error: failed add message to UI. Exiting.\n");
			g_soft_exit_flag = true;
		}
	}
	//
	// set the receive event with the updated timeout value
	client_set_receive_event(true, timeout);
}

