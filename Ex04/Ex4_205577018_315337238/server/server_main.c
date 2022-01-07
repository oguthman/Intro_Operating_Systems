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
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
//
#include "../shared/socket_handle.h"
#include "../shared/threads.h"
#include "../shared/FileApi/file.h"
#include "game.h"

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

#define GAME_ASSERT(cond, msg, ...)														\
	do {																					\
		if (!(cond)) {																		\
			printf("Thread Assertion failed at file %s line %d: \n", __FILE__, __LINE__);	\
			printf(msg, __VA_ARGS__);														\
			return false;																	\
		}																					\
	} while (0);

#define LOG_PRINTF(file, msg, ...)															\
	do {																					\
		printf(msg, __VA_ARGS__);															\
		if (file != NULL)																	\
			File_Printf(file, msg, __VA_ARGS__);											\
	} while (0);

#define SERVER_IP								"127.0.0.1"

/************************************
*       types                       *
************************************/

/************************************
*      variables                    *
************************************/
static uint16_t g_port;
s_client_data g_client_data_array[NUMBER_OF_ACTIVE_CONNECTIONS];
HANDLE g_handles[NUMBER_OF_ACTIVE_CONNECTIONS] = { NULL, NULL };
static uint8_t g_start_game_barrier_counter;
static s_game_data g_game_data;

/************************************
*      static functions             *
************************************/
static void parse_arguments(int argc, char* argv[]);
static void server_init();
//
// routines
static DWORD WINAPI server_listen_routine(LPVOID lpParam);
static DWORD WINAPI server_exit_routine(LPVOID lpParam);
static DWORD WINAPI client_thread_routine(LPVOID lpParam);
//
static bool open_log_file(File * file, char* username);
static bool find_available_thread(HANDLE* handles, int8_t* thread_index);
static void decide_first_player(HANDLE* handles, char* player_name);
static void close_all(HANDLE* handles, HANDLE* server_handles, SOCKET server_socket, s_client_data* client_data);
static void print_socket_data(SOCKET socker_originator, char* print_originator, char* string);

/************************************
*       API implementation          *
************************************/
// TODO: CHECK LAST TODOs

/// Description: initiate values, initiate server socket, create server threads. 
/// Parameters: 
///		[in] argc - number of arguments. 
///		[in] argv - arguments list. 
/// Return: 0 if succeeded or 1 otherwise.
int main(int argc, char* argv[])
{
	int exit_code = 0;
	parse_arguments(argc, argv);
	server_init();

	SOCKET server_socket = Socket_Init(socket_server, SERVER_IP, g_port);
	ASSERT(server_socket != INVALID_SOCKET, "Error: server can't open socket\n");

	HANDLE server_handles[2];
	server_handles[0] = create_new_thread(server_listen_routine, &server_socket);
	server_handles[1] = create_new_thread(server_exit_routine, NULL);
	
	if (server_handles[0] == NULL || server_handles[1] == NULL)
	{
		printf("Error: failed creating a thread\n");
		exit_code = 1;
	}
	else if (!wait_for_threads(server_handles, 2, false, INFINITE, true))
	{
		printf("Error: faild waiting on threads\n");
		exit_code = 1;
	}

	// free all allocation & close handles (Threads, Mutex, Semaphores)
	close_all(g_handles, server_handles, server_socket, g_client_data_array);
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
	ASSERT(argc == 2, "Error: not enough arguments.\n");
	g_port = (uint16_t)strtol(argv[1], NULL, 10);
}

/// Description: initialize all relevant variables.
/// Parameters: none.
/// Return: none.
static void server_init() 
{
	g_start_game_barrier_counter = 0;

	g_game_data.mutex_game_update = create_mutex(true);
	ASSERT(g_game_data.mutex_game_update != NULL, "Error: failed creating mutex. Exiting\n");

	g_game_data.semaphore_game_routine = create_semaphore(0, 1);
	ASSERT(g_game_data.semaphore_game_routine != NULL, "Error: failed creating semaphore. Exiting\n");

	g_game_data.player_turn = 1;
	g_game_data.game_counter = 0;
	
	game_init(&g_game_data);
}

