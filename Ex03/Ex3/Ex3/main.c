#define _CRT_SECURE_NO_WARNINGS
/*!
******************************************************************************
\file main.c
\date 26 November 2021
\authors Shahar Dorit Morag 315337238 & Ofir Guthman 205577018
\project #3
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
#include "db.h"
#include "FileApi/file.h"
#include "queue.h"

#include <string.h>
#include <Windows.h>
#include <stdio.h>

/************************************
*      definitions                 *
************************************/
#define ASSERT(cond, msg, ...)														\
	do {																			\
		if (!(cond)) {																\
			printf("Assertion failed at file %s line %d: \n", __FILE__, __LINE__);	\
			printf(msg, __VA_ARGS__);												\
			exit (1);																\
		}																			\
	} while (0);	

#define SIZE_OF_PAGE 12		//bits
#define THREAD_TIMEOUT	500000	// 5 seconds
#define TERMINATE_ALL_THREADS_EXITCODE 0x55
/************************************
*       types                       *
************************************/
typedef struct
{
	uint32_t time;
	uint32_t virtual_address;
	uint32_t time_of_use;
} s_request;

/************************************
*      variables                    *
************************************/
static struct {
	uint32_t virtual_memory_size;
	uint32_t physical_memory_size;
	File* input_file;
} gs_argument_inputs;

static uint32_t *g_clocks_array;
static uint32_t g_clock;
static s_node *g_clocks_queue = NULL;

/************************************
*      static functions             *
************************************/
static void parse_arguments(int argc, char* argv[]);
static bool read_file_input(File* file, s_request* request);
static DWORD WINAPI thread_routine(LPVOID lpParam);
static HANDLE create_new_thread(LPTHREAD_START_ROUTINE p_start_routine, LPVOID p_thread_parameters);
static bool wait_for_thread(HANDLE* handles, int number_of_active_handles);
static void close_handles(HANDLE* handles, int number_of_active_handles);

/************************************
*       API implementation          *
************************************/
// create virtual db for pages
// create physical db for frame
// create clk - time is updated per request

// create output file
// init loop and check if there are no requests and all threads a finished.
// read file - get request
// get time
// 
// for every line in file create thread (request)
// go to physical db to check which frame is available
// when availabe, update output: time, page, frame,output (p/e). virtual: frame, valid, endofuse. physical: the new change.

// when loop is done: clear all frames and update output by index - from lower to higher

// free handles
// close files
int main(int argc, char* argv[])
{
	parse_arguments(argc, argv);
	db_init(gs_argument_inputs.virtual_memory_size, gs_argument_inputs.physical_memory_size, &g_clock);

	s_request request;
	uint32_t count = 1;
	s_request* requests = NULL;
	HANDLE* handles = NULL;

	// open new thread for each line in input file
	while(read_file_input(gs_argument_inputs.input_file, &request))
	{
		// protect from memory leack
		s_request* temp_request = realloc(requests, sizeof(s_request) * count);
		HANDLE *temp_handle = realloc(handles, sizeof(HANDLE) * count);

		if (temp_request == NULL || temp_handle == NULL)
		{
			printf("Error: failed reallocating memory");
			
			// TODO: exit protocol
			File_Close(gs_argument_inputs.input_file);
			close_handles(handles, count - 1);
			free(requests);
			free(handles);
			exit(1);
		}
		else
		{
			requests = temp_request;
			handles = temp_handle;
		}

		requests[count - 1] = request;

		// add request time to time queue
		queue_push(&g_clocks_queue, request.time);
		printf("request - %d, %d, %d\n", request.time, request.virtual_address, request.time_of_use);

		// thread_routine();
		handles[count - 1] = create_new_thread(thread_routine, &requests[count - 1]);

		count++;
	}

	//for (int i = 0; i < count-1; i++)
	//{
	//	printf("request - %d, %d, %d\n", requests[i].time, requests[i].virtual_address, requests[i].time_of_use);
	//}

	// running the clock - get next clk from queue
	while (!queue_is_empty(&g_clocks_queue))		
	{
		g_clock = queue_pop(&g_clocks_queue);
		printf("g_clock - %d\n", g_clock);

		// TODO: waiting for some event 
		// wait enough tome for thread to finish routine
	}
	
	// wait for threads
	bool status = wait_for_thread(handles, count-1);

	//TODO: nput file

	clear_all_frames();

	// close files
	File_Close(gs_argument_inputs.input_file);
	
	// close handles
	close_handles(handles, count - 1);

	// free
	free(requests);
	free(handles);

	return status ? 0 : 1;
}

