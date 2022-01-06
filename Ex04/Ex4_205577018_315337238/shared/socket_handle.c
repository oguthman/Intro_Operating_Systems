#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
/*!
******************************************************************************
\file socket_handle.c
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
#include <string.h>
#include <stdio.h>
//
#include "socket_handle.h"

/************************************
*      definitions                 *
************************************/
#define ASSERT(cond, action, arg, msg, ...)													\
	do {																					\
		if (!(cond)) {																		\
			printf(msg, __VA_ARGS__);														\
			if (action != NULL)																\
				action(arg, false);															\
			return INVALID_SOCKET;															\
		}																					\
	} while (0);

/************************************
*       types                       *
************************************/
typedef void (*func_ptr)(void*, bool);

/************************************
*      variables                    *
************************************/

/************************************
*      static functions             *
************************************/
static void wsa_cleanup(SOCKET socket, bool socket_only);
static void socket_cleanup(SOCKET socket, bool socket_only);
static e_transfer_result send_buffer(SOCKET socket, const char* buffer, uint32_t bytes_to_send);
static e_transfer_result receive_buffer(SOCKET socket, char* buffer, uint32_t bytes_to_receive);

/************************************
*       API implementation          *
************************************/
SOCKET Socket_Init(e_socket_type type, char* ip, uint16_t port)
{
	// Initialize Winsock.
	WSADATA wsaData;
	int StartupRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	
	func_ptr null_func = NULL;	// doing this just for the assertion
	ASSERT(StartupRes == NO_ERROR, null_func, NULL, "Error %ld at WSAStartup()\n", WSAGetLastError());

	// open socket
	SOCKET main_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	// check for errors to ensure that the socket is a valid socket.
	ASSERT(main_socket != INVALID_SOCKET, wsa_cleanup, main_socket, "Error at socket(): %ld\n", WSAGetLastError());
	
	if (type == socket_client)
	{
		// create a sockaddr_in object clientService and set values.
		SOCKADDR_IN clientService = {
			.sin_family = AF_INET,
			.sin_addr.s_addr = inet_addr(ip),			//Setting the IP address to connect to
			.sin_port = htons(port)						//Setting the port to connect to.
		};
		
		// connect - try to connect to server
		ASSERT((connect(main_socket, (SOCKADDR*)&clientService, sizeof(clientService)) != SOCKET_ERROR), socket_cleanup, main_socket, "Failed to connect to server.\n");
	}
	else if (type == socket_server)
	{
		// create a sockaddr_in object and set its values.
		ULONG address = inet_addr(ip);
		ASSERT(address != INADDR_NONE, socket_cleanup, main_socket, "The string \"%s\" cannot be converted into an ip address.\n", ip);

		SOCKADDR_IN service = {
			.sin_family = AF_INET,
			.sin_addr.s_addr = address,				//Setting the IP address to connect to
			.sin_port = htons(port)					//Setting the port to connect to.
		};

		// bind
		int bindRes = bind(main_socket, (SOCKADDR*)&service, sizeof(service));
		ASSERT(bindRes != SOCKET_ERROR, socket_cleanup, main_socket, "bind( ) failed with error %ld\n", WSAGetLastError());
		ASSERT(listen(main_socket, SOMAXCONN) != SOCKET_ERROR, socket_cleanup, main_socket, "Error: failed listening on socket %ld\n", WSAGetLastError());
	}

	return main_socket;
}

e_transfer_result Socket_Send(SOCKET main_socket, s_message_params message_params)
{
	e_transfer_result transfer_result;
	char* buffer = NULL;
	if (!Socket_BuildBufferFromMessageParams(message_params, &buffer))
		return transfer_failed;

	// sending the packet length
	int string_size = (int)(strlen(buffer) + 1); // terminating zero also sent	
	transfer_result = send_buffer(main_socket, (const char*)(&string_size), (int)(sizeof(string_size)));

	if (transfer_result != transfer_succeeded)
	{
		free(buffer);
		return transfer_result;
	}
	
	// TODO: Remove
	printf("send buffer: '%s'\n", buffer);

	// sending the real packet
	transfer_result = send_buffer(main_socket, (const char*)(buffer), string_size);
	free(buffer);
	return transfer_result;
}

bool Socket_BuildBufferFromMessageParams(s_message_params message_params, char** buffer)
{
	char* message_str = get_message_str(message_params.message_type);
	// creating the string
	int message_length;
	if (message_params.params_count == 0)
		message_length = snprintf(NULL, 0, "%s\n", message_str);
	else if (message_params.params_count == 1)
		message_length = snprintf(NULL, 0, "%s:%s\n", message_str, message_params.params[0]);
	else if (message_params.params_count == 2)
		message_length = snprintf(NULL, 0, "%s:%s;%s\n", message_str, message_params.params[0], message_params.params[1]);
	else
		message_length = snprintf(NULL, 0, "%s:%s;%s;%s\n", message_str, message_params.params[0], message_params.params[1], message_params.params[2]);

	*buffer = malloc((message_length) * sizeof(char));
	if (NULL == *buffer)
	{
		printf("Error: failed allocating buffer for message\n");
		return false;
	}

	if (message_params.params_count == 0)
		snprintf(*buffer, message_length, "%s\n", message_str);
	else if (message_params.params_count == 1)
		snprintf(*buffer, message_length, "%s:%s\n", message_str, message_params.params[0]);
	else if (message_params.params_count == 2)
		snprintf(*buffer, message_length, "%s:%s;%s\n", message_str, message_params.params[0], message_params.params[1]);
	else
		snprintf(*buffer, message_length, "%s:%s;%s;%s\n", message_str, message_params.params[0], message_params.params[1], message_params.params[2]);

	return true;
}

