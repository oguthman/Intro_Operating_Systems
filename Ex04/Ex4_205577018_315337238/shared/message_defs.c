/*!
******************************************************************************
\file message_defs.c
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
#include "message_defs.h"
#include <stdint.h>
#include <string.h>

/************************************
*      definitions                 *
************************************/

/************************************
*       types                       *
************************************/

/************************************
*      variables                    *
************************************/
static char* message_str_array[] = {
	// client
	"CLIENT_REQUEST",
	"CLIENT_VERSUS",
	"CLIENT_PLAYER_MOVE",
	"CLIENT_DISCONNECT",
	// server
	"SERVER_APPROVED",
	"SERVER_DENIED",
	"SERVER_MAIN_MENU",
	"GAME_STARTED",
	"TURN_SWITCH",
	"SERVER_MOVE_REQUEST",
	"GAME_ENDED",
	"SERVER_NO_OPPONENTS",
	"GAME_VIEW",
	"SERVER_OPPONENT_QUIT",
	//
	"UNKNOWN"
};

/************************************
*      static functions             *
************************************/

/************************************
*       API implementation          *
************************************/
char* get_message_str(e_message_type message_type)
{
	return message_str_array[message_type];
}

e_message_type get_message_type(char* message_str)
{
	for (uint16_t i = 0; i <= MESSAGE_TYPE_UNKNOWN; i++)
	{
		if (!strcmp(message_str, message_str_array[i]))
			return i;
	}

	return MESSAGE_TYPE_UNKNOWN;
}


/************************************
* static implementation             *
************************************/

