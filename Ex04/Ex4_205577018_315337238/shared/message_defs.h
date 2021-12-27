/*!
******************************************************************************
\file message_defs.h
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

#ifndef __MESSAGE_DEFS_H__
#define __MESSAGE_DEFS_H__

/************************************
*      include                      *
************************************/

/************************************
*      definitions                 *
************************************/

/************************************
*       types                       *
************************************/
typedef enum {
	MESSAGE_TYPE_CLIENT_REQUEST,
	MESSAGE_TYPE_CLIENT_VERSUS,
	MESSAGE_TYPE_CLIENT_PLAYER_MOVE,
	MESSAGE_TYPE_CLIENT_DISCONNECT,
	//
	MESSAGE_TYPE_SERVER_APPROVED,
	MESSAGE_TYPE_SERVER_DENIED,
	MESSAGE_TYPE_SERVER_MAIN_MENU,
	MESSAGE_TYPE_GAME_STARTED,
	MESSAGE_TYPE_TURN_SWITCH,
	MESSAGE_TYPE_SERVER_MOVE_REQUEST,
	MESSAGE_TYPE_GAME_ENDED,
	MESSAGE_TYPE_SERVER_NO_OPPONENTS,
	MESSAGE_TYPE_GAME_VIEW,
	MESSAGE_TYPE_SERVER_OPPONENT_QUIT,
	//
	MESSAGE_TYPE_UNKNOWN
} e_message_type;

/************************************
*       API                         *
************************************/

#endif //__MESSAGE_DEFS_H__