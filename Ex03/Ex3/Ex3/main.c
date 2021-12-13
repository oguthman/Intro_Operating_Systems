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

#define THREAD_ASSERT(cond, msg, ...)														\
	do {																					\
		if (!(cond)) {																		\
			printf("Thread Assertion failed at file %s line %d: \n", __FILE__, __LINE__);	\
			printf(msg, __VA_ARGS__);														\
			return 1;																		\
		}																					\
	} while (0);

#define PAGE_SIZE 12		//bits
#define THREAD_TIMEOUT				5000	// 5 seconds
#define WAIT_FOR_THREADS_TIMEOUT	500		// 500ms
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

static int32_t g_clock = -1;
static s_node* gp_clocks_queue = NULL;
static bool g_thread_exit = false;
static File* gp_output_file = NULL;

static HANDLE g_mutex_critical_section = NULL;
static HANDLE g_semaphore = NULL;

/************************************
*      static functions             *
************************************/
static void init(int argc, char* argv[]);
static void parse_arguments(int argc, char* argv[]);
static bool read_file_input(File* file, s_request* request);
static DWORD WINAPI thread_routine(LPVOID lpParam);
static HANDLE create_new_thread(LPTHREAD_START_ROUTINE p_start_routine, LPVOID p_thread_parameters);
static bool wait_for_thread(HANDLE* handles, int number_of_active_handles);
static void close_handles(HANDLE* handles, uint32_t number_of_active_handles);
static void free_queue_items(s_node* queue);
static void exit_protocol(HANDLE* handles, uint32_t number_of_active_handles, File* input_file, File* output_file, s_node* head);
static HANDLE create_mutex();
static HANDLE create_semaphore(int init_count, int max_count);

/************************************
*       API implementation          *
************************************/
/// Description: open treads by requests from input file and running the clock. 
/// Parameters: 
///		[in] argc - number of arguments. 
///		[in] argv - arguments list. 
/// Return: true if succeeded or false otherwise.
int main(int argc, char* argv[])
{
	init(argc, argv);

	s_request request;
	uint32_t count = 1;
	s_node* requests_queue = NULL;
	HANDLE* handles = NULL;

	// open new thread for each line in input file
	while (read_file_input(gs_argument_inputs.input_file, &request))
	{
		// protect from memory leack
		s_request* temp_request = malloc(sizeof(s_request));
		HANDLE* temp_handle = realloc(handles, sizeof(HANDLE) * count);

		if (temp_request == NULL || temp_handle == NULL)
		{
			printf("Error: failed reallocating memory");

			exit_protocol(handles, count - 1, gs_argument_inputs.input_file, gp_output_file, requests_queue);
			exit(1);
		}

		memcpy(temp_request, &request, sizeof(request));
		queue_push(&requests_queue, temp_request);
		handles = temp_handle;

		// add request time to time queue
		queue_priority_push(&gp_clocks_queue, (void*)request.time, request.time, true);

		// create threads
		HANDLE handle = create_new_thread(thread_routine, temp_request);
		if (handle == NULL)
		{
			printf("Error: failed creating a thread\nReceived NULL pointer\n");
			exit_protocol(handles, count - 1, gs_argument_inputs.input_file, gp_output_file, requests_queue);
			exit(1);
		}
		handles[count - 1] = handle;
		Sleep(10);
		count++;
	}

	// run the clock - get next clk from queue
	while (!queue_is_empty(&gp_clocks_queue))
	{
		// wait for thread to finish and signal the semaphore
		// if thread is waiting for free frame, continue to the next clock after WAIT_FOR_THREADS_TIMEOUT.
		DWORD code = WaitForSingleObject(g_semaphore, WAIT_FOR_THREADS_TIMEOUT);
		if (code == WAIT_OBJECT_0 || code == WAIT_TIMEOUT)
		{
			g_clock = (int32_t)queue_pop(&gp_clocks_queue);
			printf("g_clock - %d\n", g_clock);
		}
		else
		{
			printf("Error: failed waiting for thread\n");
			exit_protocol(handles, count - 1, gs_argument_inputs.input_file, gp_output_file, requests_queue);
			exit(1);
		}
		db_print_tables();
	}

	// wait for threads
	bool status = wait_for_thread(handles, count - 1);

	clear_all_frames();
	db_print_tables();
	exit_protocol(handles, count - 1, gs_argument_inputs.input_file, gp_output_file, requests_queue);

	return status ? 0 : 1;
}

