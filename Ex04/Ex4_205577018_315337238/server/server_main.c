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

#define SERVER_IP						"127.0.0.1"
#define NUMBER_OF_ACTIVE_CONNECTIONS	2
#define USERNAME_MAX_LENGTH				20
#define WAIT_FOR_OPPENET_TIMEOUT		15000

/************************************
*       types                       *
************************************/
typedef struct {
	e_message_type message_type;
	uint32_t params_count;
	char** params;
} s_message_params;

typedef struct {
	char username[USERNAME_MAX_LENGTH+1];
	SOCKET client_socket;
} s_client_data;

/************************************
*      variables                    *
************************************/
static uint16_t g_port;
static HANDLE g_barrier_mutex;//SIGNALED
static HANDLE g_barrier_semaphore;
static uint8_t g_start_game_barrier_counter;
static uint8_t g_game_barrier_counter;

static struct {
	bool game_is_on;
	uint32_t game_counter;
	char first_player_name[USERNAME_MAX_LENGTH + 1];
	char second_player_name[USERNAME_MAX_LENGTH + 1];
	int8_t player_turn;
	char* player_move;
	char* winner;
	HANDLE mutex_game_update;
	HANDLE mutex_game_routine;
} gs_game_data;

/************************************
*      static functions             *
************************************/
static void parse_arguments(int argc, char* argv[]);
static void server_init();
static DWORD WINAPI client_thread_routine(LPVOID lpParam);
static bool check_received_message(SOCKET client_socket, e_message_type expected_message_type, s_message_params* received_message_params, uint32_t timeout);
static bool game_barrier(uint8_t* counter);
static bool update_game_data(s_client_data* client_data);
static bool game_routine(s_client_data* client_data);
static bool game_logic(char* user_move);
static bool find_available_thread(HANDLE* handles, int8_t* thread_index);
static void decide_first_player(HANDLE* handles, char* player_name);
static void close_all(HANDLE* handles, SOCKET server_socket, s_client_data* client_data);

/************************************
*       API implementation          *
************************************/
int main(int argc, char* argv[])
{
	parse_arguments(argc, argv);
	server_init();

	SOCKET server_socket = Socket_Init(socket_server, SERVER_IP, g_port);
	ASSERT(server_socket != INVALID_SOCKET, "Error: server can't open socket\n");		//TODO: exit?

	// accept or decline. create new thread for each client. TODO: WHILE LOOP on number_of_clients_connected
	s_client_data client_data_array[NUMBER_OF_ACTIVE_CONNECTIONS];
	HANDLE handles[NUMBER_OF_ACTIVE_CONNECTIONS] = { NULL, NULL };
	
	while (1)
	{
		// accept new clients (wait for clients to connect)
		SOCKET accept_socket = accept(server_socket, NULL, NULL);
		ASSERT(accept_socket != INVALID_SOCKET, "Error: server can't open socket for client\n");		//TODO: exit?
		// TODO: remove
		printf("Client has connected\n");

		// search for unused thread slot
		int8_t thread_index;
		if (!find_available_thread(handles, &thread_index))
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
			Socket_Send(accept_socket, MESSAGE_TYPE_SERVER_DENIED, 0, NULL);
			// reject client
			Socket_TearDown(accept_socket, true);
			continue;
		}
				
		// receive new client username
		s_message_params received_message_params;
		if (!check_received_message(accept_socket, MESSAGE_TYPE_CLIENT_REQUEST, &received_message_params, 0)) // TODO: Add timeout
		{
			// TODO: REMOVE
			printf("Server recived from client '[%d] %s'", received_message_params.message_type, get_message_str(received_message_params.message_type));
			// TODO: gracefull exit
			// reject client
			Socket_TearDown(accept_socket, true);
			Socket_FreeParamsArray(received_message_params.params, received_message_params.params_count);
			continue;
		}

		// approve new client
		Socket_Send(accept_socket, MESSAGE_TYPE_SERVER_APPROVED, 0, NULL);
		
		decide_first_player(handles, received_message_params.params[0]);

		// set threads
		client_data_array[thread_index].client_socket = accept_socket;
		strcpy(client_data_array[thread_index].username, received_message_params.params[0]);
		handles[thread_index] = create_new_thread(client_thread_routine, &client_data_array[thread_index]);
		THREAD_ASSERT(handles[thread_index] != NULL, "Error: failed creating a thread\n");

		// free params
		Socket_FreeParamsArray(received_message_params.params, received_message_params.params_count);
	}

	// free all allocation & close handles (Threads, Mutex, Semaphores & Events)
	close_all(handles, server_socket, client_data_array);
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
	g_barrier_mutex = create_mutex(true);
	ASSERT(g_barrier_mutex != NULL, "Error: failed creating mutex. Exiting\n");

	g_barrier_semaphore = create_semaphore(0, NUMBER_OF_ACTIVE_CONNECTIONS);
	ASSERT(g_barrier_semaphore != NULL, "Error: failed creating semaphore. Exiting\n");

	g_start_game_barrier_counter = 0;
	g_game_barrier_counter = 0;

	gs_game_data.mutex_game_update = create_mutex(true);
	ASSERT(gs_game_data.mutex_game_update != NULL, "Error: failed creating mutex. Exiting\n");

	gs_game_data.mutex_game_routine = create_mutex(false);
	ASSERT(gs_game_data.mutex_game_routine != NULL, "Error: failed creating mutex. Exiting\n");

	gs_game_data.player_turn = -1;
	gs_game_data.game_counter = 0;
}

