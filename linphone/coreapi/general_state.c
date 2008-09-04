/****************************************************************************
 *
 *  File: general_state.c
 *
 *  Copyright (C) 2006, 2007  Thomas Reitmayr <treitmayr@yahoo.com>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 ****************************************************************************/


#include "linphonecore.h"


static gstate_t _gstates[GSTATE_GROUP_CALL+1];

#if 0
static const char *_gstates_text[] = {
  "GSTATE_POWER_OFF",         /* 0 */
  "GSTATE_POWER_STARTUP",     /* 1 */
  "GSTATE_POWER_ON",          /* 2 */
  "GSTATE_POWER_SHUTDOWN",    /* 3 */
  NULL, NULL, NULL, NULL, NULL, NULL,

  "GSTATE_REG_NONE",          /* 10 */
  "GSTATE_REG_OK",            /* 11 */
  "GSTATE_REG_FAILED",        /* 12 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL,

  "GSTATE_CALL_IDLE",           /* 20 */
  "GSTATE_CALL_OUT_INVITE",     /* 21 */
  "GSTATE_CALL_OUT_CONNECTED",  /* 22 */
  "GSTATE_CALL_IN_INVITE",      /* 23 */
  "GSTATE_CALL_IN_CONNECTED",   /* 24 */
  "GSTATE_CALL_END",            /* 25 */
  "GSTATE_CALL_ERROR"           /* 26 */
};
#endif

/* set the initial states */
void gstate_initialize(void) {
  _gstates[GSTATE_GROUP_POWER] = GSTATE_POWER_OFF;
  _gstates[GSTATE_GROUP_REG]   = GSTATE_REG_NONE;
  _gstates[GSTATE_GROUP_CALL]  = GSTATE_CALL_IDLE;
}


/* retrieve the current state of the specified state group */
gstate_t gstate_get_state(gstate_group_t group) {
  return _gstates[group];
}


void gstate_new_state(struct _LinphoneCore *lc,
                      gstate_t new_state,
                      const char *message) {
  LinphoneGeneralState states_arg;
  
  /* determine the affected group */
  if (new_state < GSTATE_REG_NONE)
    states_arg.group = GSTATE_GROUP_POWER;
  else if (new_state < GSTATE_CALL_IDLE)
    states_arg.group = GSTATE_GROUP_REG;
  else
    states_arg.group = GSTATE_GROUP_CALL;
  
  /* store the new state while remembering the old one */
  states_arg.new_state = new_state;
  states_arg.old_state = _gstates[states_arg.group];
  _gstates[states_arg.group] = new_state;
  states_arg.message = message;
  
  /*printf("gstate_new_state: %s\t-> %s\t(%s)\n",
         _gstates_text[states_arg.old_state],
         _gstates_text[states_arg.new_state],
         message);*/
  
  /* call the virtual method */
  if (lc->vtable.general_state)
    lc->vtable.general_state(lc, &states_arg);
  
  /* immediately proceed to idle state */
  if (new_state == GSTATE_CALL_END ||
      new_state == GSTATE_CALL_ERROR)
    gstate_new_state(lc, GSTATE_CALL_IDLE, NULL);
}

