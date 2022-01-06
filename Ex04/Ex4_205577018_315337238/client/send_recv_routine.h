#define _WINSOCK_DEPRECATED_NO_WARNINGS
/*!
******************************************************************************
\file send_recv_routine.h
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

#ifndef __SEND_RECV_ROUTINE_H__
#define __SEND_RECV_ROUTINE_H__

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
bool SendRecvRoutine_Init(SOCKET* client_socket, bool* soft_kill_flag);
void SendRecvRoutine_BindCallback(receive_callback callback);
bool SendRecvRoutine_AddTransaction(s_message_params params);
DWORD WINAPI SendRecvRoutine_SendRoutine(LPVOID lpParam);
DWORD WINAPI SendRecvRoutine_ReceiveRoutine(LPVOID lpParam);
bool SendRecvRoutine_SetReceiveEvent(bool set, uint32_t timeout);
void SendRecvRoutine_Teardown();

#endif //__SEND_RECV_ROUTINE_H__