static DWORD WINAPI client_thread_routine(LPVOID lpParam)
{
	// parsing inputs
	s_client_data* client_data = (s_client_data*)lpParam;
	s_message_params received_message_params = { .params = NULL, .params_count = 0 };

	while (true)
	{
		// send main menu
		Socket_Send(client_data->client_socket, MESSAGE_TYPE_SERVER_MAIN_MENU, 0, NULL);
		
		// wait for CLIENT_VERSUS
		if (!check_received_message(client_data->client_socket, MESSAGE_TYPE_CLIENT_VERSUS, &received_message_params, -1)) // TODO: Fix timeout
		{
			// client chose to disconnect from server
			// TODO: REMOVE
			printf("client chose to disconnect from server '[%d] %s'\n", received_message_params.message_type, get_message_str(received_message_params.message_type));
			break;
		}

		// update game data
		if (!update_game_data(client_data))
		{
			// failed handle mutex
			// TODO: REMOVE
			printf("Error: failed handle mutex\n");
			break;
		}

		// wait for another player to connect
		if (!game_barrier(&g_start_game_barrier_counter))
		{
			// send SERVER_NO_OPPONENTS
			Socket_Send(client_data->client_socket, MESSAGE_TYPE_SERVER_NO_OPPONENTS, 0, NULL);

			// TODO: REMOVE
			printf("SERVER_NO_OPPONENTS\n");
			continue;
		}

		// send MESSAGE_TYPE_GAME_STARTED
		Socket_Send(client_data->client_socket, MESSAGE_TYPE_GAME_STARTED, 0, NULL);
		gs_game_data.game_is_on = true;

		// game routine
		if (!game_routine(client_data))
		{
			// game unexpectedly stopped
			// TODO: REMOVE
			printf("Error: Game unexpectedly stopped\n");
			break;
		}

		// reset game parameters to start a new game 
		gs_game_data.game_counter = 0;
		gs_game_data.player_turn = -1;
		Socket_FreeParamsArray(received_message_params.params, received_message_params.params_count);
		received_message_params.params = NULL;	// to be on the safe side
	}

	// free params and close socket if player disconnected/error occured
	Socket_FreeParamsArray(received_message_params.params, received_message_params.params_count);
	Socket_TearDown(client_data->client_socket, true);
	client_data->client_socket = INVALID_SOCKET;	// to prevent double close socket

	return 0;
}

static bool check_received_message(SOCKET client_socket, e_message_type expected_message_type, s_message_params* received_message_params, uint32_t timeout)
{
	Socket_Receive(client_socket, &(received_message_params->message_type), received_message_params->params, &(received_message_params->params_count), timeout);
	return received_message_params->message_type == expected_message_type;
}

static bool game_barrier(uint8_t* counter)
{
	DWORD wait_code = WaitForSingleObject(g_barrier_mutex, WAIT_FOR_OPPENET_TIMEOUT);
	if (wait_code != WAIT_OBJECT_0)
		return false;

	// critical area
	(*counter)++;
	if (*counter == NUMBER_OF_ACTIVE_CONNECTIONS)
	{		
		THREAD_ASSERT(ReleaseSemaphore(g_barrier_semaphore, NUMBER_OF_ACTIVE_CONNECTIONS, NULL) == true, "Error: failed releasing semaphore\n");
		*counter = 0;
	}
	// end of critical area
	THREAD_ASSERT(ReleaseMutex(g_barrier_mutex) == true, "Error: failed releasing mutex\n");

	wait_code = WaitForSingleObject(g_barrier_semaphore, WAIT_FOR_OPPENET_TIMEOUT);
	if (wait_code != WAIT_OBJECT_0)
		return false;
	
	return true;
}

