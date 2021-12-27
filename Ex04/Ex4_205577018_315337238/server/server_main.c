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
#define ASSERT(cond, msg, ...)														\
	do {																			\
		if (!(cond)) {																\
			printf("Assertion failed at file %s line %d: \n", __FILE__, __LINE__);	\
			printf(msg, __VA_ARGS__);												\
			exit (1);																\
		}																			\
	} while (0);

#define SERVER_IP "127.0.0.1"

/************************************
*       types                       *
************************************/

/************************************
*      variables                    *
************************************/
static uint16_t g_port;

/************************************
*      static functions             *
************************************/
static void parse_arguments(int argc, char* argv[]);

/************************************
*       API implementation          *
************************************/
int main(int argc, char* argv[])
{
	parse_arguments(argc, argv);

	SOCKET server_socket = Socket_Init(socket_server, SERVER_IP, g_port);
	ASSERT(server_socket != INVALID_SOCKET, "Error: server can't open socket\n");		//TODO: exit?

	// lesten on socket
	ASSERT(listen(server_socket, SOMAXCONN) != SOCKET_ERROR, "Error: failed listening on socket %ld\n", WSAGetLastError());		//TODO: action - socket cleanup Socket_TearDown()

	
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
	g_port = strtol(argv[1], NULL, 10);
}


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
