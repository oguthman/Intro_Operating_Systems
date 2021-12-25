#define _CRT_SECURE_NO_WARNINGS
/*!
******************************************************************************
\file main.c
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
#include <stdint.h>
//
#include "../shared/message_defs.h"

/************************************
*      definitions                 *
************************************/

/************************************
*       types                       *
************************************/

/************************************
*      variables                    *
************************************/

/************************************
*      static functions             *
************************************/

/************************************
*       API implementation          *
************************************/
int main(int argc, char* argv[])
{
	e_message_type p_message_type;
	char** params = NULL;
	char* str = "0:shahar";
	char* buffer = malloc(sizeof(str));
	strcpy(buffer, str);
	int message_length = (int)(strlen(buffer) + 1);

	char* message_type_str = strtok(buffer, ":");
	
	// TODO: Remove
	printf("recieve: %s\n", message_type_str);
	p_message_type = strtol(message_type_str, NULL, 10);

	// getting params
	int num_of_params = 0;
	message_type_str = strtok(NULL, ";");
	while (message_type_str != NULL)
	{
		num_of_params++;
		// TODO: Remove
		printf("recieve: %s\n", message_type_str);

		// TODO: free params after done with them (params list & items)
		params = realloc(params, num_of_params * sizeof(char*));
		params[num_of_params - 1] = malloc(sizeof(message_type_str));
		strcpy(params[num_of_params - 1], message_type_str);

		message_type_str = strtok(NULL, ";");
	}

	free(buffer);

}

/************************************
* static implementation             *
************************************/

