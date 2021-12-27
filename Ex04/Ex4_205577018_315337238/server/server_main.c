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
//
#include "../shared/socket_handle.h"

/************************************
*      definitions                 *
************************************/

/************************************
*       types                       *
************************************/

/************************************
*      variables                    *
************************************/

/************************************
*      static functions             *
************************************/

/************************************
*       API implementation          *
************************************/
int main(int argc, char* argv[])
{
	SOCKET server_socket = Socket_Init(socket_server, "127.0.0.1", 8888);
}

/************************************
* static implementation             *
************************************/
static uint32_t g_game_counter = 0;

//static void temp_game(int move_opponent, char* username, int last_number_before_boom)
//{
//	int move_self, current_move;
//	if (move_opponent != "boom")
//	{
//		current_move = 1 + move_opponent;
//		if ((current_move % 7) == 0 || ((current_move / 10) % 7) == 0)
//		{
//			//current_move = "boom";
//		}
//	}
//
//	else
//	{
//		current_move = last_number_before_boom + 2;
//	}
//
//}
