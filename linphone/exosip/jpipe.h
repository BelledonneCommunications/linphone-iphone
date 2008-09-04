/*
  eXosip - This is the eXtended osip library.
  Copyright (C) 2002, 2003  Aymeric MOIZARD  - jack@atosc.org
  
  eXosip is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  eXosip is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef _JPIPE_H_
#define _JPIPE_H_

#include <eXosip.h>

#ifndef WIN32
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif

#ifdef WIN32
#include <windows.h>
#endif

/**
 * @file jpipe.h
 * @brief PPL Pipe Handling Routines
 */

/**
 * @defgroup JPIPE Pipe Handling
 * @ingroup PPL
 * @{
 */

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef WIN32

/**
 * Structure for storing a pipe descriptor
 * @defvar jpipe_t
 */
  typedef struct jpipe_t jpipe_t;

  struct jpipe_t
  {
    int pipes[2];
  };

#else

/**
 * Structure for storing a pipe descriptor
 * @defvar ppl_pipe_t
 */
  typedef struct jpipe_t jpipe_t;

  struct jpipe_t
  {
    int pipes[2];
  };

#endif

/**
 * Get New pipe pair.
 */
    jpipe_t * jpipe (void);

/**
 * Close pipe
 */
    int jpipe_close (jpipe_t * apipe);

/**
 * Write in a pipe.
 */
    int jpipe_write (jpipe_t * pipe, const void *buf,
				      int count);

/**
 * Read in a pipe.
 */
    int jpipe_read (jpipe_t * pipe, void *buf,
				     int count);

/**
 * Get descriptor of reading pipe.
 */
    int jpipe_get_read_descr (jpipe_t * pipe);

#ifdef __cplusplus
}
#endif
/** @} */
#endif
