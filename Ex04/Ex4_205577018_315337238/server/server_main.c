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
			return false;																		\
		}																					\
	} while (0);

#define SERVER_IP								"127.0.0.1"
#define NUMBER_OF_ACTIVE_CONNECTIONS			2
#define WAIT_FOR_CLIENT_OPERATION_TIMEOUT		INFINITE
#define WAIT_FOR_OPPENET_TIMEOUT				(15000)		// 15sec //TODO: CHANGE ACCORDING TO INSTRUCTIONS

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
static DWORD WINAPI server_listen_routine(LPVOID lpParam);
static DWORD WINAPI server_exit_routine(LPVOID lpParam);
static DWORD WINAPI client_thread_routine(LPVOID lpParam);
static bool find_available_thread(HANDLE* handles, int8_t* thread_index);
static void decide_first_player(HANDLE* handles, char* player_name);
static void close_all(HANDLE* handles, SOCKET server_socket, s_client_data* client_data, HANDLE* server_handles);

/************************************
*       API implementation          *
************************************/
// TODO: EXIT COMMAND TO EXIT SERVER
// TODO: MAKE SURE TO CHECK EVERY CREATE NEW THREAD

int main(int argc, char* argv[])
{
	parse_arguments(argc, argv);
	server_init();
	game_init(&g_game_data);

	SOCKET server_socket = Socket_Init(socket_server, SERVER_IP, g_port);
	ASSERT(server_socket != INVALID_SOCKET, "Error: server can't open socket\n");		//TODO: exit?

	HANDLE server_handles[2];
	server_handles[0] = create_new_thread(server_listen_routine, &server_socket);
	THREAD_ASSERT(server_handles[0] != NULL, "Error: failed creating a thread\n");

	server_handles[1] = create_new_thread(server_exit_routine, NULL);
	THREAD_ASSERT(server_handles[1] != NULL, "Error: failed creating a thread\n");

	if (!wait_for_threads(server_handles, 2, false, INFINITE, true))
	{
		printf("Error: faild waiting on threads\n");
	}
	//TODO: GRACEFULL SHUTDOWN
	// free all allocation & close handles (Threads, Mutex, Semaphores & Events)
	close_all(g_handles, server_socket, g_client_data_array, server_handles);
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

static void server_init() 
{
	g_start_game_barrier_counter = 0;

	g_game_data.mutex_game_update = create_mutex(true);
	ASSERT(g_game_data.mutex_game_update != NULL, "Error: failed creating mutex. Exiting\n");

	g_game_data.semaphore_game_routine = create_semaphore(0, 1);
	ASSERT(g_game_data.semaphore_game_routine != NULL, "Error: failed creating semaphore. Exiting\n");

	g_game_data.player_turn = 1;
	g_game_data.game_counter = 0;
}

static DWORD WINAPI server_listen_routine(LPVOID lpParam)
{
	SOCKET server_socket = *((SOCKET*)lpParam);

	while (1)
	{
		// accept new clients (wait for clients to connect)
		SOCKET accept_socket = accept(server_socket, NULL, NULL);
		ASSERT(accept_socket != INVALID_SOCKET, "Error: server can't open socket for client\n");		//TODO: exit?
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

		if (thread_index == -1)
		{
			// TODO: remove
			printf("Rejecting client\n");
			// send access denied to client
			s_message_params message_params = { .message_type = MESSAGE_TYPE_SERVER_DENIED, .params_count = 0 };
			Socket_Send(accept_socket, message_params);
			// reject client
			Socket_TearDown(accept_socket, true);
			continue;
		}

		// receive new client username
		s_message_params received_message_params;
		if (!check_received_message(accept_socket, MESSAGE_TYPE_CLIENT_REQUEST, &received_message_params, WAIT_FOR_OPPENET_TIMEOUT))
		{
			// TODO: gracefull exit
			// reject client
			Socket_TearDown(accept_socket, true);
			Socket_FreeParamsArray(received_message_params.params, received_message_params.params_count);
			continue;
		}

		// approve new client
		s_message_params message_params = { .message_type = MESSAGE_TYPE_SERVER_APPROVED, .params_count = 0 };
		Socket_Send(accept_socket, message_params);

		decide_first_player(g_handles, received_message_params.params[0]);

		// set threads
		g_client_data_array[thread_index].client_socket = accept_socket;
		strcpy(g_client_data_array[thread_index].username, received_message_params.params[0]);
		g_handles[thread_index] = create_new_thread(client_thread_routine, &g_client_data_array[thread_index]);
		THREAD_ASSERT(g_handles[thread_index] != NULL, "Error: failed creating a thread\n");

		// free params
		Socket_FreeParamsArray(received_message_params.params, received_message_params.params_count);
	}

	return 0;
}

// exit server when written "exit" in server console
// return 0 - exit occured
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
		if (!check_received_message(client_data->client_socket, MESSAGE_TYPE_CLIENT_VERSUS, &received_message_params, WAIT_FOR_CLIENT_OPERATION_TIMEOUT)) // TODO: Fix timeout
		{
			// client chose to disconnect from server
			// TODO: REMOVE
			printf("client chose to disconnect from server '[%d] %s'\n", received_message_params.message_type, get_message_str(received_message_params.message_type));
			break;
		}

		// wait for another player to connect
		if (!game_barrier(&g_start_game_barrier_counter))
		{
			// send SERVER_NO_OPPONENTS
			message_params.message_type = MESSAGE_TYPE_SERVER_NO_OPPONENTS;
			Socket_Send(client_data->client_socket, message_params);
			continue;
		}

		// send MESSAGE_TYPE_GAME_STARTED
		message_params.message_type = MESSAGE_TYPE_GAME_STARTED;
		Socket_Send(client_data->client_socket, message_params);
		g_game_data.game_is_on = true;

		// game routine
		if (!game_routine(client_data))
		{
			// game unexpectedly stopped
			// TODO: REMOVE
			printf("Error: Game unexpectedly stopped\n");
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

	return 0;
}

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

static void close_all(HANDLE* handles, SOCKET server_socket, s_client_data* client_data, HANDLE* server_handles)
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

