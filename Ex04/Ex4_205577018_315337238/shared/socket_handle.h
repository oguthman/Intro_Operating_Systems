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
#define MAX_PARAMS_ARGUMENTS	4

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
/// Description: open socket for client or server. 
/// Parameters: 
///		[in] type - socket type. 
///		[in] ip
///		[in] port
/// Return: socket handle.
SOCKET Socket_Init(e_socket_type type, char* ip, uint16_t port);

/// Description: handle sent message. 
/// Parameters: 
///		[in] socket - socket handle. 
///		[in] message_params - message_type, message parameters, number of parameters.
/// Return: transfer result.
e_transfer_result Socket_Send(SOCKET socket, s_message_params message_params);

/// Description: build a message string from message params.  
/// Parameters: 
///		[in] message_params - the params to build the string. 
///		[in] buffer - pointer to the buffer string. 
/// Return: none.
bool Socket_BuildBufferFromMessageParams(s_message_params message_params, char** buffer);

/// Description: handle sent message. 
/// Parameters: 
///		[in] main_socket - socket handle. 
///		[in] message_params - message_type, message parameters, number of parameters.
///		[in] timeout - message timeout.
/// Return: transfer result.
e_transfer_result Socket_Receive(SOCKET main_socket, s_message_params* message_params, uint32_t timeout);

/// Description: socket shutdown and socket cleanup.  
/// Parameters: 
///		[in] socket - socket handle.
///		[in] socket_only - flag to perform only socket_cleanup
/// Return: none.
void Socket_TearDown(SOCKET socket, bool socket_only);

/// Description: close parameters array.  
/// Parameters: 
///		[in] params
///		[in] number_of_params
/// Return: none.
void Socket_FreeParamsArray(char* params[], uint32_t number_of_params);

#endif //__SOCKET_HANDLE_H__