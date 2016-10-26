/****************************************************************************
 *
 *  $Id: linphonec.h,v 1.3 2006/01/20 14:12:34 strk Exp $
 *
 *  Copyright (C) 2005  Sandro Santilli <strk@keybit.net>
 *
 ****************************************************************************
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************/

#ifndef LINPHONEC_H
#define LINPHONEC_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_READLINE
#ifdef HAVE_READLINE_H
#include <readline.h>
#else
#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#endif
#endif
#ifdef HAVE_HISTORY_H
#include <history.h>
#else
#ifdef HAVE_READLINE_HISTORY_H
#include <readline/history.h>
#endif
#endif
#endif

#undef PARAMS
/**************************************************************************
 *
 * Compile-time defines
 *
 **************************************************************************/

#define HISTSIZE 500		/* how many lines of input history */
#define PROMPT_MAX_LEN 256	/* max len of prompt string */
#define LINE_MAX_LEN 256	/* really needed ? */

/*
 * Define this to have your primary contact
 * as the prompt string
 */
/* #define IDENTITY_AS_PROMPT 1 */

/*
 * Time between calls to linphonec_idle_call during main
 * input read loop in microseconds.
 */
#define LPC_READLINE_TIMEOUT 10000

/*
 * Filename of linphonec history
 */
#define LPC_HIST_FILE "~/.linphonec_history"

/*
 * Maximum number of pending authentications
 */
#define MAX_PENDING_AUTH 8

/**************************************************************************
 *
 *  Types
 *
 **************************************************************************/

/*
 * A structure which contains information on the commands this program
 * can understand.
 */
typedef int (*lpc_cmd_handler)(LinphoneCore *, char *);
typedef struct {
	char *name;		/* User printable name of the function. */
	lpc_cmd_handler func;	/* Function to call to do the job. */
	char *help;		/* Short help for this function.  */
	char *doc;		/* Long description.  */
} LPC_COMMAND;

typedef struct {
	int x,y,w,h;
	void *wid;
	bool_t show;
	bool_t refresh;
} VideoParams;


extern VideoParams lpc_video_params;
extern VideoParams lpc_preview_params;

/***************************************************************************
 *
 *  Forward declarations
 *
 ***************************************************************************/

extern int linphonec_parse_command_line(LinphoneCore *lc, char *cl);
extern char *linphonec_command_generator(const char *text, int state);
void linphonec_main_loop_exit(void);
extern void linphonec_finish(int exit_status);
extern char *linphonec_readline(char *prompt);
void linphonec_set_autoanswer(bool_t enabled);
bool_t linphonec_get_autoanswer(void);
void linphonec_command_finished(void);
void linphonec_set_caller(const char *caller);
LinphoneCall *linphonec_get_call(long id);
void linphonec_call_identify(LinphoneCall* call);

extern bool_t linphonec_camera_enabled;

#endif /* def LINPHONEC_H */

/****************************************************************************
 *
 * $Log: linphonec.h,v $
 * Revision 1.3  2006/01/20 14:12:34  strk
 * Added linphonec_init() and linphonec_finish() functions.
 * Handled SIGINT and SIGTERM to invoke linphonec_finish().
 * Handling of auto-termination (-t) moved to linphonec_finish().
 * Reworked main (input read) loop to not rely on 'terminate'
 * and 'run' variable (dropped). configfile_name allocated on stack
 * using PATH_MAX limit. Changed print_usage signature to allow
 * for an exit_status specification.
 *
 * Revision 1.2  2006/01/14 13:29:32  strk
 * Reworked commands interface to use a table structure,
 * used by command line parser and help function.
 * Implemented first level of completion (commands).
 * Added notification of invalid "answer" and "terminate"
 * commands (no incoming call, no active call).
 * Forbidden "call" intialization when a call is already active.
 * Cleaned up all commands, adding more feedback and error checks.
 *
 * Revision 1.1  2006/01/13 13:00:29  strk
 * Added linphonec.h. Code layout change (added comments, forward decl,
 * globals on top, copyright notices and Logs). Handled out-of-memory
 * condition on history management. Removed assumption on sizeof(char).
 * Fixed bug in authentication prompt (introduced by readline).
 * Added support for multiple authentication requests (up to MAX_PENDING_AUTH).
 *
 *
 ****************************************************************************/
