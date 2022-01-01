#define _CRT_SECURE_NO_WARNINGS
/*!
******************************************************************************
\file client_send_recv.c
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
#include "client_send_recv.h"
#include "../shared/queue.h"
#include "../shared/threads.h"

/************************************
*      definitions                 *
************************************/
// TODO: change return (exit)
#define ASSERT(cond, msg, ...)													\
	do {																					\
		if (!(cond)) {																		\
			printf(msg, __VA_ARGS__);														\
			return;																			\
		}																					\
	} while (0);

/************************************
*       types                       *
************************************/

/************************************
*      variables                    *
************************************/
static SOCKET* g_client_socket;

static struct {
	s_node* transaction_queue;
	HANDLE send_event_handle;
} gs_sending_vars;

static struct {
	receive_callback callback;
} gs_receiving_vars;

/************************************
*      static functions             *
************************************/

/************************************
*       API implementation          *
************************************/
void client_init_send_recv(SOCKET* client_socket)
{
	g_client_socket = client_socket;

	// init sending
	gs_sending_vars.send_event_handle = create_event_handle(true);
	gs_sending_vars.transaction_queue = NULL;

	// init receiving
	gs_receiving_vars.callback = NULL;
}

void client_bind_callback(receive_callback callback)
{
	gs_receiving_vars.callback = callback;
}

void client_add_transaction(s_message_params params)
{
	s_message_params* p_params = malloc(sizeof(s_message_params));
	ASSERT(p_params != NULL, "Error: falied allocating memory\n");
	memcpy(p_params, &params, sizeof(params));
	
	// TODO: check if needed below allocation
	for (uint8_t i = 0; i < params.params_count; i++)
	{
		p_params->params[i] = malloc((strlen(params.params[i]) + 1)* sizeof(char));
		ASSERT(p_params->params[i] != NULL, "Error: falied allocating memory\n");
		strcpy(p_params->params[i], params.params[i]);
	}

	queue_push(&gs_sending_vars.transaction_queue, (void*)p_params, false);

	// notify by event on new transaction
	SetEvent(gs_sending_vars.send_event_handle);
}

DWORD WINAPI client_send_routine(LPVOID lpParam)
{
	while (1)
	{
		// waiting for new params to send
		if (queue_is_empty(&gs_sending_vars.transaction_queue))
			ResetEvent(gs_sending_vars.send_event_handle);

		// waiting for event
		if (!wait_for_event(gs_sending_vars.send_event_handle))
			return 1;	//exitcode
		
		s_message_params* p_params = queue_pop(&gs_sending_vars.transaction_queue);
		
		// sending packet through socket
		e_transfer_result result = Socket_Send(*g_client_socket, p_params->message_type, p_params->params_count, p_params->params);
		if (result == transfer_failed)
		{
			//TODO: NOT HAPPY PATH
		}

		Socket_FreeParamsArray(p_params->params, p_params->params_count);
	}

	return 0; //TODO: temp - check if exit protocol needed
}

DWORD WINAPI client_receive_routine(LPVOID lpParam)
{
	while (1)
	{
		// wait to receive new transaction
		// happy path
		s_message_params message_params = { MESSAGE_TYPE_UNKNOWN };
		uint32_t timeout = 10;	//TODO: change
		e_transfer_result result = Socket_Receive(*g_client_socket, &message_params, timeout);

		if (result == transfer_disconnected)
		{
			// breaking the loop
			printf("socket disconnected\n");
			Socket_FreeParamsArray(message_params.params, message_params.params_count);
			break;
		}

		//callback - move the info to ui
		if (gs_receiving_vars.callback != NULL)
			gs_receiving_vars.callback(message_params);

		// TODO: not happy path

		Socket_FreeParamsArray(message_params.params, message_params.params_count);
	}

	return 0;
}

void client_teardown()
{
	// free all queue items

	// close all handlers

}

/************************************
* static implementation             *
************************************/

