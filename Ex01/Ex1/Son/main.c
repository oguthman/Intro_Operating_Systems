#define _CRT_SECURE_NO_WARNINGS
/*!
******************************************************************************
\file main.c
\date 24 October 2021
\author Shahr Dorit Morag & Ofir Guthman
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
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
//
#include "encrypter.h"

/************************************
*      definitions                 *
************************************/
#define NUMBER_OF_BYTES		16

/************************************
*       types                       *
************************************/

/************************************
*      variables                    *
************************************/
static FILE* gMessageFile = NULL;
static FILE* gKeyFile = NULL;
static uint32_t gOffset = 0;

/************************************
*      static functions             *
************************************/
static void parse_arguments(int argc, char* argv[]);
static void extract_key(char* key);
static void extract_text(char* text);
//
static void write_encryption_to_file(char* text);
static void close_files(void);

/************************************
*       API implementation          *
************************************/
int main(int argc, char* argv[])
{
	parse_arguments(argc, argv);
	//
	// 
	char key[NUMBER_OF_BYTES + 1];
	extract_key(key);
	// init the encrypter
	Encrypter_Init(key);
	//
	char text[NUMBER_OF_BYTES + 1];
	char encryptedText[NUMBER_OF_BYTES + 1];
	// extract the text for the encrypter
	extract_text(text);
	// run encryption
	Encrypter_RunEncryption(text, encryptedText);
	write_encryption_to_file(encryptedText);

	printf("original text - %s\n", text);
	printf("encrypted text - %s\n", encryptedText);

	close_files();
	return 0;
}

/************************************
* static implementation             *
************************************/
static void parse_arguments(int argc, char* argv[])
{
	// check if there is exact args.
	assert(argc == 4);
	//
	// open files
	if ((gMessageFile = fopen(argv[1], "r")) == NULL || (gKeyFile = fopen(argv[3], "r")) == NULL)
	{
		printf("Error: failed opening files. \n");
		exit(1);
	}
	//
	char* ptr;
	gOffset = strtol(argv[2], &ptr, 10);
}

static void extract_key(char* key)
{
	// read the key from the file.
	fscanf(gKeyFile, "%s", key);
	
	// todo: what to do if the key is not len of 16.
}

static void extract_text(char* text)
{
	// set offset location to brgin read
	fseek(gMessageFile, gOffset, SEEK_SET);
	fgets(text, NUMBER_OF_BYTES + 1, gMessageFile);
	if (strlen(text) != NUMBER_OF_BYTES)
	{
		printf("couldn't find sub string in length of %d", NUMBER_OF_BYTES);
		exit(1);
	}
}

static void write_encryption_to_file(char* text)
{
	FILE* result_file = NULL;
	if ((result_file = fopen("Encrypted_message.txt", "w")) == NULL)
	{
		printf("Error: failed opening Encrypted_message.txt. \n");
		exit(1);
	}

	fseek(result_file, gOffset, SEEK_SET);
	fprintf(result_file, "%s", text);
	fclose(result_file);
}

static void close_files(void)
{
	fclose(gMessageFile);
	fclose(gKeyFile);
}