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
#define ASSERT(cond, action, ...)		\
		if (!cond) {					\
			printf(__VA_ARGS__);		\
			if (action != NULL)			\
				action(socket);			\	
			return NULL;				\
		};								

/************************************
*       types                       *
************************************/

/************************************
*      variables                    *
************************************/

/************************************
*      static functions             *
************************************/
static void wsa_cleanup(SOCKET socket);
static void socket_cleanup(SOCKET socket);
static e_transfer_result send_buffer(SOCKET socket, const char* buffer, uint32_t bytes_to_send);
static e_transfer_result receive_buffer(SOCKET socket, char* buffer, uint32_t bytes_to_receive);

/************************************
*       API implementation          *
************************************/
SOCKET Socket_Init(e_socket_type type, char* ip, uint16_t port)
{
	// Initialize Winsock.
	WSADATA wsaData;
	ASSERT(WSAStartup(MAKEWORD(2, 2), &wsaData) == NO_ERROR, NULL, "Error %ld at WSAStartup()\n", WSAGetLastError());

	// open socket
	SOCKET socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	// Check for errors to ensure that the socket is a valid socket.
	ASSERT(socket != INVALID_SOCKET, wsa_cleanup, "Error at socket(): %ld\n", WSAGetLastError());
	
	if (type == socket_client)
	{
		// Create a sockaddr_in object clientService and set values.
		SOCKADDR_IN clientService = {
			.sin_family = AF_INET,
			.sin_addr.s_addr = inet_addr(ip),			//Setting the IP address to connect to
			.sin_port = htons(port)						//Setting the port to connect to.
		};
		
		// Try to connect to server
		ASSERT((connect(socket, (SOCKADDR*)&clientService, sizeof(clientService)) != SOCKET_ERROR), socket_cleanup, "Failed to connect to server.\n");
	}
	else if (type == socket_server)
	{
		// Create a sockaddr_in object and set its values.
		uint64_t address = inet_addr(ip);
		ASSERT(address != INADDR_NONE, socket_cleanup, "The string \"%s\" cannot be converted into an ip address.\n", ip);

		SOCKADDR_IN service = {
			.sin_family = AF_INET,
			.sin_addr.s_addr = address,				//Setting the IP address to connect to
			.sin_port = htons(port)					//Setting the port to connect to.
		};

		int bindRes = bind(socket, (SOCKADDR*)&service, sizeof(service));
		ASSERT(bindRes == SOCKET_ERROR, socket_cleanup, "bind( ) failed with error %ld\n", WSAGetLastError());
	}

	return socket;
}

e_transfer_result Socket_Send(SOCKET socket, e_message_type message_type, int params_count, char* params[])
{
	e_transfer_result transfer_result;
	// creating the string
	int message_length;
	if (params_count == 0)
		message_length = snprintf(NULL, 0, "%d\n", message_type);
	else if (params_count == 1)
		message_length = snprintf(NULL, 0, "%d:%s\n", message_type, params[0]);
	else if (params_count == 2)
		message_length = snprintf(NULL, 0, "%d:%s;%s\n", message_type, params[0], params[1]);
	else {
		message_length = snprintf(NULL, 0, "%d:%s;%s;%s\n", message_type, params[0], params[1], params[2]);
	}
}
	char* buffer = malloc((message_length) * sizeof(char));
	if (NULL == buffer)
	{
		printf("Error: failed allocating buffer for message\n");
		return transfer_failed;
	}

	if (params_count == 0)
		snprintf(buffer, message_length, "%d\n", message_type);
	else if (params_count == 1)
		snprintf(buffer, message_length, "%d:%s\n", message_type, params[0]);
	else if (params_count == 2)
		snprintf(buffer, message_length, "%d:%s;%s\n", message_type, params[0], params[1]);
	else 
		snprintf(buffer, message_length, "%d:%s;%s;%s\n", message_type, params[0], params[1], params[2]);

	// sending the packet length
	string_size = (int)(strlen(buffer) + 1); // terminating zero also sent	
	transfer_result = send_buffer(socket, (const char*)(&string_size), (int)(sizeof(string_size)));

	if (transfer_result != transfer_succeeded)
	{
		free(buffer);
		return transfer_result;
	}

	// sending the real packet
	transfer_result = send_buffer(socket, (const char*)(buffer), string_size);
	free(buffer);
	return transfer_result;
}

