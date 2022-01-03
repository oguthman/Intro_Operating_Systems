/*!
******************************************************************************
\file socket_handle.h
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

#ifndef __SOCKET_HANDLE_H__
#define __SOCKET_HANDLE_H__



/************************************
*      include                      *
************************************/
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#include <stdint.h>
#include <stdbool.h>

#include "message_defs.h"

/************************************
*      definitions                 *
************************************/
#define MAX_PARAMS_ARGUMENTS			4

/************************************
*       types                       *
************************************/
typedef enum {
	socket_server,
	socket_client
} e_socket_type;

typedef enum { 
	transfer_succeeded,
	transfer_failed,
	transfer_disconnected,
	transfer_timeout
} e_transfer_result;

typedef struct {
	e_message_type message_type;
	uint32_t params_count;
	char* params[MAX_PARAMS_ARGUMENTS];
} s_message_params;

/************************************
*       API                         *
************************************/
SOCKET Socket_Init(e_socket_type type, char* ip, uint16_t port);

e_transfer_result Socket_Send(SOCKET socket, e_message_type message_type, int params_count, char* params[]);

/// <summary>
/// 
/// </summary>
/// <param name="socket"></param>
/// <param name="p_message_type"></param>
/// <param name="params">return allocated list of param, need to free them outside</param>
/// <returns></returns>
e_transfer_result Socket_Receive(SOCKET main_socket, s_message_params* message_params, uint32_t timeout);

void Socket_TearDown(SOCKET socket, bool socket_only);

void Socket_FreeParamsArray(char* params[], uint32_t number_of_params);

#endif //__SOCKET_HANDLE_H__