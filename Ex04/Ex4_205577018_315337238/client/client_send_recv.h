#define _WINSOCK_DEPRECATED_NO_WARNINGS
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
#include <stdint.h>
//
#include "../shared/message_defs.h"
#include "../shared/socket_handle.h"

/************************************
*      definitions                 *
************************************/

/************************************
*       types                       *
************************************/
typedef void (*receive_callback)(s_message_params params);

/************************************
*       API                         *
************************************/
bool client_init_send_recv(SOCKET* client_socket, bool* soft_kill_flag);
void client_bind_callback(receive_callback callback);
bool client_add_transaction(s_message_params params);
DWORD WINAPI client_send_routine(LPVOID lpParam);
DWORD WINAPI client_receive_routine(LPVOID lpParam);
bool client_set_receive_event(bool set, uint32_t timeout);
void client_teardown();

#endif //__CLIENT_SEND_RECV_H__