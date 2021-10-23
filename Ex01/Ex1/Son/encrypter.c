/*!
******************************************************************************
\file encrypter.c
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
#include <string.h>
#include <stdio.h>
#include <assert.h>

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
static char* gEncryptionKey;

/************************************
*       API implementation          *
************************************/
void Encrypter_Init(char* key)
{
	// verify key is not null
	assert(key != NULL);
	//
	gEncryptionKey = key;
}

void Encrypter_RunEncryption(char* text, char* encryptedText)
{
	// verify ptrs are not null
	assert(text != NULL);
	assert(encryptedText != NULL);
	// verify text and key on the same length
	assert(strlen(text) == strlen(gEncryptionKey));
	// 
	// XORing bitwise
	for (int i = 0; i < strlen(gEncryptionKey); i++)
	{
		encryptedText[i] = text[i] ^ gEncryptionKey[i];
	}
	encryptedText[strlen(gEncryptionKey)] = '\0';
}


/************************************
* static implementation             *
************************************/

