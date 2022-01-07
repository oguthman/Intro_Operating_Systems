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
#include "send_recv_routine.h"
#include "client_UI.h"

/************************************
*      definitions                 *
************************************/
#define LOG_PRINTF(msg, ...)																\
	do {																					\
		printf(msg, __VA_ARGS__);															\
		if (g_client_log_file != NULL)														\
			File_Printf(g_client_log_file, msg, __VA_ARGS__);								\
	} while (0);

#define ASSERT(cond, msg, ...)																\
	do {																					\
		if (!(cond)) {																		\
			printf("Assertion failed at file %s line %d: \n", __FILE__, __LINE__);			\
			LOG_PRINTF(msg, __VA_ARGS__);													\
			exit (1);																		\
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
static bool wait_for_server_accept(void);
static void data_received_handle(s_message_params params);
static void data_send_handle(s_message_params* params, uint32_t timeout);
static void print_socket_data(SOCKET socker_originator, char* print_originator, char* string);

/************************************
*       API implementation          *
************************************/
/// Description: initiate values and modules, connect to server, create client threads. 
/// Parameters: 
///		[in] argc - number of arguments. 
///		[in] argv - arguments list. 
/// Return: true if succeeded or false otherwise.
int main(int argc, char* argv[])
{
	int exit_code = 0;
	// parse arguments
	parse_arguments(argc, argv);
	open_log_file();

	g_exit_flag = false;
	g_soft_exit_flag = false;
	g_connection_state = connection_idle;
	HANDLE thread_handels[3] = { NULL, NULL, NULL };

	// init client send receive module
	ASSERT(SendRecvRoutine_Init(&g_client_socket, &g_soft_exit_flag), "Error: failed initiate SendRecvRoutine module\n");
	SendRecvRoutine_BindCallback(data_received_handle);
	ASSERT(SendRecvRoutine_SetReceiveEvent(true, 15 * 1000), "Error: failed to set receive event\n");
	
	ASSERT(ClientUI_Init(&g_soft_exit_flag, &g_connection_state, (s_server_data*)&gs_inputs, &g_client_log_file), "Error: failed initiate clientUI module\n");
	ClientUI_BindSendCallback(data_send_handle);
	
	Socket_BindSocketPrintCallback(print_socket_data);

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
		if (!SendRecvRoutine_AddTransaction(message_params))
		{
			LOG_PRINTF("Error: failed adding new transaction to the send routine\n");
			exit_code = 1;
			break;
		}

		// initiate send and receive threads
		thread_handels[0] = create_new_thread(SendRecvRoutine_SendRoutine, NULL);
		thread_handels[1] = create_new_thread(SendRecvRoutine_ReceiveRoutine, NULL);
		thread_handels[2] = create_new_thread(ClientUI_Routine, NULL);
		if (thread_handels[0] == NULL || thread_handels[1] == NULL || thread_handels[2] == NULL)
		{
			LOG_PRINTF("Error: failed creating a thread\n");
			exit_code = 1;
			break;
		}

		// wait for connection succeed
		// verify server return approved code
		if (!wait_for_server_accept())
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
			exit_code = 1;
			break;
		}

		// one thread finished action, close program (softly) 
		g_soft_exit_flag = true;
		wait_for_threads(thread_handels, 3, false, 5000, true);		// not checking the return value, exit anyway
		break;
	}
	
	if (thread_handels[1] != NULL)
	{
		// check receive thread exit code to determine if disconnected
		DWORD thread_exitcode;
		if (!GetExitCodeThread(thread_handels[1], &thread_exitcode))
		{
			LOG_PRINTF("Error: GetExitCodeThread return error\n");
			exit_code = 1;
		}

		if (thread_exitcode == transfer_disconnected || thread_exitcode == transfer_timeout)
		{
			LOG_PRINTF("Server disconnected. Exiting.\n");
			exit_code = 1;
		}
	}

	// close all handles
	close_handles(thread_handels, 3);
	File_Close(g_client_log_file);

	// on exit
	SendRecvRoutine_Teardown();
	return exit_code;
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
	if (ClientUI_ValidateMenuInput(acceptable_chars, 2, message) == 2)
		g_exit_flag = true;
}

/// Description: wait for server to accept the connection.
/// Parameters: none.
/// Return: true if connection succeed and false if connection denied.
static bool wait_for_server_accept(void)
{
	while (g_connection_state != connection_succeed)
	{
		if (g_connection_state == connection_denied)
			return false;
	}
	return true;
}

/// Description: handle data received from recieve_routine.
/// Parameters: 
///		[in] params - the message params from server.
/// Return: none.
static void data_received_handle(s_message_params params)
{
	if (!ClientUI_AddMessage(params))
	{
		LOG_PRINTF("Error: failed adding message to UI. Exiting.\n");
		g_soft_exit_flag = true;
	}
	//
	// reset the receive event until timeout update
	if (!SendRecvRoutine_SetReceiveEvent(false, 15 * 1000))
	{
		LOG_PRINTF("Error: failed to set receive event. Exiting.\n");
		g_soft_exit_flag = true;
	}
}

/// Description: handle data sending to server.
/// Parameters: 
///		[in] params - the message params to server.
///		[in] timeout - the timeout for waiting server response.
/// Return: none.
static void data_send_handle(s_message_params* params, uint32_t timeout)
{
	if (params != NULL)
	{
		if (!SendRecvRoutine_AddTransaction(*params))
		{
			LOG_PRINTF("Error: failed add message to UI. Exiting.\n");
			g_soft_exit_flag = true;
		}
	}
	//
	// set the receive event with the updated timeout value
	if (!SendRecvRoutine_SetReceiveEvent(true, timeout))
	{
		LOG_PRINTF("Error: failed to set receive event. Exiting.\n");
		g_soft_exit_flag = true;
	}
}

/// Description: handle socket data for printing.
/// Parameters: 
///		[in] socker_originator - the send/recv socket.
///		[in] print_originator - send/receive string.
///		[in] string - the data from socket to print.
/// Return: none.
static void print_socket_data(SOCKET socker_originator, char* print_originator, char* string)
{
	if (g_client_socket == socker_originator)
	{
		if (g_client_log_file != NULL)
			File_Printf(g_client_log_file, "%s server-%s\n", print_originator, string);
	}
}