/************************************
* static implementation             *
************************************/
/// Description: parse arguments and open files.  
/// Parameters: 
///		[in] argc - number of arguments. 
///		[in] argv - arguments list. 
/// Return: none.
static void parse_arguments(int argc, char* argv[])
{
	// Check if there are enough arguments
	ASSERT(argc == 4, "Error: not enough arguments.\n");

	// Parse arguments
	gs_argument_inputs.virtual_memory_size = strtol(argv[1], NULL, 10);
	gs_argument_inputs.physical_memory_size = strtol(argv[2], NULL, 10);
	gs_argument_inputs.input_file = File_Open(argv[3], "r");
	ASSERT(gs_argument_inputs.input_file != NULL, "Error: Can't open input file\n");
}

/// Description: Open file, read line from file and create request using line parameters.  
/// Parameters: 
///		file - pointer to a file 
///		grade - pointer to grade
/// Return: bool if read grade succeeded or false otherwise
static bool read_file_input(File* file, s_request* request)
{
	int max_length = 20;
	char line[20];

	return	(File_ReadLine(file, line, max_length) &&
			(sscanf(line, "%d %d %d", &(request->time), &(request->virtual_address), &(request->time_of_use)) != 0));
}
//choose the correct frame to place the new page
static DWORD WINAPI thread_routine(LPVOID lpParam)
{
	// 0. check if g_clock >= time, if so continue. otherwise wait.
	// 1. if page is already inside a frame - update end of use of virtual/physical
	// 2. if page doesnt exist and there is a free frame - update valid, frame number, eou
	// 3. if page doesnt exist and frame in eou - replace page in frame. update frame number, eou
	// 4. if page doesnt exist and frames in use, wait.
	
	s_request* request = (s_request*)lpParam;

	// TODO: decide if we want to change the while to semaphore
	// check if there are relevant threads to start routine
	while (request->time < g_clock);

	int page_number = (request->virtual_address) >> SIZE_OF_PAGE;
	// check if relevant page is already inside a frame
	if (is_page_in_frame(page_number))
	{
		update_frame_eou(page_number, request->time_of_use);
		
		// add g_clock + request->time_of_use to clocks queue
		queue_push(&g_clocks_queue, request->time_of_use + g_clock);
		
		return 0;
	}

	// TODO: decide if we want to change the while to semaphore
	// find relevant frame for new page. If there isn't, wait.
	while(!try_find_free_frame(page_number, request->time_of_use));
	
	// add g_clock + request->time_of_use to clocks queue
	queue_push(&g_clocks_queue, request->time_of_use + g_clock);

}

/// Description: Create new thread.  
/// Parameters: 
///		p_start_routine - thread function. 
///		p_thread_parameters - parametes for thread function.
///		p_thread_id - id of the new thread.
/// Return: thread_handle - handle for the new thread.
static HANDLE create_new_thread(LPTHREAD_START_ROUTINE p_start_routine, LPVOID p_thread_parameters)
{
	HANDLE thread_handle;
	DWORD thread_id;

	ASSERT(p_start_routine != NULL, "Error: failed creating a thread\nReceived NULL pointer\n");

	thread_handle = CreateThread(
		NULL,                /*  default security attributes */
		0,                   /*  use default stack size */
		p_start_routine,     /*  thread function */
		p_thread_parameters, /*  argument to thread function */
		0,                   /*  use default creation flags */
		&thread_id);         /*  returns the thread identifier */

	return thread_handle;
}

/// Description: Handle wait for processes.  
/// Parameters: 
///		[in] p_procinfo - process information. 
/// Return: true - process ended successfully, false - otherwise.
static bool wait_for_thread(HANDLE* handles, int number_of_active_handles)
{
	DWORD threads_status = WaitForMultipleObjects(
		number_of_active_handles,		// number of arguments in handles array
		handles,						// pointer to handles array
		true,							// rather wait for all handles to finish or not
		THREAD_TIMEOUT);				// how much time in msec to wait for first handle to finish

	if (threads_status >= WAIT_OBJECT_0 && threads_status < WAIT_OBJECT_0 + number_of_active_handles)
		return true;
	switch (threads_status)
	{
		case WAIT_TIMEOUT:
		{
			printf("All running threads didn't finish after %d ms. Terminationg all thereads\n", THREAD_TIMEOUT);

			// Terminate all running threads
			for (int i = 0; i < number_of_active_handles; i++)
			{
				TerminateThread(handles[i], TERMINATE_ALL_THREADS_EXITCODE);
				CloseHandle(handles[i]);
			}

			// Wait a few milliseconds for the process to terminate.
			Sleep(10);
			return false;
		}
		default:
		{
			printf("WaitForMultipleObject has returned with code %d. The program will exit\n", single_thread_status);
			return false;
		}
	}
}

static void close_handles(HANDLE* handles, int number_of_active_handles)
{
	for (int i = 0; i < number_of_active_handles; i++)
		CloseHandle(handles[i]);
}
