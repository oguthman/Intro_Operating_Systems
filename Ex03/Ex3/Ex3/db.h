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
s_virtual* gp_page_table;
s_pysical* gp_frame_table;


void db_init(uint32_t virtual_memory_size, uint32_t physical_memory_size, uint32_t* clock);
bool is_page_in_frame(uint32_t page_numbe);
void update_frame_eou(uint32_t page_numbe, uint32_t time_of_use);
bool try_find_free_frame(uint32_t page_number, uint32_t time_of_use);
void clear_all_frames();

/*!
******************************************************************************
\brief
Initialize func 

\details
Must be called only once

\param
 [in] counter_val - reset counter value
 [out] out_val    - 

\return none
*****************************************************************************/


#endif //__DB_H__