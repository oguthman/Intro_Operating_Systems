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
/// Description: initialize all relevant variables.
/// Parameters: 
///		[in] client_socket - the socket pointer. 
///		[in] soft_kill_flag - soft termination flag pointer. 
/// Return: true if opreation succeeded, and false otherwise.
bool SendRecvRoutine_Init(SOCKET* client_socket, bool* soft_kill_flag);

/// Description: bind a callback for handling received data.
/// Parameters: 
///		[in] callback - the callback handler. 
/// Return: none.
void SendRecvRoutine_BindCallback(receive_callback callback);

/// Description: add new transaction to the sending routine.
/// Parameters: 
///		[in] params - the transaction data. 
/// Return: true if opreation succeeded, and false otherwise.
bool SendRecvRoutine_AddTransaction(s_message_params params);

/// Description: the sending routine.
/// Parameters: 
///		[in] lpParam - NULL. 
/// Return: DWORD 0 if thread routine succeeded or 1 otherwise.
DWORD WINAPI SendRecvRoutine_SendRoutine(LPVOID lpParam);

/// Description: the receiving routine.
/// Parameters: 
///		[in] lpParam - NULL. 
/// Return: DWORD 0 if thread routine succeeded or 1 otherwise.
DWORD WINAPI SendRecvRoutine_ReceiveRoutine(LPVOID lpParam);

/// Description: set timeout event to the receiving routine.
/// Parameters: 
///		[in] set - set/ reset the event. 
///		[in] timeout - the timeout for the receive operation. 
/// Return: true if opreation succeeded, and false otherwise.
bool SendRecvRoutine_SetReceiveEvent(bool set, uint32_t timeout);

/// Description: exit protocol - close events and socket.  
/// Parameters: none.
/// Return: none.
void SendRecvRoutine_Teardown();

#endif //__SEND_RECV_ROUTINE_H__