/*!
******************************************************************************
\file db.h
\date 26 November 2021
\author Shahar Dorit Morag & Ofir Guthman
\brief 

\details

\par Copyright
(c) Copyright 2021 Ofir & Shahar
\par
ALL RIGHTS RESERVED
*****************************************************************************/

#ifndef __DB_H__
#define __DB_H__

/************************************
*      include                      *
************************************/
#include <stdint.h>
#include <stdbool.h>

#include "FileApi/file.h"

/************************************
*      definitions                 *
************************************/

/************************************
*       types                       *
************************************/
typedef struct {
	uint32_t frame_number;
	bool valid;
	uint32_t end_of_use;
}s_virtual;

typedef struct {
	uint32_t page_number;
	bool valid;
	uint32_t end_of_use;
}s_pysical;

/************************************
*       API                         *
************************************/

/// Description: initialize memory tables and global clock.  
/// Parameters: 
///		[in] virtual_memory_size. 
///		[in] physical_memory_size. 
///		[in] clock. 
///		[in] p_output_file - pointer to output file. 
/// Return: none.
void db_init(uint32_t virtual_memory_size, uint32_t physical_memory_size, uint32_t* clock, File* p_output_file);

/// Description: check if page is already in physical memory (frames).
/// Parameters: 
///		[in] page_numbe. 
/// Return: true - if page in frame or false otherwise.
bool is_page_in_frame(uint32_t page_numbe);

/// Description: update the page end of use both in vitual and physical memory.
/// Parameters: 
///		[in] page_numbe. 
/// Return: none.
void update_frame_eou(uint32_t page_numbe, uint32_t time_of_use);

/// Description: try to find correct frame for new page request.
	// 1. if page is already inside a frame - update end of use of virtual/physical memoey.
	// 2. if page doesn't exist in frame and there is a frame available - update page parameters.
	// 3. if page doesn't exist and frame in eou - replace page in frame. clear the old frame and update with new page parameters.
	// 4. if page doesn't exist and all frames in use, wait. 
/// Parameters: 
///		[in] page_numbe. 
///		[in] time_of_use - end of use of new page request. 
/// Return: true - if frame found anf false otherwise.
bool try_find_free_frame(uint32_t page_number, uint32_t time_of_use);

/// Description: at the end of the process, clear memory and free gp_LRU_queue.
/// Parameters: none.
/// Return: none.
void clear_all_frames();

/// Description: print databases tables to the screen.
/// Parameters: none.
/// Return: none.
void db_print_tables();

#endif //__DB_H__