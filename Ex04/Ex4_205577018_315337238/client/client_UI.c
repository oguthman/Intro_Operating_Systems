#define _CRT_SECURE_NO_WARNINGS
/*!
******************************************************************************
\file client_UI.c
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
#include "client_UI.h"
//
#include "../shared/queue.h"
#include "../shared/threads.h"

/************************************
*      definitions                 *
************************************/
#define ASSERT(cond, msg, ...)					\
	do {										\
		if (!(cond)) {							\
			printf(msg, __VA_ARGS__);			\
			return false;						\
		}										\
	} while (0);

#define LOG_PRINTF(msg, ...)																\
	do {																					\
		printf(msg, __VA_ARGS__);															\
		if (*g_client_log_file != NULL)														\
			File_Printf(*g_client_log_file, msg, __VA_ARGS__);								\
	} while (0);


/************************************
*       types                       *
************************************/

/************************************
*      variables                    *
************************************/
static bool* g_killing_me_softly_flag;
static e_connection_state* g_connection_state;
static s_server_data* g_server_data;
static File* g_client_log_file;
//
static s_node* g_message_queue;
static HANDLE g_event_handle;
static send_callback g_send_callback;

/************************************
*      static functions             *
************************************/
static void data_received_handle(s_message_params message_params);
static char* string_to_lower(char* str);
static void vaildate_user_move(char* accepatble_move, char* message);

/************************************
*       API implementation          *
************************************/
bool clientUI_init(bool* soft_kill_flag, e_connection_state * connection_state, s_server_data* server_data, File* client_log_file)
{
	g_killing_me_softly_flag = soft_kill_flag;
	g_connection_state = connection_state;
	g_server_data = server_data;
	g_client_log_file = client_log_file;

	// init sending
	g_event_handle = create_event_handle(true);
	ASSERT(g_event_handle != NULL, "");
	
	g_message_queue = NULL;
	g_send_callback = NULL;
	return true;
}

void clientUI_bind_send_callback(send_callback callback)
{
	g_send_callback = callback;
}

bool clientUI_add_message(s_message_params params)
{
	s_message_params* p_params = malloc(sizeof(s_message_params));
	ASSERT(p_params != NULL, "Error: falied allocating memory\n");
	memcpy(p_params, &params, sizeof(params));

	for (uint8_t i = 0; i < params.params_count; i++)
	{
		p_params->params[i] = malloc((strlen(params.params[i]) + 1) * sizeof(char));
		ASSERT(p_params->params[i] != NULL, "Error: falied allocating memory\n");
		strcpy(p_params->params[i], params.params[i]);
	}

	queue_push(&g_message_queue, (void*)p_params, false);

	// notify by event on new transaction
	SetEvent(g_event_handle);
	return true;
}

DWORD WINAPI clientUI_routine(LPVOID lpParam)
{
	while ((*g_killing_me_softly_flag) == false)
	{
		// waiting for new params to send
		if (queue_is_empty(&g_message_queue))
			ResetEvent(g_event_handle);

		// waiting for event
		if (!wait_for_event(g_event_handle))
		{
			*g_killing_me_softly_flag = true;
			return 1;	//exitcode
		}

		s_message_params* p_params = queue_pop(&g_message_queue);
		data_received_handle(*p_params);

		Socket_FreeParamsArray(p_params->params, p_params->params_count);
	}

	return 0;
}

int clientUI_validate_menu_input(char* acceptable_str[], int array_length, char* message)
{
	do
	{
		// print require message
		printf("%s", message);

		// get user input
		char input[256];
		gets_s(input, sizeof(input));

		// check if input is in the acceptable array
		for (int i = 0; i < array_length; i++)
			if (!strcmp(input, acceptable_str[i]))
				return strtol(input, NULL, 10);

		LOG_PRINTF("Error: Illegal command\n");
	} while (1);
}

void clientUI_teardown()
{
	// free all queue items
	while (!queue_is_empty(&g_message_queue))
	{
		s_message_params* p_params = queue_pop(&g_message_queue);
		Socket_FreeParamsArray(p_params->params, p_params->params_count);
	}

	// close all handlers
	CloseHandle(g_event_handle);
}

