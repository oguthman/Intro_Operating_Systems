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
static SOCKET g_client_socket;

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
static HANDLE create_event_handle(bool manual_reset);
static bool wait_for_event(HANDLE event);

/************************************
*       API implementation          *
************************************/
void client_init_send_recv(SOCKET client_socket)
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

void client_add_transaction(s_client_message_params params)
{
	s_client_message_params* p_params = malloc(sizeof(s_client_message_params));
	ASSERT(p_params != NULL, "Error: falied allocating memory\n");
	
	memcpy(p_params, &params, sizeof(params));
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
		
		s_client_message_params* p_params = queue_pop(&gs_sending_vars.transaction_queue);
		
		// sending packet through socket
		e_transfer_result result = Socket_Send(g_client_socket, p_params->message_type, p_params->params_count, p_params->params);
		if (result == transfer_failed)
		{
			//TODO: NOT HAPPY PATH
		}

		free(p_params);
	}

	return 0; //TODO: temp - check if exit protocol needed
}

DWORD WINAPI client_receive_routine(LPVOID lpParam)
{
	while (1)
	{
		// wait to receive new transaction
		// happy path
		e_message_type message_type = MESSAGE_TYPE_UNKNOWN;
		char** params = NULL;
		uint32_t number_of_params = 0;
		uint32_t timeout = 10;	//TODO: change
		e_transfer_result result = Socket_Receive(g_client_socket, &message_type, params, &number_of_params, timeout);

		if (result == transfer_disconnected)
		{
			// breaking the loop
			Socket_FreeParamsArray(params, number_of_params);
			break;
		}

		//callback - move the info to ui
		s_client_message_params message_params = { message_type, number_of_params, params };
		if (gs_receiving_vars.callback != NULL)
			gs_receiving_vars.callback(message_params);

		// TODO: not happy path

		Socket_FreeParamsArray(params, number_of_params);
	}
}

void client_teardown()
{
	// free all queue items

	// close all handlers

}

/************************************
* static implementation             *
************************************/
static HANDLE create_event_handle(bool manual_reset)
{
	HANDLE event = NULL;

	event = CreateEvent(
		NULL,				// default security attributes
		manual_reset,		// manual reset event
		false,				// initail state
		NULL);				// object name

	if (event == NULL)
	{
		printf("Error: CreateEvent failed (%d)\n", GetLastError());
		return NULL;
	}

	return event;
}

static bool wait_for_event(HANDLE event)
{
	DWORD code = WaitForSingleObject(event, INFINITE);
	if (code != WAIT_OBJECT_0)
	{
		printf("Error: waiting to event failed (%d)\n", GetLastError());
		return false;
	}

	return true;
}
