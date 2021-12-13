#define _CRT_SECURE_NO_WARNINGS
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
#include "queue.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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

static File* gp_output_file;
static uint32_t g_virtual_memory_size;
static uint32_t g_physical_memory_size;
static uint32_t* gp_clock;
static s_node_lru* gp_LRU_queue = NULL;

/************************************
*      static functions             *
************************************/
static void update_tables(uint32_t page_number, uint32_t time_of_use, uint32_t frame_number, bool print_to_output);
static void clear_frame(uint32_t page_number);

/************************************
*       API implementation          *
************************************/
void db_init(uint32_t virtual_memory_size, uint32_t physical_memory_size, uint32_t *clock, File* p_output_file)
{
	gp_output_file = p_output_file;
	gp_clock = clock;
	g_virtual_memory_size = virtual_memory_size;
	g_physical_memory_size = physical_memory_size;
	gp_page_table = malloc(g_virtual_memory_size * sizeof(s_virtual));
	gp_frame_table = malloc(g_physical_memory_size * sizeof(s_pysical));

	if (gp_page_table == NULL || gp_frame_table == NULL)
	{
		printf("db allocation failed\n");
		exit(1);
	}

	memset((uint8_t*)gp_page_table, 0, g_virtual_memory_size * sizeof(s_virtual));
	memset((uint8_t*)gp_frame_table, 0, g_physical_memory_size * sizeof(s_pysical));
}

bool is_page_in_frame(uint32_t page_numbe)
{
	return (gp_page_table[page_numbe].valid);
}

void update_frame_eou(uint32_t page_numbe, uint32_t time_of_use)
{
	update_tables(page_numbe, time_of_use, gp_page_table[page_numbe].frame_number, false);
}

bool try_find_free_frame(uint32_t page_number, uint32_t time_of_use)
{
	// if 2 or more threads are running with the same page number, update max eou
	if (is_page_in_frame(page_number))
	{
		update_frame_eou(page_number, time_of_use);
		return true;
	}
	
	// search for frame with valid == 0
	for (uint32_t frame_number = 0; frame_number < g_physical_memory_size; frame_number++)
	{
		if (!gp_frame_table[frame_number].valid)
		{
			update_tables(page_number, time_of_use, frame_number, true);
			return true;
		}
	}
	
	// check if page in frame reached eou
	uint32_t page_to_clear;
	if (queue_lru_pop_available_page(&gp_LRU_queue, *gp_clock, &page_to_clear))
	{
		uint32_t frame_to_clear = gp_page_table[page_to_clear].frame_number;
		clear_frame(page_to_clear);
		update_tables(page_number, time_of_use, frame_to_clear, true);
		return true;
	}
	
	// no frame available
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
	
	// free gp_LRU_queue
	while (!queue_lru_is_empty(&gp_LRU_queue))
		queue_lru_pop(&gp_LRU_queue);
}

void db_print_tables()
{
	printf("Frame Table:\n");
	for (uint32_t i = 0; i < g_physical_memory_size; i++)
	{
		printf("page number {%d}, valid {%d}, eou {%d}\n", gp_frame_table[i].page_number, gp_frame_table[i].valid, gp_frame_table[i].end_of_use);
	}

	printf("Page Table:\n");
	for (uint32_t i = 0; i < g_virtual_memory_size; i++)
	{
		printf("frame number {%d}, valid {%d}, eou {%d}\n", gp_page_table[i].frame_number, gp_page_table[i].valid, gp_page_table[i].end_of_use);
	}
}

/************************************
* static implementation             *
************************************/
static void update_tables(uint32_t page_number, uint32_t time_of_use, uint32_t frame_number, bool print_to_output)
{
	// update physical memory
	gp_frame_table[frame_number].page_number = page_number;
	gp_frame_table[frame_number].valid = true;
	gp_frame_table[frame_number].end_of_use = max(gp_frame_table[frame_number].end_of_use, *gp_clock + time_of_use);

	// update virtual memory
	gp_page_table[page_number].frame_number = frame_number;
	gp_page_table[page_number].valid = true;
	gp_page_table[page_number].end_of_use = max(gp_page_table[page_number].end_of_use, *gp_clock + time_of_use);

	// add to LRU linked list
	queue_lru_push(&gp_LRU_queue, page_number, gp_page_table[page_number].end_of_use);

	// update output file
	if (print_to_output) 
	{
		char line[50];
		sprintf(line, "%d %d %d %s\n", *gp_clock, page_number, frame_number, "P");
		File_Write(gp_output_file, line, (int)strlen(line));
	}
}

static void clear_frame(uint32_t page_number)
{
	uint32_t frame_number = gp_page_table[page_number].frame_number;
	
	// clear virtual memory
	gp_page_table[page_number].frame_number = 0;
	gp_page_table[page_number].valid = false;
	gp_page_table[page_number].end_of_use = 0;

	// update output file
	char line[50];
	sprintf(line, "%d %d %d %s\n", *gp_clock, page_number, frame_number, "E");
	File_Write(gp_output_file, line, (int)strlen(line));
}