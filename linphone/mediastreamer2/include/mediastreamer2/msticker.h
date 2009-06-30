/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2006  Simon MORLAT (simon.morlat@linphone.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/


#ifndef MS_TICKER_H
#define MS_TICKER_H


#include "msfilter.h"
#include "mscommon.h"

/**
 * @file msticker.h
 * @brief mediastreamer2 msticker.h include file
 *
 * This file provide the API needed to create, start
 * and stop a graph.
 *
 */

/**
 * @defgroup mediastreamer2_ticker Ticker API - manage mediastreamer2 graphs.
 * @ingroup mediastreamer2_api
 * @{
 */


/**
 * Structure for method getting time in miliseconds from an external source.
 * @var MSTickerTimeFunc
 */
typedef uint64_t (*MSTickerTimeFunc)(void *);

struct _MSTicker
{
	ms_mutex_t lock;
	ms_cond_t cond;
	MSList *execution_list;     /* the list of source filters to be executed.*/
	ms_thread_t thread;   /* the thread ressource*/
	int interval; /* in miliseconds*/
	int exec_id;
	uint32_t ticks;
	uint64_t time;	/* a time since the start of the ticker expressed in milisec*/
	uint64_t orig; /* a relative time to take in account difference between time base given by consecutive get_cur_time_ptr() functions.*/
	MSTickerTimeFunc get_cur_time_ptr;
	void *get_cur_time_data;
	char *name;
	bool_t run;       /* flag to indicate whether the ticker must be run or not */
#ifdef WIN32_TIMERS
	HANDLE TimeEvent;
#endif
};

/**
 * Structure for ticker object.
 * @var MSTicker
 */
typedef struct _MSTicker MSTicker;


#ifdef __cplusplus
extern "C"{
#endif


/**
 * Create a ticker that will be used to start
 * and stop a graph.
 *
 * Returns: MSTicker * if successfull, NULL otherwise.
 */
MSTicker *ms_ticker_new(void);

/**
 * Set a name to the ticker (used for logging)
**/
void ms_ticker_set_name(MSTicker *ticker, const char *name);

/**
 * Attach a chain of filters to a ticker.
 * The processing chain will be executed until ms_ticker_detach
 * will be called.
 *
 * @param ticker  A #MSTicker object.
 * @param f       A #MSFilter object.
 *
 * Returns: 0 if successfull, -1 otherwise.
 */
int ms_ticker_attach(MSTicker *ticker,MSFilter *f);

/**
 * Dettach a chain of filters to a ticker.
 * The processing chain will no more be executed.
 *
 * @param ticker  A #MSTicker object.
 * @param f  A #MSFilter object.
 *
 *
 * Returns: 0 if successfull, -1 otherwise.
 */
int ms_ticker_detach(MSTicker *ticker,MSFilter *f);

/**
 * Destroy a ticker.
 *
 * @param ticker  A #MSTicker object.
 *
 */
void ms_ticker_destroy(MSTicker *ticker);

/**
 * Override MSTicker's time function.
 * This can be used to control the ticker from an external time provider, for example the 
 * clock of a sound card.
 *
 * @param ticker  A #MSTicker object.
 * @param func    A replacement method for calculating "current time"
 * @param user_data Any pointer to user private data.
 */
void ms_ticker_set_time_func(MSTicker *ticker, MSTickerTimeFunc func, void *user_data);

/**
 * Print on stdout all filters of a ticker. (INTERNAL: DO NOT USE)
 *
 * @param ticker  A #MSTicker object.
 */
void ms_ticker_print_graphs(MSTicker *ticker);

/* private functions:*/

#ifdef __cplusplus
}
#endif

/** @} */

#endif
