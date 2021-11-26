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
//
#include <string.h>

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

/************************************
*      static functions             *
************************************/
static void parse_arguments(int argc, char* argv[]);
bool read_file_input(File* file, s_request* request);

/************************************
*       API implementation          *
************************************/
// create virtual db for pages
// create physical db for frame
// create clk - time is updated per request
// create output file

// init loop and check if the are no requests and all threads a finished.
// read file - get request
// get time
// 
// for every line in file create thread (request)
// go to physical db to check which frame is available
// when availabe, update output: time, page, frame,output (p/e). virtual: frame, valid, endofuse. physical: the new change.

// when loop is done: update output by index - from lower to higher

// free handles
// close files
int main(int argc, char* argv[])
{
	parse_arguments(argc, argv);
	db_init(gs_argument_inputs.virtual_memory_size, gs_argument_inputs.physical_memory_size);

	//
	s_request request;
	read_file_input(gs_argument_inputs.input_file, &request);



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

/// Description: Open file.  
/// Parameters: 
///		file - pointer to a file 
///		grade - pointer to grade
/// Return: bool if read grade succeeded or false otherwise
bool read_file_input(File* file, s_request* request)
{
	int max_length = 50;
	char* line[50];

	return	(File_ReadLine(file, line, max_length) &&
			(sscanf(line, "%d %d %d", &(request->time), &(request->virtual_address), &(request->time_of_use)) != 0));
}
