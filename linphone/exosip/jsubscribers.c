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


#ifdef ENABLE_MPATROL
#include <mpatrol.h>
#endif

#include "eXosip2.h"
#include <eXosip.h>
#include <eXosip_cfg.h>

extern eXosip_t eXosip;

#ifndef EXOSIP_ETC_DIR
#define EXOSIP_ETC_DIR ".eXosip"
#endif

#ifndef EXOSIP_ADDSUBSCRIBERS_SH
#define EXOSIP_ADDSUBSCRIBERS_SH "eXosip_addsubscriber.sh"
#endif


static int jsubscriber_init(jsubscriber_t **js, char *ch)
{
  char *next;
  int i;

  *js = (jsubscriber_t *)osip_malloc(sizeof(jsubscriber_t));
  if (*js==NULL) return -1;

  i = jfriend_get_and_set_next_token(&((*js)->s_nick), ch, &next);
  if (i != 0)
    goto js_error1;
  osip_clrspace ((*js)->s_nick);
  ch = next;

  i = jfriend_get_and_set_next_token(&((*js)->s_uri), ch, &next);
  if (i != 0)
    goto js_error2;
  osip_clrspace ((*js)->s_uri);
  ch = next;

  (*js)->s_allow = osip_strdup(next);
  osip_clrspace ((*js)->s_allow);

  return 0;

 js_error2:
  osip_free((*js)->s_nick);
 js_error1:
  osip_free(*js);
  *js = NULL;
  return -1;
}

void
jsubscriber_unload()
{
  jsubscriber_t *js;
  if (eXosip.j_subscribers==NULL) return;
  for (js=eXosip.j_subscribers; js!=NULL; js=eXosip.j_subscribers)
    {
      REMOVE_ELEMENT(eXosip.j_subscribers,js);
      osip_free(js->s_nick);
      osip_free(js->s_uri);
      osip_free(js->s_allow);
      osip_free(js);
    }

  osip_free(eXosip.j_subscribers);
  eXosip.j_subscribers=NULL;
  return;
}

int
jsubscriber_load()
{
  FILE *file;
  char *s; 
  jsubscriber_t *js;
  int pos;
  char *home;
  char filename[255];

  jsubscriber_unload();
  home = getenv("HOME");
  sprintf(filename, "%s/%s/%s", home, EXOSIP_ETC_DIR, "jm_subscriber");
  

  file = fopen(filename, "r");
  if (file==NULL) return -1;
  s = (char *)osip_malloc(255*sizeof(char));
  pos = 0;
  while (NULL!=fgets(s, 254, file))
    {
      char *tmp = s;
      while (*tmp!='\0' && *tmp!=' ') tmp++;
      while (*tmp!='\0' && *tmp==' ') tmp++;
      while (*tmp!='\0' && *tmp!=' ') tmp++;
      tmp++; /* first usefull characters */
      pos++;

      jsubscriber_init(&js, tmp);
      if (js!=NULL)
	{ ADD_ELEMENT(eXosip.j_subscribers, js); }
    }
  osip_free(s);
  fclose(file);

  return 0; /* ok */
}

void subscribers_add(char *nickname, char *uri, int black_list)
{
  char *home;
  char command[256];
  char *tmp = command;
  int length = 0;
  if (nickname!=NULL)
    length = strlen(nickname);

  if (uri==NULL)
    return ;
  home = getenv("HOME");
  length = length + strlen(home);
  length = length + strlen(uri);

  length = length + 10; /* for black_list info */
  length = length + strlen(EXOSIP_ETC_DIR);

  length = length + strlen("/jm_subscriber");
  if (length>235) /* leave some room for SPACEs and \r\n */
    return ;

  sprintf(tmp , "%s %s/%s/jm_subscriber", EXOSIP_ADDSUBSCRIBERS_SH, home, EXOSIP_ETC_DIR);

  tmp = tmp + strlen(tmp);
  if (nickname!=NULL)
    sprintf(tmp , " %s", nickname);
  else
    sprintf(tmp , " \"\"");

  tmp = tmp + strlen(tmp);
  sprintf(tmp , " %s", uri);

  tmp = tmp + strlen(tmp);
  if (black_list==0) /* allowed */
    sprintf(tmp , " allow");
  else
    sprintf(tmp , " reject");

  system(command);  

  jsubscriber_load();
}

char *
jsubscriber_get_uri(int fid)
{
  jsubscriber_t *js;
  for (js = eXosip.j_subscribers; js!=NULL ; js=js->next)
    {
      if (fid==0)
	return osip_strdup(js->s_uri);
      fid--;
    }
  return NULL;
}
