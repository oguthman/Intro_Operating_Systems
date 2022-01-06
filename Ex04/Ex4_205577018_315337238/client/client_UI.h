/*!
******************************************************************************
\file client_UI.h
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

#ifndef __CLIENT_UI_H__
#define __CLIENT_UI_H__

/************************************
*      include                      *
************************************/
#include "../shared/socket_handle.h"
#include "../shared/FileApi/file.h"

/************************************
*      definitions                 *
************************************/

/************************************
*       types                       *
************************************/
typedef struct
{
	char* server_ip;
	uint16_t port;
	char* username;
} s_server_data;

typedef enum {
	connection_idle,
	connection_succeed,
	connection_denied
} e_connection_state;

typedef void (*send_callback)(s_message_params* params, uint32_t timeout);

/************************************
*       API                         *
************************************/
bool clientUI_init(bool* soft_kill_flag, e_connection_state* connection_state, s_server_data* server_data, File* client_log_file);

void clientUI_bind_send_callback(send_callback callback);

bool clientUI_add_message(s_message_params params);

DWORD WINAPI clientUI_routine(LPVOID lpParam);

/// Description: check user's response to main menu. if invalid value - resend the main menu and wait for valid response. 
/// Parameters: 
///		[in] acceptable_str - valid user responses. 
///		[in] array_length - number of valid user responses. 
///		[in] message - main menu message. 
/// Return: none.
int clientUI_validate_menu_input(char* acceptable_str[], int array_length, char* message);

#endif //__CLIENT_UI_H__