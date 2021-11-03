#define _CRT_SECURE_NO_WARNINGS
/*!
******************************************************************************
\file main.c
\date 23 October 2021
\author Shahar Dorit Morag & Ofir Guthman
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


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
static FILE *pg_message_file = NULL;
static FILE *pg_key_file = NULL;
static uint32_t g_offset = 0;

/************************************
*      static functions             *
************************************/
static void parse_arguments(int argc, char *argv[]);
static void extract_key(char *key);
static void extract_text(char *text);
//
static void run_encryption(char *text, char *key, char *text_encrypted);
static void write_encryption_to_file(char *text);
static void close_files(void);

/************************************
*       API implementation          *
************************************/
int main(int argc, char *argv[])
{
	parse_arguments(argc, argv);
	
	char key[NUMBER_OF_BYTES + 1];
	extract_key(key);
	
	char text[NUMBER_OF_BYTES + 1];
	char text_encrypted[NUMBER_OF_BYTES + 1];
	// extract the text for the encrypter
	extract_text(text);
	// run encryption
	run_encryption(text, key, text_encrypted);
	write_encryption_to_file(text_encrypted);

	printf("Son: Original text - %s\n", text);
	printf("Son: Encrypted text - %s\n", text_encrypted);

	close_files();
	return 0;
}

/************************************
* static implementation             *
************************************/
static void parse_arguments(int argc, char *argv[])
{
	// check if there is exact args.
	if (argc != 4)
	{
		printf("Error: not exact args number. \n");
		exit(1);
	}
	
	// open files
	if ((pg_message_file = fopen(argv[1], "r")) == NULL || (pg_key_file = fopen(argv[3], "r")) == NULL)
	{
		printf("Error: failed opening files. \n");
		exit(1);
	}
	
	printf("exe - %s\n", argv[0]);
	printf("text - %s\n", argv[1]);
	printf("key - %s\n", argv[3]);

	char *ptr;
	// str to uint
	g_offset = strtol(argv[2], &ptr, 10);
}

static void extract_key(char *key)
{
	// read the key from the file.
	if (!fscanf(pg_key_file, "%s", key))
	{
		printf("Error: couldn't extract key from file.  \n");
		exit(1);
	}
}

static void extract_text(char *text)
{
	// set offset location to begin reading
	fseek(pg_message_file, g_offset, SEEK_SET);
	fgets(text, NUMBER_OF_BYTES + 1, pg_message_file);
	if (strlen(text) != NUMBER_OF_BYTES)
	{
		printf("Error: couldn't find sub string in length of %d", NUMBER_OF_BYTES);
		exit(1);
	}
}

void run_encryption(char *text, char *key, char *text_encrypted)
{
	// XORing bitwise
	for (int i = 0; i < strlen(key); i++)
	{
		text_encrypted[i] = text[i] ^ key[i];
	}
	text_encrypted[strlen(key)] = '\0';
}

static void write_encryption_to_file(char *text_encrypted)
{
	FILE *p_result_file = NULL;
	char path[] = "Encrypted_message.txt";
	p_result_file = (g_offset == 0) ? fopen(path, "w") : fopen(path, "a");
	if (p_result_file == NULL)
	{
		printf("Error: failed opening Encrypted_message.txt. \n");
		exit(1);
	}

	fseek(p_result_file, g_offset, SEEK_SET);
	fprintf(p_result_file, "%s", text_encrypted);
	fclose(p_result_file);
}

static void close_files(void)
{
	fclose(pg_message_file);
	fclose(pg_key_file);
}