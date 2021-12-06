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

/************************************
*      static functions             *
************************************/
static void parse_arguments(int argc, char* argv[]);
static bool read_file_input(File* file, s_request* request);

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
	//parse_arguments(argc, argv);
	//db_init(gs_argument_inputs.virtual_memory_size, gs_argument_inputs.physical_memory_size, &g_clock);

	//s_request request;
	////while()		// creating threads
	//{
	//	ASSERT(read_file_input(gs_argument_inputs.input_file, &request), "Couldn't read line from a file\n");
	//	// realloc g_clocks_array
	//	// g_clocks_array[i] = request.time
	//	// thread_routine();
	//}

	//while (g_clocks_array.length)		// running the clock
	//{
	//	g_clock = g_clocks_array[i];
		
	//	wait enough tome for thread to finish routine
	//}
	//clear_all_frames();

	s_node* head = NULL;
	queue_push(&head, 4);
	queue_push(&head, 2);
	queue_push(&head, 1);
	printf("pop value: %d\n", queue_pop(&head));
	queue_push(&head, 3);
	printf("pop value: %d\n", queue_pop(&head));
	printf("pop value: %d\n", queue_pop(&head));
	printf("pop value: %d\n", queue_pop(&head));
	return 0;
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
}

/// Description: Open file, read line from file and create request using line parameters.  
/// Parameters: 
///		file - pointer to a file 
///		grade - pointer to grade
/// Return: bool if read grade succeeded or false otherwise
static bool read_file_input(File* file, s_request* request)
{
	int max_length = 50;
	char* line[50];

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
		// add g_clock + request->time_of_use to g_clocks_array
		return 0;
	}

	// TODO: decide if we want to change the while to semaphore
	// find relevant frame for new page. If there isn't, wait.
	while(!try_find_free_frame(page_number, request->time_of_use));
	// add g_clock + request->time_of_use to g_clocks_array

}