/// Description: server listens to accept new clients and creates new thread for each client.  
/// Parameters: 
///		[in] lpParam - parameters: server socket. 
/// Return: DWORD 0 if thread routine succeeded or 1 otherwise.
static DWORD WINAPI server_listen_routine(LPVOID lpParam)
{
	SOCKET server_socket = *((SOCKET*)lpParam);

	while (1)
	{
		// accept new clients (wait for clients to connect)
		SOCKET accept_socket = accept(server_socket, NULL, NULL);
		GAME_ASSERT(accept_socket != INVALID_SOCKET, "Error: server can't open socket for client\n");
		// TODO: remove
		printf("Client has connected\n");

		// search for unused thread slot
		int8_t thread_index;
		if (!find_available_thread(g_handles, &thread_index))
		{
			// failed to wait for object
			printf("Error: WaitForSingleObject has failed (%d)\n", GetLastError());
			Socket_TearDown(accept_socket, true);
			break;
		}

		// send access denied to client
		if (thread_index == -1)
		{
			// reject client
			s_message_params message_params = { .message_type = MESSAGE_TYPE_SERVER_DENIED, .params_count = 0 };
			Socket_Send(accept_socket, message_params);
			Socket_TearDown(accept_socket, true);
			continue;
		}

		// receive new client username
		s_message_params received_message_params;
		if (check_received_message(accept_socket, MESSAGE_TYPE_CLIENT_REQUEST, &received_message_params, CLIENT_NON_USER_TIMEOUT) != transfer_succeeded)
		{
			Socket_TearDown(accept_socket, true);
			Socket_FreeParamsArray(received_message_params.params, received_message_params.params_count);
			continue;
		}

		// open log file
		if (!open_log_file(&(g_client_data_array[thread_index].client_thread_file), received_message_params.params[0]))
		{
			// failed to open file
			printf("Error: open file has failed\n");
			Socket_TearDown(accept_socket, true);
			Socket_FreeParamsArray(received_message_params.params, received_message_params.params_count);
			break;
		}

		Socket_BindSocketPrintCallback(print_socket_data);

		// approve new client
		s_message_params message_params = { .message_type = MESSAGE_TYPE_SERVER_APPROVED, .params_count = 0 };
		Socket_Send(accept_socket, message_params);

		decide_first_player(g_handles, received_message_params.params[0]);

		// set threads
		g_client_data_array[thread_index].client_socket = accept_socket;
		strcpy(g_client_data_array[thread_index].username, received_message_params.params[0]);
		g_handles[thread_index] = create_new_thread(client_thread_routine, &g_client_data_array[thread_index]);
		if (g_handles[thread_index] == NULL)
		{
			LOG_PRINTF(g_client_data_array[thread_index].client_thread_file, "Error: failed creating a thread\n");
			Socket_FreeParamsArray(received_message_params.params, received_message_params.params_count);
			break;
		}

		// free params
		Socket_FreeParamsArray(received_message_params.params, received_message_params.params_count);
	}

	return 0;
}

/// Description: check if user wrote "exit" on server console.  
/// Parameters: none.
/// Return: DWORD 0 if thread routine succeeded or 1 otherwise.
static DWORD WINAPI server_exit_routine(LPVOID lpParam)
{
	char server_console_buffer[50];
	do
	{
		scanf("%s", &server_console_buffer);
	}
	while (strcmp(server_console_buffer, "exit"));
	return 0;
}

/// Description: handle game events - message transfer between server and clients, client disconnect event.
/// Parameters: 
///		[in] lpParam - client_data: client's username and socket.
/// Return: DWORD 0 if thread routine succeeded or 1 otherwise.
static DWORD WINAPI client_thread_routine(LPVOID lpParam)
{
	// parsing inputs
	s_client_data* client_data = (s_client_data*)lpParam;
	s_message_params received_message_params = { .message_type = MESSAGE_TYPE_UNKNOWN };

	while (true)
	{
		// send main menu
		s_message_params message_params = { .message_type = MESSAGE_TYPE_SERVER_MAIN_MENU, .params_count = 0 };
		Socket_Send(client_data->client_socket, message_params);
		
		// wait for CLIENT_VERSUS
		if (check_received_message(client_data->client_socket, MESSAGE_TYPE_CLIENT_VERSUS, &received_message_params, INFINITE) != transfer_succeeded)
		{
			// client disconnect from server
			LOG_PRINTF(client_data->client_thread_file, "Player disconnected. Exiting.\n");
			break;
		}

		// wait for another player to connect
		if (!game_barrier(&g_start_game_barrier_counter, &client_data->client_thread_file))
		{
			// send SERVER_NO_OPPONENTS
			message_params.message_type = MESSAGE_TYPE_SERVER_NO_OPPONENTS;
			Socket_Send(client_data->client_socket, message_params);
			g_start_game_barrier_counter = 0;
			Socket_FreeParamsArray(received_message_params.params, received_message_params.params_count);
			continue;
		}

		// send MESSAGE_TYPE_GAME_STARTED
		message_params.message_type = MESSAGE_TYPE_GAME_STARTED;
		Socket_Send(client_data->client_socket, message_params);
		g_game_data.game_is_on = true;

		// game routine
		e_game_result game_routine_result = game_routine(client_data);
		if (game_routine_result != game_succeed)
		{
			if (ReleaseSemaphore(g_game_data.semaphore_game_routine, 1, NULL) == false)
			{
				LOG_PRINTF(client_data->client_thread_file, "Error: failed releasing semaphore. %d\n", GetLastError());
				break;
			}

			g_game_data.player_disconnected = true;
			g_game_data.game_is_on = false;
			
			// game unexpectedly stopped
			printf(game_routine_result == game_failed ? "Error: Game unexpectedly stopped\n" : "Error: Client has disconnected\n"); // TODO: REMOVE
			break;
		}

		// reset game parameters to start a new game 
		g_game_data.game_counter = 0;
		g_game_data.player_turn = 1;
		Socket_FreeParamsArray(received_message_params.params, received_message_params.params_count);
	}

	// free params and close socket if player disconnected/error occured
	Socket_FreeParamsArray(received_message_params.params, received_message_params.params_count);
	Socket_TearDown(client_data->client_socket, true);
	client_data->client_socket = INVALID_SOCKET;	// to prevent double close socket
	File_Close(client_data->client_thread_file);
	return 0;
}

