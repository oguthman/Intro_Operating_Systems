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

/************************************
*      static functions             *
************************************/

/************************************
*       API implementation          *
************************************/
void db_init(uint32_t virtual_memory_size, uint32_t physical_memory_size)
{
	gp_page_table = malloc(virtual_memory_size * sizeof(s_virtual));
	gp_frame_table = malloc(physical_memory_size * sizeof(s_pysical));
	memset((uint8_t*)gp_page_table, 0, sizeof(gp_page_table));
	memset((uint8_t*)gp_frame_table, 0, sizeof(gp_frame_table));
}

/************************************
* static implementation             *
************************************/

