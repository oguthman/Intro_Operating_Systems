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

/************************************
*      definitions                 *
************************************/
#define USERNAME_MAX_LENGTH				20

/************************************
*       types                       *
************************************/
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
	HANDLE semaphore_game_routine;
} gs_game_data;

typedef struct {
	char username[USERNAME_MAX_LENGTH + 1];
	SOCKET client_socket;
} s_client_data;

/************************************
*       API                         *
************************************/
bool game_barrier(uint8_t* counter);
bool check_received_message(SOCKET client_socket, e_message_type expected_message_type, s_message_params* received_message_params, uint32_t timeout);
bool game_routine(s_client_data* client_data);

// return true if the game is still on, false if player lost in this turn.
bool game_logic(char* user_move);
void game_tear_down();

#endif //__GAME_H__