// TODO: implement timeout
e_transfer_result Socket_Receive(SOCKET main_socket, s_message_params* message_params, uint32_t timeout)
{
	// setting timeout
	setsockopt(main_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

	e_transfer_result transfer_result;
	// read buffer length
	int message_length;
	transfer_result = receive_buffer(main_socket, (char*)(&message_length), (uint32_t)(sizeof(message_length)));
	
	if (transfer_result != transfer_succeeded)
		return transfer_result;

	// read the real buffer
	char* buffer = malloc(message_length * sizeof(char));
	if (buffer == NULL)
	{
		printf("Error: failed allocating buffer for message\n");
		return transfer_failed;
	}
	
	transfer_result = receive_buffer(main_socket, buffer, message_length);
	
	if (transfer_result != transfer_succeeded)
	{
		free(buffer);
		return transfer_result;
	}

	// TODO: Remove
	printf("recv buffer: '%s'\n", buffer);

	char* message_type_str = strtok(buffer, ":");
	message_params->message_type = get_message_type(message_type_str);

	// get params
	message_params->params_count = 0;
	message_type_str = strtok(NULL, ";");
	while (message_type_str != NULL)	// strtok returns NULL when there is no split left
	{
		message_params->params_count++;

		// TODO: free params after done with them (params list & items)
		
		message_params->params[message_params->params_count - 1] = malloc((strlen(message_type_str) + 1) * sizeof(char));
		ASSERT(message_params->params[message_params->params_count - 1] != NULL, socket_cleanup, main_socket, "Error: failed allocation memory\n");  // TODO: fix action
		strcpy(message_params->params[message_params->params_count - 1], message_type_str);

		message_type_str = strtok(NULL, ";");
	}

	free(buffer);
	return transfer_result;
}

void Socket_TearDown(SOCKET main_socket, bool socket_only)
{
	shutdown(main_socket, SD_SEND);
	
	s_message_params params;
	if (Socket_Receive(main_socket, &params, 5000) != transfer_disconnected)
		printf("didn't receive disconnection from the other side\n");

	socket_cleanup(main_socket, socket_only);
}

void Socket_FreeParamsArray(char* params[], uint32_t number_of_params)
{
	if (params == NULL)
		return;

	for (uint32_t i = 0; i < number_of_params; i++) // TODO: maybe change to function
	{
		if (params[i] != NULL) // handle warning
			free(params[i]);
	}
}

/************************************
* static implementation             *
************************************/
/// Description: 
/// Parameters: 
///		[in] type - socket type. 
///		[in] ip
///		[in] port
/// Return: socket handle.
static void wsa_cleanup(SOCKET main_socket, bool socket_only)		// entering socket just for compilation reasons (ASSERT) & socket_only
{
	if (WSACleanup() == SOCKET_ERROR)
		printf("Failed to close Winsocket, error %ld.\n", WSAGetLastError());
}

/// Description: socket cleanup.  
/// Parameters: 
///		[in] main_socket - socket handle.
///		[in] socket_only - flag to perform only socket_cleanup
/// Return: none.
static void socket_cleanup(SOCKET main_socket, bool socket_only)
{
	if (closesocket(main_socket) == SOCKET_ERROR)
		printf("Failed to close MainSocket, error %ld. Ending program\n", WSAGetLastError());
	if(!socket_only) 
		wsa_cleanup(main_socket, false);
}

/// Description: send buffer handle.  
/// Parameters: 
///		[in] main_socket - socket handle.
///		[in] buffer - buffer to send.
///		[in] bytes_to_send - number of bytes to send.
/// Return: transfer result.
static e_transfer_result send_buffer(SOCKET main_socket, const char* buffer, uint32_t bytes_to_send)
{
	const char* p_current_buffer = buffer;
	int bytes_transferred;
	int remaining_bytes_to_send = bytes_to_send;

	while (remaining_bytes_to_send > 0)
	{
		// send does not guarantee that the entire message is sent 
		bytes_transferred = send(main_socket, p_current_buffer, remaining_bytes_to_send, 0);
		if (bytes_transferred == SOCKET_ERROR)
		{
			printf("send() failed, error %d\n", WSAGetLastError());
			return transfer_failed;
		}

		remaining_bytes_to_send -= bytes_transferred;
		p_current_buffer += bytes_transferred;
	}

	return transfer_succeeded;
}

/// Description: receive buffer handle.  
/// Parameters: 
///		[in] main_socket - socket handle.
///		[in] bytes_to_send - number of bytes to be received.
///		[out] buffer - buffer to be filled with received message.
/// Return: transfer result.
static e_transfer_result receive_buffer(SOCKET main_socket, char* buffer, uint32_t bytes_to_receive)
{
	char* p_current_buffer = buffer;
	int bytes_transferred;
	int remaining_bytes_to_receive = bytes_to_receive;

	while (remaining_bytes_to_receive > 0)
	{
		// send does not guarantee that the entire message is sent
		bytes_transferred = recv(main_socket, p_current_buffer, remaining_bytes_to_receive, 0);
		if (bytes_transferred == SOCKET_ERROR)
		{
			int last_error = WSAGetLastError();
			if (last_error == WSAETIMEDOUT)
				return transfer_timeout;

			printf("recv() failed, error %d\n", last_error);
			return transfer_failed;
		}
		else if (bytes_transferred == 0)
			return transfer_disconnected; // recv() returns zero if connection was gracefully disconnected.

		remaining_bytes_to_receive -= bytes_transferred;
		p_current_buffer += bytes_transferred;
	}

	return transfer_succeeded;
}
