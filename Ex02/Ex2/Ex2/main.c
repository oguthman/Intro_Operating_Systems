/*!
******************************************************************************
\file main.c
\date 9 November 2021
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
#include <windows.h>
#include "thread.h"


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
	// Open new dir 'Result'
	HANDLE handle = OpenNewThread(2);

	WaitForSingleObject(handle, 5000);

	CloseHandle(handle);

	// Loop on number of schools and open new thread for each.
	// Thread limitation for max 10 threads.


	// close all handles.

}

/************************************
* static implementation             *
************************************/
