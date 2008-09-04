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
#include <eXosip_cfg.h>

extern eXosip_t eXosip;

#ifndef EXOSIP_ETC_DIR
#define EXOSIP_ETC_DIR ".eXosip"
#endif

#ifndef EXOSIP_ADDFRIENDS_SH
#define EXOSIP_ADDFRIENDS_SH "eXosip_addfriend.sh"
#endif


static int jfriend_init(jfriend_t **fr, char *ch)
{
  char *next;
  int i;

  *fr = (jfriend_t *)osip_malloc(sizeof(jfriend_t));
  if (*fr==NULL) return -1;

  i = jfriend_get_and_set_next_token(&((*fr)->f_nick), ch, &next);
  if (i != 0)
    goto jf_error1;
  osip_clrspace ((*fr)->f_nick);
  ch = next;

  i = jfriend_get_and_set_next_token(&((*fr)->f_home), next, &next);
  if (i != 0)
    goto jf_error2;
  osip_clrspace ((*fr)->f_home);
  ch = next;

  i = jfriend_get_and_set_next_token(&((*fr)->f_work), ch, &next);
  if (i != 0)
    goto jf_error3;
  osip_clrspace ((*fr)->f_work);
  ch = next;

  i = jfriend_get_and_set_next_token(&((*fr)->f_email), ch, &next);
  if (i != 0)
    goto jf_error4;
  osip_clrspace ((*fr)->f_email);

  (*fr)->f_e164 = osip_strdup(next);
  osip_clrspace ((*fr)->f_e164);

  return 0;

 jf_error4:
  osip_free((*fr)->f_work);
 jf_error3:
  osip_free((*fr)->f_home);
 jf_error2:
  osip_free((*fr)->f_nick);
 jf_error1:
  osip_free(*fr);
  *fr = NULL;
  return -1;
}

int
jfriend_get_and_set_next_token (char **dest, char *buf, char **next)
{
  char *end;
  char *start;

  *next = NULL;

  /* find first non space and tab element */
  start = buf;
  while (((*start == ' ') || (*start == '\t')) && (*start != '\0')
         && (*start != '\r') && (*start != '\n') )
    start++;
  end = start+1;
  while ((*end != '\0') && (*end != '\r') && (*end != '\n')
         && (*end != '\t') && (*end != '|'))
    end++;
  
  if ((*end == '\r') || (*end == '\n'))
    /* we should continue normally only if this is the separator asked! */
    return -1;
  if (end == start)
    return -1;                  /* empty value (or several space!) */

  *dest = osip_malloc (end - (start) + 1);
  osip_strncpy (*dest, start, end - start);

  *next = end + 1;   /* return the position right after the separator
 */
  return 0;
}

void __jfriend_remove(char *nickname, char *home)
{
  char *Home;
  char command[256];
  char *tmp = command;
  int length = 0;
  if (nickname!=NULL)
    length = strlen(nickname);

  Home = getenv("HOME");
  if (Home==NULL)
    return;
  length = length + strlen(Home);
  osip_clrspace(nickname);
  osip_clrspace(home);

  if (home!=NULL)
    length = length + strlen(home);
  else
    return; /* MUST be set */
  length = length + strlen(EXOSIP_ETC_DIR);

  length = length + strlen("/jm_contact");
  if (length>235) /* leave some room for SPACEs and \r\n */
    return ;

  sprintf(tmp , "%s %s/%s/jm_contact", EXOSIP_ADDFRIENDS_SH,
	  Home, EXOSIP_ETC_DIR);

  tmp = tmp + strlen(tmp);
  if (nickname!=NULL)
    sprintf(tmp , " %s", nickname);
  else
    sprintf(tmp , " \"\"");

  tmp = tmp + strlen(tmp);
  if (home!=NULL)
    sprintf(tmp , " %s", home);
  else
    sprintf(tmp , " \"\"");

  sprintf(tmp , "delete");

  OSIP_TRACE (osip_trace
    (__FILE__, __LINE__, OSIP_ERROR, NULL,
     "%s", command));
  system(command);
}

void jfriend_add(char *nickname, char *home,
		 char *work, char *email, char *e164)
{
  char *Home;
  char command[256];
  char *tmp = command;
  int length = 0;
  if (nickname!=NULL)
    length = strlen(nickname);

  Home = getenv("HOME");
  if (Home==NULL)
    return;
  length = length + strlen(Home);

  osip_clrspace(nickname);
  osip_clrspace(home);
  osip_clrspace(work);
  osip_clrspace(email);
  osip_clrspace(e164);

  if (home!=NULL)
    length = length + strlen(home);
  else
    return; /* MUST be set */
  if (work!=NULL)
    length = length + strlen(work);
  if (email!=NULL)
    length = length + strlen(email);
  if (e164!=NULL)
    length = length + strlen(e164);
  length = length + strlen(EXOSIP_ETC_DIR);

  length = length + strlen("/jm_contact");
  if (length>235) /* leave some room for SPACEs and \r\n */
    return ;

  sprintf(tmp , "%s %s/%s/jm_contact", EXOSIP_ADDFRIENDS_SH,
	  Home, EXOSIP_ETC_DIR);

  tmp = tmp + strlen(tmp);
  if (nickname!=NULL)
    sprintf(tmp , " %s", nickname);
  else
    sprintf(tmp , " \"\"");

  tmp = tmp + strlen(tmp);
  if (home!=NULL)
    sprintf(tmp , " %s", home);
  else
    sprintf(tmp , " \"\"");

  tmp = tmp + strlen(tmp);
  if (work!=NULL)
    sprintf(tmp , " %s", work);
  else
    sprintf(tmp , " \"\"");

  tmp = tmp + strlen(tmp);
  if (email!=NULL)
    sprintf(tmp , " %s", email);
  else
    sprintf(tmp , " \"\"");

  tmp = tmp + strlen(tmp);
  if (e164!=NULL)
    sprintf(tmp , " %s", e164);
  else
    sprintf(tmp , " \"\"");

  /*  fprintf(stderr, "%s", command); */
  system(command);
}

void
jfriend_unload()
{
  jfriend_t *fr;
  if (eXosip.j_friends==NULL) return;
  for (fr=eXosip.j_friends; fr!=NULL; fr=eXosip.j_friends)
    {
    REMOVE_ELEMENT(eXosip.j_friends,fr);
    osip_free(fr->f_nick);
    osip_free(fr->f_home);
    osip_free(fr->f_work);
    osip_free(fr->f_email);
    osip_free(fr->f_e164);
    osip_free(fr);
    }

  osip_free(eXosip.j_friends);
  eXosip.j_friends=NULL;
  return;
}

int
jfriend_load()
{
  FILE *file;
  char *s;
  jfriend_t *fr;
  int pos;
  char *home;
  char filename[255];

  jfriend_unload();
  home = getenv("HOME");
  sprintf(filename, "%s/%s/%s", home, EXOSIP_ETC_DIR, "jm_contact");
  
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

      jfriend_init(&fr, tmp);
      if (fr!=NULL)
	{ ADD_ELEMENT(eXosip.j_friends, fr); }
    }
  osip_free(s);
  fclose(file);

  return 0; /* ok */
}

char *
jfriend_get_home(int fid)
{
  jfriend_t *fr;
  for (fr = eXosip.j_friends; fr!=NULL ; fr=fr->next)
    {
      if (fid==0)
	return osip_strdup(fr->f_home);
      fid--;
    }
  return NULL;
}
