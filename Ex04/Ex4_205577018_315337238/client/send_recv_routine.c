#define _CRT_SECURE_NO_WARNINGS
/*!
******************************************************************************
\file send_recv_routine.c
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
#include <stdio.h>
//
#include "send_recv_routine.h"
//
#include "../shared/queue.h"
#include "../shared/threads.h"

/************************************
*      definitions                 *
************************************/
#define ASSERT(cond, msg, ...)								\
	do {													\
		if (!(cond)) {										\
			printf(msg, __VA_ARGS__);						\
			return false;									\
		}													\
	} while (0);

/************************************
*       types                       *
************************************/

/************************************
*      variables                    *
************************************/
static SOCKET* g_client_socket;
static bool* g_killing_me_softly_flag;

static struct {
	s_node* transaction_queue;
	HANDLE send_event_handle;
} gs_sending_vars;

static struct {
	receive_callback callback;
	HANDLE receive_event_handle;
	uint32_t timeout;
} gs_receiving_vars;

/************************************
*      static functions             *
************************************/

/************************************
*       API implementation          *
************************************/
bool SendRecvRoutine_Init(SOCKET* client_socket, bool* soft_kill_flag)
{
	g_client_socket = client_socket;
	g_killing_me_softly_flag = soft_kill_flag;

	// init sending
	gs_sending_vars.send_event_handle = create_event_handle(true);
	ASSERT(gs_sending_vars.send_event_handle != NULL, "");
	gs_sending_vars.transaction_queue = NULL;

	// init receiving
	gs_receiving_vars.receive_event_handle = create_event_handle(true);
	ASSERT(gs_receiving_vars.receive_event_handle != NULL, "");
	gs_receiving_vars.callback = NULL;
	gs_receiving_vars.timeout = 15 * 1000; // 15sec
	return true;
}

void SendRecvRoutine_BindCallback(receive_callback callback)
{
	gs_receiving_vars.callback = callback;
}

bool SendRecvRoutine_AddTransaction(s_message_params params)
{
	s_message_params* p_params = malloc(sizeof(s_message_params));
	ASSERT(p_params != NULL, "Error: falied allocating memory\n");
	memcpy(p_params, &params, sizeof(params));
	
	for (uint8_t i = 0; i < params.params_count; i++)
	{
		p_params->params[i] = malloc((strlen(params.params[i]) + 1)* sizeof(char));
		ASSERT(p_params->params[i] != NULL, "Error: falied allocating memory\n");
		strcpy(p_params->params[i], params.params[i]);
	}

	queue_push(&gs_sending_vars.transaction_queue, (void*)p_params, false);

	// notify by event on new transaction
	ASSERT(SetEvent(gs_sending_vars.send_event_handle) != 0, "Error: failed setting event");
	return true;
}

DWORD WINAPI SendRecvRoutine_SendRoutine(LPVOID lpParam)
{
	while ((*g_killing_me_softly_flag) == false)
	{
		// waiting for new params to send
		if (queue_is_empty(&gs_sending_vars.transaction_queue))
			if (ResetEvent(gs_sending_vars.send_event_handle) == 0)
				return 1;	//exitcode

		// waiting for event
		if (!wait_for_event(gs_sending_vars.send_event_handle))
		{
			*g_killing_me_softly_flag = true;
			return 1;	//exitcode
		}

		s_message_params* p_params = queue_pop(&gs_sending_vars.transaction_queue);
		
		// sending packet through socket
		e_transfer_result result = Socket_Send(*g_client_socket, *p_params);
		if (result == transfer_failed)
		{
			*g_killing_me_softly_flag = true;
			Socket_FreeParamsArray(p_params->params, p_params->params_count);
			return 1;	//exitcode
		}

		Socket_FreeParamsArray(p_params->params, p_params->params_count);
	}

	return 0; //TODO: temp - check if exit protocol needed
}

DWORD WINAPI SendRecvRoutine_ReceiveRoutine(LPVOID lpParam)
{
	while ((*g_killing_me_softly_flag) == false)
	{
		// waiting for event
		if (!wait_for_event(gs_receiving_vars.receive_event_handle))
		{
			*g_killing_me_softly_flag = true;
			return 1;	//exitcode
		}
		
		// wait to receive new transaction
		s_message_params message_params = { MESSAGE_TYPE_UNKNOWN };
		uint32_t timeout = gs_receiving_vars.timeout;
		e_transfer_result result = Socket_Receive(*g_client_socket, &message_params, timeout);

		if (result != transfer_succeeded)
		{
			Socket_FreeParamsArray(message_params.params, message_params.params_count);
			*g_killing_me_softly_flag = true;

			// breaking the loop
			if (result == transfer_disconnected)
				printf("server disconnected\n");	// TOOD: Remove
			else if (result == transfer_timeout)
				printf("client socket timeout\n");		// TOOD: Remove
			else 
				printf("client socket failed\n");	// TOOD: Remove

			return result;
		}

		//callback - move the info to ui
		if (gs_receiving_vars.callback != NULL)
			gs_receiving_vars.callback(message_params);

		// TODO: not happy path

		Socket_FreeParamsArray(message_params.params, message_params.params_count);
	}

	return 0;
}

bool SendRecvRoutine_SetReceiveEvent(bool set, uint32_t timeout)
{
	gs_receiving_vars.timeout = timeout;

	if (set) {
		ASSERT(SetEvent(gs_receiving_vars.receive_event_handle) != 0, "Error: failed setting event");
	}
	else {
		ASSERT(ResetEvent(gs_receiving_vars.receive_event_handle) != 0, "Error: failed resetting event");
	}

	return true;
}

void SendRecvRoutine_Teardown()
{
	// free all queue items
	while (!queue_is_empty(&gs_sending_vars.transaction_queue))
	{
		s_message_params* p_params = queue_pop(&gs_sending_vars.transaction_queue);
		Socket_FreeParamsArray(p_params->params, p_params->params_count);
	}

	// close all handlers
	CloseHandle(gs_sending_vars.send_event_handle);
	CloseHandle(gs_receiving_vars.receive_event_handle);
	Socket_TearDown(*g_client_socket, false);
}

/************************************
* static implementation             *
************************************/