/************************************
* static implementation             *
************************************/
/// Description: initialize all relevant variables and modules.
/// Parameters: 
///		[in] argc - number of arguments. 
///		[in] argv - arguments list. 
/// Return: none.
static void init(int argc, char* argv[])
{
	parse_arguments(argc, argv);
	gp_output_file = File_Open("Output.txt", "w");
	ASSERT(gp_output_file != NULL, "Error: failed opening output file\n");
	db_init(gs_argument_inputs.virtual_memory_size, gs_argument_inputs.physical_memory_size, &g_clock, gp_output_file);
	g_mutex_critical_section = create_mutex();
	g_semaphore = create_semaphore(1, 1);
}

/// Description: parse arguments and open files.  
/// Parameters: 
///		[in] argc - number of arguments. 
///		[in] argv - arguments list. 
/// Return: none.
static void parse_arguments(int argc, char* argv[])
{
	// check if there are enough arguments
	ASSERT(argc == 4, "Error: not enough arguments.\n");

	// parse arguments
	gs_argument_inputs.virtual_memory_size = 1 << (strtol(argv[1], NULL, 10) - PAGE_SIZE);
	gs_argument_inputs.physical_memory_size = 1 << (strtol(argv[2], NULL, 10) - PAGE_SIZE);
	gs_argument_inputs.input_file = File_Open(argv[3], "r");
	ASSERT(gs_argument_inputs.input_file != NULL, "Error: Can't open input file\n");
}

/// Description: open file, read line from file and create request using line parameters.  
/// Parameters: 
///		[in] file - pointer to a file.
///		[out] request - pointer to request list.
/// Return: true if read line and write request succeeded or false otherwise.
static bool read_file_input(File* file, s_request* request)
{
	int max_length = 20;
	char line[20];

	return	(File_ReadLine(file, line, max_length) &&
			(sscanf(line, "%d %d %d", &(request->time), &(request->virtual_address), &(request->time_of_use)) != 0));
}

/// Description: find the correct frame to place the new page.
	// 0. check if global clock >= request time, if so continue. otherwise wait.
	// 1. if page is already inside a frame - update end of use of virtual/physical memoey.
	// 2. if page doesn't exist in frame and there is a frame available - update page parameters.
	// 3. if page doesn't exist and  there is frame in eou - replace page in frame. clear the old frame and update with new page parameters.
	// 4. if page doesn't exist and all frames in use, wait.  
/// Parameters: 
///		[in] lpParam - parameters of new page request.
/// Return: DWORD 0 if thread routine succeeded or 1 otherwise.
static DWORD WINAPI thread_routine(LPVOID lpParam)
{
	s_request* request = (s_request*)lpParam;
	
	// check if there are relevant threads to start routine
	while (g_clock < (int32_t)request->time);
	
	printf("request after while - %d, %d, %d, clock %d\n", request->time, request->virtual_address, request->time_of_use, g_clock);

	int page_number = (request->virtual_address) >> PAGE_SIZE;

	// enter critical section
	THREAD_ASSERT(WaitForSingleObject(g_mutex_critical_section, THREAD_TIMEOUT) == WAIT_OBJECT_0, "Error: failed waiting for mutex (#%d)\n", request->time);

	bool page_in_frame = is_page_in_frame(page_number);
	if (!page_in_frame)
		THREAD_ASSERT(ReleaseMutex(g_mutex_critical_section) == true, "Error: failed releasing mutex\n");

	// check if relevant page is already inside a frame
	if (page_in_frame)
	{
		update_frame_eou(page_number, request->time_of_use);
		THREAD_ASSERT(ReleaseMutex(g_mutex_critical_section) == true, "Error: failed releasing mutex\n");

		// add g_clock + request->time_of_use to clocks queue
		uint32_t new_time_eou = request->time_of_use + g_clock;
		queue_priority_push(&gp_clocks_queue, (void*)new_time_eou, new_time_eou, true);

		ReleaseSemaphore(g_semaphore, 1, NULL);
		return 0;
	}

	// find relevant frame for new page. If there isn't, wait.
	bool succeed = false;
	do
	{
		// enter critical section
		THREAD_ASSERT(WaitForSingleObject(g_mutex_critical_section, THREAD_TIMEOUT) == WAIT_OBJECT_0, "Error: failed waiting for mutex (#%d)\n", request->time);
		succeed = try_find_free_frame(page_number, request->time_of_use);
		THREAD_ASSERT(ReleaseMutex(g_mutex_critical_section) == true, "Error: failed releasing mutex\n");
		
		// prevent starvation
		Sleep(5);
	} while (!succeed);
	
	// add g_clock + request->time_of_use to clocks queue
	uint32_t new_time_eou = request->time_of_use + g_clock;
	queue_priority_push(&gp_clocks_queue, (void*)new_time_eou, new_time_eou, true);

	ReleaseSemaphore(g_semaphore, 1, NULL);
	return 0;
}

