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
			return 1;																		\
		}																					\
	} while (0);

#define SERVER_IP "127.0.0.1"
#define NUMBER_OF_ACTIVE_CONNECTIONS	2

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
static DWORD WINAPI client_thread_routine(LPVOID lpParam);

/************************************
*       API implementation          *
************************************/
int main(int argc, char* argv[])
{
	parse_arguments(argc, argv);

	SOCKET server_socket = Socket_Init(socket_server, SERVER_IP, g_port);
	ASSERT(server_socket != INVALID_SOCKET, "Error: server can't open socket\n");		//TODO: exit?

	// listen on socket
	uint8_t number_of_clients_connected = 0;
	ASSERT(listen(server_socket, SOMAXCONN) != SOCKET_ERROR, "Error: failed listening on socket %ld\n", WSAGetLastError());		//TODO: action - socket cleanup Socket_TearDown()

	// accept or decline. create new thread for each client. TODO: WHILE LOOP on number_of_clients_connected
	SOCKET sockets[NUMBER_OF_ACTIVE_CONNECTIONS];
	HANDLE handles[NUMBER_OF_ACTIVE_CONNECTIONS];
	
	while (1)
	{
		SOCKET accept_socket = accept(server_socket, NULL, NULL);
		ASSERT(accept_socket != INVALID_SOCKET, "Error: server can't open socket for client\n");		//TODO: exit?
		// TODO: remove
		printf("Client has connected\n");

		number_of_clients_connected++;
		if (true) //if (number_of_clients_connected > 2)
		{
			// TODO: remove
			printf("Rejecting client\n");
			// send access denied to client
			Socket_Send(accept_socket, MESSAGE_TYPE_SERVER_DENIED, 0, NULL);
			// reject client
			Socket_TearDown(accept_socket, true);

			number_of_clients_connected--;
			continue;
		}

		if (number_of_clients_connected == 1) //TODO: check importance of players order in handles
		{
			// first player
			sockets[0] = accept_socket;
			handles[0] = create_new_thread(client_thread_routine, NULL);
			THREAD_ASSERT(handles[0] != NULL, "Error: failed creating a thread\n");
		}
		else
		{
			// second player
			sockets[1] = accept_socket;
			handles[1] = create_new_thread(client_thread_routine, NULL);
			THREAD_ASSERT(handles[1] != NULL, "Error: failed creating a thread\n");
		}
	}


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

static DWORD WINAPI client_thread_routine(LPVOID lpParam)
{
}

static uint32_t g_game_counter = 0;

// return true if the game is still on, false if player lost in this turn.
static bool temp_game_logic(char** move_self)
{
	uint32_t correct_move, units;
	bool boom = false;
	
	correct_move = 1 + g_game_counter;//TODO: decide when to tik
	units = correct_move % 10;

	if ((correct_move % 7) == 0 || (units % 7) == 0)
	{
		boom = true;
	}
	
	printf("your move\n");
	scanf("%s\n", *move_self);	//with or without \n

	if (boom)
	{
		if (strcmp("boom", *move_self)) return true;
		return false;
	}
	else
	{
		uint32_t number = strtol(*move_self, NULL, 10);
		if (number == correct_move) return true;
	}
	return false;

}