static bool game_routine(s_client_data* client_data)
{
	uint8_t game_barrier_counter = 0;
	while (gs_game_data.game_is_on)
	{
		s_message_params received_message_params = { .params = NULL, .params_count = 0 };
		// send TURN_SWITCH
		char* send_message_params[3];
		send_message_params[0] = gs_game_data.player_turn == 1 ? gs_game_data.first_player_name : gs_game_data.second_player_name;
		Socket_Send(client_data->client_socket, MESSAGE_TYPE_TURN_SWITCH, 1, send_message_params);
		// TODO: REMOVE
		printf("this turn belongs to %s\n", send_message_params[0]);

		// if this is my turn
		if (!strcmp(client_data->username, send_message_params[0]))
		{
			// send SERVER_MOVE_REQUEST
			Socket_Send(client_data->client_socket, MESSAGE_TYPE_SERVER_MOVE_REQUEST, 0, NULL);

			// waiting for response CLIENT_PLAYER_MOVE
			if (!check_received_message(client_data->client_socket, MESSAGE_TYPE_CLIENT_PLAYER_MOVE, &received_message_params, WAIT_FOR_OPPENET_TIMEOUT)) // TODO: Fix timeout
			{
				// message didn't match to the expected
				// TODO: REMOVE
				printf("Response message didn't match to the expected '[%d] %s'\n", received_message_params.message_type, get_message_str(received_message_params.message_type));
				// TODO: free the params
				return false;
			}

			// check player's move
			if (received_message_params.params != NULL)
				gs_game_data.player_move = received_message_params.params[0];
			gs_game_data.game_is_on = game_logic(gs_game_data.player_move);
			
			// set other player name as the winner
			if (!gs_game_data.game_is_on)
				gs_game_data.winner = gs_game_data.player_turn == 1 ? gs_game_data.second_player_name : gs_game_data.first_player_name;

			// release mutex
			THREAD_ASSERT(ReleaseMutex(gs_game_data.mutex_game_routine) == true, "Error: failed releasing mutex\n");
		}
		else // other player turn
		{
			// wait for player move
			DWORD wait_code = WaitForSingleObject(gs_game_data.mutex_game_routine, WAIT_FOR_OPPENET_TIMEOUT);
			if (wait_code != WAIT_OBJECT_0)
				return false;

			send_message_params[1] = gs_game_data.player_move;
			send_message_params[2] = gs_game_data.game_is_on ? "CONT" : "END";

			// send GAME_VIEW
			Socket_Send(client_data->client_socket, MESSAGE_TYPE_GAME_VIEW, 3, send_message_params);

			// switch turns
			gs_game_data.player_turn = gs_game_data.player_turn == 1 ? 2 : 1;
		}

		// game ended
		if (gs_game_data.game_is_on)
		{
			send_message_params[0] = gs_game_data.winner;
			// send GAME_ENDED to winner (other player)
			Socket_Send(client_data->client_socket, MESSAGE_TYPE_GAME_ENDED, 1, send_message_params);
			break;
		}

		// make sure to switch turn of user only when both players have played
		game_barrier(&g_game_barrier_counter);
		
		// free the params
		Socket_FreeParamsArray(received_message_params.params, received_message_params.params_count);
	}

	return true;
}

static bool update_game_data(s_client_data* client_data)
{
	DWORD wait_code = WaitForSingleObject(gs_game_data.mutex_game_update, WAIT_FOR_OPPENET_TIMEOUT);
	if (wait_code != WAIT_OBJECT_0)
		return false;

	if (gs_game_data.player_turn == -1)
	{
		gs_game_data.player_turn = 1;
		strcpy(gs_game_data.first_player_name, client_data->username);
	}
	else 
		strcpy(gs_game_data.second_player_name, client_data->username);
	
	THREAD_ASSERT(ReleaseMutex(gs_game_data.mutex_game_update) == true, "Error: failed releasing mutex\n");
	return true;
}

// return true if the game is still on, false if player lost in this turn.
static bool game_logic(char* user_move)
{
	uint32_t units;
	bool boom = false;

	// update game counter 
	gs_game_data.game_counter++;
	units = gs_game_data.game_counter % 10;

	if ((gs_game_data.game_counter % 7) == 0 || (units % 7) == 0)
	{
		boom = true;
	}

	// TODO: maybe need to check valid user input.
	
	if (boom)
	{
		if (strcmp("boom", user_move)) return true;
		return false;
	}
	else
	{
		uint32_t number = strtol(user_move, NULL, 10);
		if (number == gs_game_data.game_counter) return true;
	}
	return false;
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
		strcpy(gs_game_data.first_player_name, player_name);
	// first user disconnected
	else if (handles[0] == NULL && handles[1] != NULL)
	{
		strcpy(gs_game_data.first_player_name, gs_game_data.second_player_name);
		strcpy(gs_game_data.second_player_name, player_name);
	}
	// second user disconnected or second user first connection
	else
	{
		strcpy(gs_game_data.second_player_name, player_name);
	}
}

static void close_all(HANDLE* handles, SOCKET server_socket, s_client_data* client_data)
{
	CloseHandle(g_barrier_mutex);
	CloseHandle(g_barrier_semaphore);
	CloseHandle(gs_game_data.mutex_game_routine);
	CloseHandle(gs_game_data.mutex_game_update);

	for (int8_t i = 0; i < NUMBER_OF_ACTIVE_CONNECTIONS; i++)
	{
		if (client_data[i].client_socket != INVALID_SOCKET)
			Socket_TearDown(client_data[i].client_socket, true);
	}

	close_handles(handles, NUMBER_OF_ACTIVE_CONNECTIONS);
	Socket_TearDown(server_socket, false);
}