/// Description: create new thread.  
/// Parameters: 
///		[in] p_start_routine - thread function. 
///		[in] p_thread_parameters - parametes for thread function.
/// Return: thread_handle - handle for the new thread.
static HANDLE create_new_thread(LPTHREAD_START_ROUTINE p_start_routine, LPVOID p_thread_parameters)
{
	HANDLE thread_handle;
	DWORD thread_id;

	if (p_start_routine == NULL)
	{
		printf("Error: failed creating a thread\nReceived NULL pointer\n");
		return NULL;
	}

	thread_handle = CreateThread(
		NULL,                /*  default security attributes */
		0,                   /*  use default stack size */
		p_start_routine,     /*  thread function */
		p_thread_parameters, /*  argument to thread function */
		0,                   /*  use default creation flags */
		&thread_id);         /*  returns the thread identifier */

	return thread_handle;
}

/// Description: wait for running threads to finish.  
/// Parameters: 
///		[in] handles - handles array of running threads. 
///		[in] number_of_active_handles - count of running threads. 
/// Return: true - thread ended successfully, false - otherwise.
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
				// close all handles in the end of main()
			}

			// Wait a few milliseconds for the process to terminate.
			Sleep(10);
			return false;
		}
		default:
		{
			printf("WaitForMultipleObject has returned with code %d. The program will exit\n", threads_status);
			return false;
		}
	}
}

/// Description: close all thread handles.  
/// Parameters: 
///		[in] handles - handles array of running threads. 
///		[in] number_of_active_handles - count of runnig threads. 
/// Return: none.
static void close_handles(HANDLE* handles, uint32_t number_of_active_handles)
{
	for (uint32_t i = 0; i < number_of_active_handles; i++)
		CloseHandle(handles[i]);
}

/// Description: free all nodes in linked list.  
/// Parameters: 
///		[in] queue - pointer to queue list. 
/// Return: none.
static void free_queue_items(s_node* queue)
{
	while (!queue_is_empty(&queue))
	{
		free(queue_pop(&queue));
	}
}

/// Description: exit protocol - close open filse and free handles and memory allocations.  
/// Parameters: 
///		[in] handles - handles array of running threads. 
///		[in] number_of_active_handles - count of runnig threads. 
///		[in] input_file - pointer to input file. 
///		[in] output_file - pointer to output file. 
///		[in] head - pointer to head of linked list. 
/// Return: none.
static void exit_protocol(HANDLE* handles, uint32_t number_of_active_handles, File* input_file, File* output_file, s_node* head) {
	//close files and handles, free memory
	File_Close(input_file);
	File_Close(output_file);
	close_handles(handles, number_of_active_handles);
	free_queue_items(head);
	free(handles);
}

static HANDLE create_mutex()
{
	/* Create the mutex that will be used to synchronize access to critical section */
	HANDLE mutex = CreateMutex(
		NULL,	/* default security attributes */
		FALSE,	/* initially not owned */
		NULL);	/* unnamed mutex */
	
	ASSERT(mutex != NULL, "Error: failed creating mutex: %d\n", GetLastError());
	return mutex;
}

static HANDLE create_semaphore(int init_count, int max_count)
{
	/* Create the mutex that will be used to synchronize access to critical section */
	HANDLE semaphore = CreateSemaphore(
		NULL,		/* semaphore attributes */
		init_count,	/* initial count */
		max_count,	/* max count */
		NULL);		/* name semaphore */

	ASSERT(semaphore != NULL, "Error: failed creating semaphore: %d\n", GetLastError());
	return semaphore;
}