/************************************
* static implementation             *
************************************/
static void data_received_handle(s_message_params message_params)
{
	switch (message_params.message_type)
	{
		case MESSAGE_TYPE_SERVER_APPROVED:
			*g_connection_state = connection_succeed;
			break;
		case MESSAGE_TYPE_SERVER_DENIED:
			*g_connection_state = connection_denied;
			printf("Server on %s:%d denied the connection request.\n", g_server_data->server_ip, g_server_data->port);
			break;
			// TODO: check if needed, becuase I think the server always send MESSAGE_TYPE_SERVER_NO_OPPONENTS and than MESSAGE_TYPE_SERVER_MAIN_MENU
			// case MESSAGE_TYPE_SERVER_NO_OPPONENTS: 
		case MESSAGE_TYPE_SERVER_MAIN_MENU:
		{
			char* message = "Choose what to do next:\n1. Play against another client\n2. Quit\n";
			char* acceptable_chars[] = { "1", "2" };
			g_send_callback(NULL, INFINITE);	// Wait infinite time for client decision
			switch (clientUI_validate_menu_input(acceptable_chars, 2, message))
			{
			case 1:
			{
				s_message_params send_message_params = { .message_type = MESSAGE_TYPE_CLIENT_VERSUS };
				g_send_callback(&send_message_params, 15 * 1000);
				break;
			}
			case 2:
				// quit the program
				*g_killing_me_softly_flag = true;
				break;
			}
			break;
		}
		case MESSAGE_TYPE_GAME_STARTED:
			printf("Game is on!\n");
			g_send_callback(NULL, 30 * 1000);		// Set waiting time to 30sec
			break;
		case MESSAGE_TYPE_TURN_SWITCH:
			if (!strcmp(message_params.params[0], g_server_data->username))
				printf("Your turn!\n");
			else
				printf("%s's turn!\n", message_params.params[0]);
			break;
		case MESSAGE_TYPE_SERVER_MOVE_REQUEST:
		{
			char* message = "Enter the next number or boom:\n";
			char send_string[256] = "";
			vaildate_user_move(send_string, message);

			// send user move
			s_message_params send_message_params = { .message_type = MESSAGE_TYPE_CLIENT_PLAYER_MOVE, .params_count = 1 };
			send_message_params.params[0] = send_string;
			g_send_callback(&send_message_params, 30 * 1000);
			break;
		}
		case MESSAGE_TYPE_GAME_ENDED:
			printf("%s won!\n", message_params.params[0]);
			break;
		case MESSAGE_TYPE_GAME_VIEW:
			// TODO: maybe shouhld check the num_of_params
			printf("%s move was %s\n", message_params.params[0], message_params.params[1]);
			if (!strcmp(message_params.params[2], "END"))
				printf("END\n");
			else
			{
				//TODO: CHECK IF NEEDED TO RETURN PRINT
				printf("CONT\n");
			}
			break;
		case MESSAGE_TYPE_SERVER_OPPONENT_QUIT:
			printf("Opponent quit.\n");	//TODO: MAKE SURE THE PRINT IS CORRECT "Opponent quit .\n"
			break;
	}
	g_send_callback(NULL, 15 * 1000);		// Set waiting time to 15sec
}

/// Description: check user's game move validity. if invalid value - resend the next move message and wait for valid response. 
/// Parameters: 
///		[in] accepatble_move - valid user game move. 
///		[in] message - next move message. 
/// Return: none.
static void vaildate_user_move(char* accepatble_move, char* message)
{
	static char boom[] = "boom";
	do
	{
		// print require message
		printf("%s", message);

		// get user input
		char input[256];
		gets_s(input, sizeof(input));

		// check if input is 'boom'
		if (!strcmp(string_to_lower(input), boom) || strspn(input, "0123456789") == strlen(input))
		{
			strcpy(accepatble_move, string_to_lower(input));
			return;
		}

		LOG_PRINTF("Error: Illegal command\n");
	} while (1);
}

/// Description: change string from upper case to lower case.
/// Parameters: 
///		[in] str - upper case string. 
/// Return: lower case string.
static char* string_to_lower(char* str)
{
	for (int i = 0; i < (int)strlen(str); i++)
		str[i] = tolower(str[i]);

	return str;
}
