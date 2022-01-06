/*!
******************************************************************************
\file game.c
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
#include "game.h"

/************************************
*      definitions                 *
************************************/
#define NUMBER_OF_ACTIVE_CONNECTIONS			2
#define WAIT_FOR_OPPENET_TIMEOUT				(15000)		// 15sec //TODO: CHANGE ACCORDING TO INSTRUCTIONS

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

/************************************
*       types                       *
************************************/

/************************************
*      variables                    *
************************************/
static HANDLE g_barrier_mutex; //SIGNALED
static HANDLE g_barrier_semaphore;
static uint8_t g_game_barrier_counter;

static s_game_data* g_game_data;

/************************************
*      static functions             *
************************************/
static bool find_seven();

/************************************
*       API implementation          *
************************************/
void game_init(s_game_data* game_data)
{
	g_barrier_mutex = create_mutex(true);
	ASSERT(g_barrier_mutex != NULL, "Error: failed creating mutex. Exiting\n");

	g_barrier_semaphore = create_semaphore(0, NUMBER_OF_ACTIVE_CONNECTIONS);
	ASSERT(g_barrier_semaphore != NULL, "Error: failed creating semaphore. Exiting\n");

	g_game_barrier_counter = 0;
	g_game_data = game_data;
}

bool game_barrier(uint8_t* counter)
{
	DWORD wait_code = WaitForSingleObject(g_barrier_mutex, WAIT_FOR_OPPENET_TIMEOUT);
	if (wait_code == WAIT_TIMEOUT)
		return false;
	else if (wait_code != WAIT_OBJECT_0)
	{
		printf("Error waiting for barrier mutex (%d)\n", wait_code);
		return false;
	}

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

e_game_result check_received_message(SOCKET client_socket, e_message_type expected_message_type, s_message_params* received_message_params, uint32_t timeout)
{
	e_transfer_result result = Socket_Receive(client_socket, received_message_params, timeout);
	if (result == transfer_succeeded && received_message_params->message_type == expected_message_type)
		return game_succeed;
	if (result == transfer_timeout)
		return game_client_timeout;
	if (result == transfer_disconnected)
		return game_client_disconnected;

	return game_failed;
}

e_game_result game_routine(s_client_data* client_data)
{
	uint8_t game_barrier_counter = 0;
	while (g_game_data->game_is_on)
	{
		s_message_params received_message_params = { .message_type = MESSAGE_TYPE_UNKNOWN };
		// send TURN_SWITCH
		s_message_params send_message_params = { .message_type = MESSAGE_TYPE_TURN_SWITCH, .params_count = 1 };
		send_message_params.params[0] = g_game_data->player_turn == 1 ? g_game_data->first_player_name : g_game_data->second_player_name;
		Socket_Send(client_data->client_socket, send_message_params);

		// my turn
		if (!strcmp(client_data->username, send_message_params.params[0]))
		{
			// send SERVER_MOVE_REQUEST
			send_message_params.message_type = MESSAGE_TYPE_SERVER_MOVE_REQUEST;
			send_message_params.params_count = 0;
			Socket_Send(client_data->client_socket, send_message_params);

			// wait for response CLIENT_PLAYER_MOVE
			e_game_result received_result = check_received_message(client_data->client_socket, MESSAGE_TYPE_CLIENT_PLAYER_MOVE, &received_message_params, WAIT_FOR_OPPENET_TIMEOUT); // TODO: Fix timeout
			if (received_result != game_succeed)
			{
				// message didn't match to the expected
				if (received_result == game_failed)
					printf("Response message didn't match to the expected '[%d] %s'\n", received_message_params.message_type, get_message_str(received_message_params.message_type));
				Socket_FreeParamsArray(received_message_params.params, received_message_params.params_count);
				return received_result;
			}

			// check player's move
			if (received_message_params.params != NULL)
				g_game_data->player_move = received_message_params.params[0];
			g_game_data->game_is_on = game_logic(g_game_data->player_move);

			// set other player name as the winner
			if (!g_game_data->game_is_on)
				g_game_data->winner = g_game_data->player_turn == 1 ? g_game_data->second_player_name : g_game_data->first_player_name;

			// release semaphore
			THREAD_ASSERT(ReleaseSemaphore(g_game_data->semaphore_game_routine, 1, NULL) == true, "Error: failed releasing semaphore. %d\n", GetLastError());
		}
		else // other player turn
		{
			// wait for player move
			DWORD wait_code = WaitForSingleObject(g_game_data->semaphore_game_routine, WAIT_FOR_OPPENET_TIMEOUT);
			if (wait_code != WAIT_OBJECT_0)
				return game_failed;

			send_message_params.message_type = MESSAGE_TYPE_GAME_VIEW;
			send_message_params.params_count = 3;
			send_message_params.params[1] = g_game_data->player_move;
			send_message_params.params[2] = g_game_data->game_is_on ? "CONT" : "END";

			// send GAME_VIEW
			Socket_Send(client_data->client_socket, send_message_params);

			// switch turnss
			g_game_data->player_turn = g_game_data->player_turn == 1 ? 2 : 1;
		}

		// game ended
		if (!g_game_data->game_is_on)
		{
			send_message_params.message_type = MESSAGE_TYPE_GAME_ENDED;
			send_message_params.params_count = 1;
			send_message_params.params[0] = g_game_data->winner;
			// send GAME_ENDED to winner (other player)
			Socket_Send(client_data->client_socket, send_message_params);
			break;
		}

		// make sure to switch turn only when both players have played
		game_barrier(&g_game_barrier_counter);

		// free the params
		Socket_FreeParamsArray(received_message_params.params, received_message_params.params_count);
	}

	return game_succeed;
}

bool game_logic(char* user_move)
{
	bool boom = false;
	bool seven = false;

	// update game counter 
	g_game_data->game_counter++;

	if ((g_game_data->game_counter % 7) == 0 || find_seven())
	{
		boom = true;
	}

	if (boom)
	{
		if (!strcmp("boom", user_move)) return true;
		return false;
	}
	else
	{
		uint32_t number = strtol(user_move, NULL, 10);
		if (number == g_game_data->game_counter) return true;
	}
	return false;
}

void game_tear_down()
{
	CloseHandle(g_barrier_mutex);
	CloseHandle(g_barrier_semaphore);
}

/************************************
* static implementation             *
************************************/
/// Description: check if user's game move contains '7'.
/// Parameters: none.
/// Return: true if '7' is found in user's game move and false otherwise.
static bool find_seven()
{
	int message_length = snprintf(NULL, 0, "%d", g_game_data->game_counter);
	char* buffer = malloc((message_length + 1) * sizeof(char));
	if (NULL == buffer)
	{
		printf("Error: failed allocating buffer for message\n");
		return false;
	}
	snprintf(buffer, message_length + 1, "%d", g_game_data->game_counter);

	for (uint32_t i = 0; i < strlen(buffer); i++)
	{
		if (buffer[i] == '7')
		{
			free(buffer);
			return true;
		}
	}
	free(buffer);
	return false;
}