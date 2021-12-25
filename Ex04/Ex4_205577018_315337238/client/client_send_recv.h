/*!
******************************************************************************
\file client_send_recv.h
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

#ifndef __CLIENT_SEND_RECV_H__
#define __CLIENT_SEND_RECV_H__

/************************************
*      include                      *
************************************/
#include <windows.h>
//
#include "../shared/socket_handle.h"

/************************************
*      definitions                 *
************************************/

/************************************
*       types                       *
************************************/
typedef struct {
	e_message_type message_type;
	int params_count;
	char* params[];
} s_client_message_params;

/************************************
*       API                         *
************************************/
void client_init_send_recv(SOCKET socket);

void client_add_transaction(s_client_message_params params);
DWORD WINAPI client_send_routine(LPVOID lpParam);


DWORD WINAPI client_receive_routine(LPVOID lpParam);

void client_teardown();

#endif //__CLIENT_SEND_RECV_H__