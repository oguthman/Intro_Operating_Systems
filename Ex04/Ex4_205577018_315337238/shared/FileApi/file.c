#define _CRT_SECURE_NO_WARNINGS
/*!
******************************************************************************
\file file.c
\date 24 October 2021
\authors Shahar Dorit Morag 315337238 & Ofir Guthman 205577018
\project #
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
#include <stdbool.h>
//
#include "file.h"

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
static bool parse_mode(char* mode, DWORD* access, DWORD* disposition, DWORD* share);

/************************************
*       API implementation          *
************************************/
File* File_Open(char* fileName, char* mode)
{
    DWORD access = 0, disposition = 0, share = 0;
    if (!parse_mode(mode, &access, &disposition, &share))
    {
        printf("Error: File mode(%s) is not valid", mode);
        return NULL;
    }
    
    // allocate variables.
    int length = (int)(strlen(fileName) + 1);
    wchar_t* file_name = malloc((length) * sizeof(wchar_t));
    File* source = (File*)malloc(sizeof(File));

    if (file_name == NULL || source == NULL)
    {
        printf("Error: failed allocating memory \n");
        return NULL;
    }

    // cast file name to wchar_t
    swprintf(file_name, length, L"%hs", fileName);

    // Open the source file
    *source = CreateFile(file_name,               // name of the write
                         access,                  // open for writing
                         share,                   // do not share
                         NULL,                    // default security
                         disposition,             // create new file only
                         FILE_ATTRIBUTE_NORMAL,   // normal file
                         NULL);                   // no attr. template

    // free file_name
    free(file_name);

    // Check for error
    if (*source == INVALID_HANDLE_VALUE) {
        printf("Error: Source file not opened. Error %u\n", GetLastError());
        free(source);
        return NULL;
    }

    return source;
}

void File_Close(File* file)
{
    CloseHandle(*file);
    free(file);
}

int File_Read(File* file, char* buffer, int count)
{
    DWORD bytesRead;
    // Read file, check for error
    if (!ReadFile(*file, buffer, count - 1, &bytesRead, NULL)) {
        printf("Error: Source file not read from (error %u)", GetLastError());
        return 0;
    }

    return bytesRead;
}

bool File_ReadLine(File* file, char* line, int max_line_length) 
{
    char* buffer = (char*)malloc((max_line_length) * sizeof(char));
    if (buffer == NULL)
    {
        printf("Error: failed allocating memory \n");
        return false;
    }

    int read_len = File_Read(file, buffer, max_line_length);
    buffer[read_len < max_line_length ? read_len : max_line_length - 1] = '\0';
    char* p_line = strtok(buffer, "\n\r");
    if (read_len <= 2 || p_line == NULL)
    {
        //printf("Error: parsing line\n");
        free(buffer);
        return false;
    }
    // cpy line
    strcpy(line, p_line);

    // clean buffer
    free(buffer);
        
    int return_len = (int)(strlen(line) - read_len + 2);
    DWORD succeed = SetFilePointer(*file, return_len, NULL, FILE_CURRENT);
    DWORD pos = SetFilePointer(*file, 0, NULL, FILE_CURRENT);

    if (succeed == INVALID_SET_FILE_POINTER)
    {
        printf("Error: unable to move file pointer (error %d) from [%d] to [%d]\n", succeed, pos, pos - return_len);
        return false;
    }
    return true;
}

bool File_Scanf(File* file, int max_length, char* format, va_list args)
{
    char* line = (char*)malloc((max_length) * sizeof(char));

    if (line == NULL)
    {
        printf("Error: failed allocating memory \n");
        return false;
    }

    if(!File_ReadLine(file, line, max_length) || !sscanf(line, format, args))
    {
        //printf("Error: Read line failed\n");
        free(line);
        return false;
    }
    
    free(line);
    return true;
}

int File_Write(File* file, char* buffer, int count)
{
    DWORD bytesWritten;

    if (!WriteFile(*file, buffer, count, &bytesWritten, NULL)) {
        printf("Error: Target file not written to (error %u)\n", GetLastError());
        return 0;
    }
    return bytesWritten;
}

bool File_Printf(File* file, _Printf_format_string_ char* format, ...) 
{
    va_list _ArgList;
    __crt_va_start(_ArgList, format);

    int length = snprintf(NULL, 0, format, _ArgList) + 1;
    char* buffer = malloc((length) * sizeof(char));
    if (NULL == buffer)
    {
        printf("Error: failed allocating command \n");
        return 0;
    }

    snprintf(buffer, length, format, _ArgList);
    if (!File_Write(file, buffer, (int)strlen(buffer)))
    {
        printf("Error: printf function failed\n");
        free(buffer);
        return false;
    }

    __crt_va_end(_ArgList);
    free(buffer);
    return true;
}

/************************************
* static implementation             *
************************************/
static bool parse_mode(char* mode, DWORD* access, DWORD* disposition, DWORD* share)
{
    *share = 0;
    if (!strcmp(mode, "r"))
    {
        *access = GENERIC_READ;
        *disposition = OPEN_EXISTING;
        *share = FILE_SHARE_READ;
    }
    else if (!strcmp(mode, "w"))
    {
        *access = GENERIC_WRITE;
        *disposition = CREATE_ALWAYS;
    }
    else if (!strcmp(mode, "a"))
    {
        *access = GENERIC_WRITE;
        *disposition = OPEN_EXISTING;
    }
    else
    {
        return false;
    }

    return true;
}

