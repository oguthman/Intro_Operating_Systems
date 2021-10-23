/*!
******************************************************************************
\file encrypter.h
\date 23 October 2021
\author Shahar Dorit Morag & Ofir Guthman
\brief
 Creating an encription for the incoming data.

\details
 API Summary:
	

\par Copyright
(c) Copyright 2021 Ofir & Shahar
\par
ALL RIGHTS RESERVED
*****************************************************************************/

#ifndef __ENCRYPTER_H__
#define __ENCRYPTER_H__

/************************************
*      include                      *
************************************/

/************************************
*      definitions                 *
************************************/

/************************************
*       types                       *
************************************/


/************************************
*       API                         *
************************************/

/*!
******************************************************************************
\brief
Initialize encrypter module

\details
Must be called only once

\param
 [in] key - the encryption key

\return none
*****************************************************************************/
void Encrypter_Init(char* key);

/*!
******************************************************************************
\brief
Run encryption

\details
 Encypte text by bitwise xor operation with the encryption key.

\param
 [in] text - the text to encryption.
 [out] encryptedText - the text after the encryption.

\return none
*****************************************************************************/
void Encrypter_RunEncryption(char* text, char* encryptedText);

#endif //__ENCRYPTER_H__