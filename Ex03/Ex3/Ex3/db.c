/*!
******************************************************************************
\file db.c
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
#include <string.h>
#include <stdlib.h>

/************************************
*      definitions                 *
************************************/

/************************************
*       types                       *
************************************/

/************************************
*      variables                    *
************************************/
s_virtual *gp_page_table;
s_pysical *gp_frame_table;
static uint32_t g_virtual_memory_size;
static uint32_t g_physical_memory_size;
static uint32_t *g_clock;

/************************************
*      static functions             *
************************************/
static void update_tables(uint32_t page_number, uint32_t time_of_use, uint32_t frame_number);
static void clear_frame(uint32_t page_number);

/************************************
*       API implementation          *
************************************/
void db_init(uint32_t virtual_memory_size, uint32_t physical_memory_size, uint32_t *clock)
{
	g_clock = clock;
	g_virtual_memory_size = virtual_memory_size;
	g_physical_memory_size = physical_memory_size;
	gp_page_table = malloc(g_virtual_memory_size * sizeof(s_virtual));
	gp_frame_table = malloc(g_physical_memory_size * sizeof(s_pysical));
	memset((uint8_t*)gp_page_table, 0, g_virtual_memory_size * sizeof(s_virtual));
	memset((uint8_t*)gp_frame_table, 0, g_physical_memory_size * sizeof(s_pysical));
}

bool is_page_in_frame(uint32_t page_numbe)
{
	return (gp_page_table[page_numbe].valid);
}

void update_frame_eou(uint32_t page_numbe, uint32_t time_of_use)
{
	gp_page_table[page_numbe].end_of_use = time_of_use;
	gp_page_table[page_numbe].valid = true;	//for now- just in case
}

bool try_find_free_frame(uint32_t page_number, uint32_t time_of_use)
{
	for (uint32_t frame_number = 0; frame_number < g_physical_memory_size; frame_number++)
	{
		if (!gp_frame_table[frame_number].valid)
		{
			update_tables(page_number, time_of_use, frame_number);
			return true;
		}
	}
	
	for (uint32_t frame_number = 0; frame_number < g_physical_memory_size; frame_number++)
	{
		//TODO: LRU implementation
		if (!gp_frame_table[frame_number].end_of_use < *g_clock)
		{
			clear_frame(page_number);
			update_tables(page_number, time_of_use, frame_number);
			return true;
		}
	}
	
	return false;
}

void clear_all_frames()
{
	for (uint32_t frame_number = 0; frame_number < g_physical_memory_size; frame_number++) 
	{
		if (gp_frame_table[frame_number].valid)
		{
			clear_frame(gp_frame_table[frame_number].page_number);
		}
	}
}

/************************************
* static implementation             *
************************************/
static void update_tables(uint32_t page_number, uint32_t time_of_use, uint32_t frame_number)
{
	// update physical memory
	gp_frame_table[frame_number].page_number = page_number;
	gp_frame_table[frame_number].valid = true;
	gp_frame_table[frame_number].end_of_use = g_clock + time_of_use;

	// update virtual memory
	gp_page_table[page_number].frame_number = frame_number;
	gp_page_table[page_number].valid = true;
	gp_page_table[page_number].end_of_use = g_clock + time_of_use;
}

static void clear_frame(uint32_t page_number)
{
	// clear virtual memory
	gp_page_table[page_number].frame_number = 0;
	gp_page_table[page_number].valid = false;
	gp_page_table[page_number].end_of_use = 0;

	//TODO: update output file
}