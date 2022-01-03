/*!
******************************************************************************
\file file.h
\date 24 October 2021
\author Shahar Dorit Morag & Ofir Guthman
\brief

\details

\par Copyright
(c) Copyright 2021 Ofir & Shahar
\par
ALL RIGHTS RESERVED
*****************************************************************************/

#ifndef __FILE_H__
#define __FILE_H__

/************************************
*      include                      *
************************************/
#include <Windows.h>
#include <stdbool.h>

/************************************
*      definitions                 *
************************************/
#define FILE_PRINTF(file, format, ...)                                  \
        int length = snprintf(NULL, 0, format, __VA_ARGS__) + 1;        \
        char* buffer = malloc((length) * sizeof(char));                 \
        if (NULL == buffer)                                             \
        {                                                               \
            printf("Error: failed allocating command \n");              \
            return 0;                                                   \
        }                                                               \
        snprintf(buffer, length, format, __VA_ARGS__);                  \
        if (!File_Write(file, buffer, (int)strlen(buffer)))             \
        {                                                               \
            printf("Error: printf function failed\n");                  \
            free(buffer);                                               \
            return false;                                               \
        }                                                               \
        free(buffer);

/************************************
*       types                       *
************************************/
typedef HANDLE File;

/************************************
*       API                         *
************************************/

/*!
******************************************************************************
\brief
 Open new or exsiting file

\details
 open the file according to the mode param.

\param
 [in] filename - file path (can be relative)
 [in] mode - read/write mode (like fopen modes)

\return file handle.
*****************************************************************************/
File* File_Open(char* fileName, char* mode);

/*!
******************************************************************************
\brief
 Close file handle

\details
 release file resources

\param
 [in] file - file handle

\return none.
*****************************************************************************/
void File_Close(File* file);

/*!
******************************************************************************
\brief
 Read data from file

\param
 [in] file - file handle
 [out] buffer - buffer for the received data.
 [in] count - number of byte to read

\return number of bytes which succeed to read.
*****************************************************************************/
int File_Read(File* file, char* buffer, int count);

/*!
******************************************************************************
\brief
 Read line from file

\details
 read the line untill the new line character.

\param
 [in] file - file handle
 [out] line - buffer for the received line.
 [in] max_line_length - max character in line (need to pass more then max expected line length)

\return true if reading succeed.
*****************************************************************************/
bool File_ReadLine(File* file, char* line, int max_line_length);

/*!
******************************************************************************
\brief
 Read data from file with format functionality

\details
 used like regular scanf function. 
 However, in this case you need to pass the maximum expected line length.

\param
 [in] file - file handle
 [in] max_length - max character in line (need to pass more then max expected line length)
 [in] format - format string
 [out] args - format arguments

\return true if reading succeed.
*****************************************************************************/
bool File_Scanf(File* file, int max_length, char* format, va_list args);

/*!
******************************************************************************
\brief
 Write data to file

\param
 [in] file - file handle
 [in] buffer - string to write
 [in] count - buffer length

\return number of bytes which succeed to write.
*****************************************************************************/
int File_Write(File* file, char* buffer, int count);

/*!
******************************************************************************
\brief
 Write data to file with format functionality

\details
 used like regular printf function

\param
 [in] file - file handle
 [in] format - format string
 [in] args - format arguments

\return true if writing succeed.
*****************************************************************************/
bool File_Printf(File* file, _Printf_format_string_ char* format, ...);


#endif //__FILE_H__