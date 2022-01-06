/*!
******************************************************************************
\file game.h
\date 20 December 2021
\author Shahar Dorit Morag & Ofir Guthman
\project #4
\brief 

\details

\par Copyright
(c) Copyright 2021 Ofir & Shahar
\par
ALL RIGHTS RESERVED
*****************************************************************************/

#ifndef __GAME_H__
#define __GAME_H__

/************************************
*      include                      *
************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "../shared/threads.h"
#include "../shared/socket_handle.h"
#include "../shared/FileApi/file.h"

/************************************
*      definitions                 *
************************************/
#define USERNAME_MAX_LENGTH		20

/************************************
*       types                       *
************************************/
typedef struct {
	bool game_is_on;
	uint32_t game_counter;
	char first_player_name[USERNAME_MAX_LENGTH + 1];
	char second_player_name[USERNAME_MAX_LENGTH + 1];
	int8_t player_turn;
	char* player_move;
	char* winner;
	HANDLE mutex_game_update;
	HANDLE mutex_game_routine;
	HANDLE semaphore_game_routine;
	bool player_disconnected;
} s_game_data;

typedef struct {
	char username[USERNAME_MAX_LENGTH + 1];
	SOCKET client_socket;
	File client_thread_file;
} s_client_data;

typedef enum {
	game_failed,
	game_succeed,
	game_client_disconnected,
	game_client_timeout
} e_game_result;

/************************************
*       API                         *
************************************/
/// Description: initialize all relevant variables.
/// Parameters: 
///		[in] game_data - struct of the game data. 
/// Return: none.
void game_init(s_game_data* game_data);

/// Description: wait on number of threads to complete an action.
/// Parameters: 
///		[in] counter - counts how many threads finished the action. 
/// Return: true if succeeded and false otherwise.
bool game_barrier(uint8_t* counter);

/// Description: check if received the expected message.
/// Parameters: 
///		[in] client_socket
///		[in] expected_message_type - expected client message 
///		[out] received_message_params
///		[in] timeout - socket receive timeout. 
/// Return: true if transfer was successful and the receive message was the expected message, and false otherwise.
e_transfer_result check_received_message(SOCKET client_socket, e_message_type expected_message_type, s_message_params* received_message_params, uint32_t timeout);

/// Description: game routine - message transfer between server and clients while game is on.
/// Parameters: 
///		[in] client_data - struct that contains client's username and socket.
/// Return: true if no errors occured and false otherwise.
e_game_result game_routine(s_client_data* client_data);

/// Description: check user's game move according to the rules of "seven boom".
/// Parameters: 
///		[in] user_move - client's game move.
/// Return: true if the game is still on, false if player lost in this turn.
bool game_logic(char* user_move);

/// Description: exit protocol - close mutex and semaphore.  
/// Parameters: none.
/// Return: none.
void game_tear_down();

#endif //__GAME_H__