e_transfer_result Socket_Receive(SOCKET socket, e_message_type* p_message_type, char* params[])
{
	e_transfer_result transfer_result;
	// reading buffer length
	int message_length;
	transfer_result = receive_buffer(socket, (char *))(&message_length), int(sizeof(message_length));
	
	if (transfer_result != transfer_succeeded)
	{
		free(buffer);
		return transfer_result;
	}

	// read the real buffer
	char* buffer = malloc(message_length * sizeof(char));
	if (buffer == NULL)
	{
		printf("Error: failed allocating buffer for message\n");
		return transfer_failed;
	}
	
	transfer_result = receive_buffer(socket, buffer, message_length);
	
	if (transfer_result != transfer_succeeded)
	{
		free(buffer);
		return transfer_result;
	}

	char* message_type_str = strtok(buffer, ":");
	if (strcmp(message_type_str, buffer) == 0)
		p_message_type = (e_message_type) strtol(buffer, NULL, 10);
	else
	{
		// TODO: Remove
		printf("recieve: %s\n", message_type_str);
		p_message_type = (e_message_type)strtol(message_type_str, NULL, 10);
		
		// getting params
		int num_of_params = 0;
		while (message_type_str != NULL)
		{
			num_of_params++;
			message_type_str = strtok(NULL, ";");
			// TODO: Remove
			printf("recieve: %s\n", message_type_str);

			// TODO: free params after done with them (params list & items)
			params = realloc(params, num_of_params * sizeof(char*));
			// TODO: check allocation
			params[num_of_params - 1] = malloc(sizeof(message_type_str));
			strcpy(params[num_of_params - 1], message_type_str);
		}
	}

	free(buffer);
	return transfer_result;
}

void Socket_TearDown(SOCKET socket)
{
	socket_cleanup(socket);
}

/************************************
* static implementation             *
************************************/
static void wsa_cleanup(SOCKET socket)		// entering socket just for compilation reasons (ASSERT)
{
	if (WSACleanup() == SOCKET_ERROR)
		printf("Failed to close Winsocket, error %ld.\n", WSAGetLastError());
}

static void socket_cleanup(SOCKET socket)
{
	if (closesocket(socket) == SOCKET_ERROR)
		printf("Failed to close MainSocket, error %ld. Ending program\n", WSAGetLastError());
	wsa_cleanup();
}

static e_transfer_result send_buffer(SOCKET socket, const char* buffer, uint32_t bytes_to_send)
{
	const char* p_current_buffer = buffer;
	int bytes_transferred;
	int remaining_bytes_to_send = bytes_to_send;

	while (remaining_bytes_to_send > 0)
	{
		// send does not guarantee that the entire message is sent 
		bytes_transferred = send(socket, p_current_buffer, remaining_bytes_to_send, 0);
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

static e_transfer_result receive_buffer(SOCKET socket, char* buffer, uint32_t bytes_to_receive)
{
	char* p_current_buffer = buffer;
	int bytes_transferred;
	int remaining_bytes_to_receive = bytes_to_receive;

	while (remaining_bytes_to_receive > 0)
	{
		// send does not guarantee that the entire message is sent
		bytes_transferred = recv(socket, p_current_buffer, remaining_bytes_to_receive, 0);
		if (bytes_transferred == SOCKET_ERROR)
		{
			printf("recv() failed, error %d\n", WSAGetLastError());
			return transfer_failed;
		}
		else if (bytes_transferred == 0)
			return transfer_disconnected; // recv() returns zero if connection was gracefully disconnected.

		remaining_bytes_to_receive -= bytes_transferred;
		p_current_buffer += bytes_transferred;
	}

	return transfer_succeeded;
}