static bool open_log_file(File* file, char* username)
{
	int filename_length = snprintf(NULL, 0, "Thread_log_%s.txt\n", username);
	char* file_name = malloc((filename_length) * sizeof(char));
	GAME_ASSERT(file_name != NULL, "Error: failed allocating buffer for message\n")

	snprintf(file_name, filename_length, "Thread_log_%s.txt\n", username);
	*file = File_Open(file_name, "w");
	free(file_name);
	GAME_ASSERT(*file != NULL, "Error: failed opening output file\n");

	return true;
}

/// Description: find available slot in thread handles array.
/// Parameters: 
///		[in] handles - array of thread handles.
///		[out] thread_index - free slot in handles array.
/// Return: true if available slot found and false otherwise.
static bool find_available_thread(HANDLE* handles, int8_t* thread_index)
{
	for (uint8_t i = 0; i < NUMBER_OF_ACTIVE_CONNECTIONS; i++)
	{
		if (handles[i] == NULL)
		{
			*thread_index = i;
			return true;
		}

		// polling on threads
		DWORD wait_code = WaitForSingleObject(handles[i], 0);
		if (wait_code == WAIT_OBJECT_0)
		{
			CloseHandle(handles[i]);
			*thread_index = i;
			handles[i] = NULL;
			return true;
		}

		else if (wait_code == WAIT_FAILED)
		{
			return false;
		}
	}

	*thread_index = -1;
	return true;
}

/// Description: set players' turn according to order of connection to server. 
/// Parameters: 
///		[in] handles - array of thread handles.
///		[in] player_name
/// Return: none.
static void decide_first_player(HANDLE* handles, char* player_name)
{
	// no user connected yet
	if (handles[0] == NULL && handles[1] == NULL)
		strcpy(g_game_data.first_player_name, player_name);
	
	// first user disconnected
	else if (handles[0] == NULL && handles[1] != NULL)
	{
		strcpy(g_game_data.first_player_name, g_game_data.second_player_name);
		strcpy(g_game_data.second_player_name, player_name);
	}
	
	// second user disconnected or second user first connection
	else
	{
		strcpy(g_game_data.second_player_name, player_name);
	}
}

/// Description: exit protocol - close mutexes, sockets and handles.  
/// Parameters: 
///		[in] handles - array of client thread handles.
///		[in] server_handles - array of server thread handles.
///		[in] server_socket
///		[in] client_data - client's username and socket.
/// Return: none.
static void close_all(HANDLE* handles, HANDLE* server_handles, SOCKET server_socket, s_client_data* client_data)
{
	game_tear_down();
	CloseHandle(g_game_data.mutex_game_routine);
	CloseHandle(g_game_data.mutex_game_update);

	for (int8_t i = 0; i < NUMBER_OF_ACTIVE_CONNECTIONS; i++)
	{
		if (client_data[i].client_socket != INVALID_SOCKET)
			Socket_TearDown(client_data[i].client_socket, true);
	}

	close_handles(handles, NUMBER_OF_ACTIVE_CONNECTIONS);
	close_handles(server_handles, 2);
	Socket_TearDown(server_socket, false);
}

/// Description: handle socket data for printing.
/// Parameters: 
///		[in] socker_originator - the send/recv socket.
///		[in] print_originator - send/receive string.
///		[in] string - the data from socket to print.
/// Return: none.
static void print_socket_data(SOCKET socker_originator, char* print_originator, char* string)
{
	for (int i = 0; i < NUMBER_OF_ACTIVE_CONNECTIONS; i++)
		if (g_client_data_array[i].client_socket == socker_originator)
		{
			if (g_client_data_array[i].client_thread_file != NULL)
				File_Printf(g_client_data_array[i].client_thread_file, "%s client-%s\n", print_originator, string);
		}
}