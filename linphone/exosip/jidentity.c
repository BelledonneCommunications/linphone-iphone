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


#ifndef EXOSIP_ADDIDENTITYS_SH
#define EXOSIP_ADDIDENTITYS_SH "eXosip_addidentity.sh"
#endif

static int
jidentity_get_and_set_next_token (char **dest, char *buf, char **next)
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

static int jidentity_init(jidentity_t **fr, char *ch)
{
  char *next;
  int i;

  *fr = (jidentity_t *)osip_malloc(sizeof(jidentity_t));
  if (*fr==NULL) return -1;

  i = jidentity_get_and_set_next_token(&((*fr)->i_identity), ch, &next);
  if (i != 0)
    goto ji_error1;
  osip_clrspace ((*fr)->i_identity);
  ch = next;

  i = jidentity_get_and_set_next_token(&((*fr)->i_registrar), next, &next);
  if (i != 0)
    goto ji_error2;
  osip_clrspace ((*fr)->i_registrar);
  ch = next;

  i = jidentity_get_and_set_next_token(&((*fr)->i_realm), ch, &next);
  if (i != 0)
    goto ji_error3;
  osip_clrspace ((*fr)->i_realm);
  ch = next;

  i = jidentity_get_and_set_next_token(&((*fr)->i_userid), ch, &next);
  if (i != 0)
    goto ji_error4;
  osip_clrspace ((*fr)->i_userid);

  (*fr)->i_pwd = osip_strdup(next);
  osip_clrspace ((*fr)->i_pwd);

  if ((*fr)->i_pwd!=NULL && (*fr)->i_pwd[0]!='\0')
    {
      eXosip_add_authentication_info((*fr)->i_userid, (*fr)->i_userid,
				     (*fr)->i_pwd, NULL,
				     (*fr)->i_realm);
    }
  return 0;

 ji_error4:
  osip_free((*fr)->i_realm);
 ji_error3:
  osip_free((*fr)->i_registrar);
 ji_error2:
  osip_free((*fr)->i_identity);
 ji_error1:
  osip_free(*fr);
  *fr = NULL;
  return -1;
}

void identitys_add(char *identity, char *registrar,
		   char *realm, char *userid, char *pwd)
{
  char command[256];
  char *tmp = command;
  char *home;

  int length = 0;
  if (identity==NULL)
    return ;
  if (registrar==NULL)
    return ;

  if (realm!=NULL && *realm=='\0')
    realm = NULL;
  if (userid!=NULL && *userid=='\0')
    userid = NULL;
  if (pwd!=NULL && *pwd=='\0')
    pwd = NULL;

  length = strlen(identity) +3;
  length = length + strlen(registrar) +3;

  if (realm!=NULL && userid!=NULL && pwd!=NULL)
    {
      length = length + strlen(realm) +3;
      length = length + strlen(userid) +3;
      length = length + strlen(pwd) +3;
    }
  else if (realm==NULL && userid==NULL && pwd==NULL)
    {}
  else
    return ;

  home = getenv("HOME");
  length = length + strlen(home);
  length = length + strlen(EXOSIP_ETC_DIR) + 3;
  length = length + strlen("/jm_identity") + 1;

  if (length>235) /* leave some room for SPACEs and \r\n */
    return ;

  sprintf(tmp , "%s \"%s/%s/jm_identity\"", EXOSIP_ADDIDENTITYS_SH,
	  home, EXOSIP_ETC_DIR);
  tmp = tmp + strlen(tmp);
  sprintf(tmp , " \"%s\"", identity);
  tmp = tmp + strlen(tmp);
  sprintf(tmp , " \"%s\"", registrar);
  tmp = tmp + strlen(tmp);

  if (realm!=NULL && userid!=NULL && pwd!=NULL)
    {
      sprintf(tmp , " \"%s\"", realm);
      tmp = tmp + strlen(tmp);
      sprintf(tmp , " \"%s\"", userid);
      tmp = tmp + strlen(tmp);
      sprintf(tmp , " \"%s\"", pwd);
    }
  else if (realm==NULL && userid==NULL && pwd==NULL)
    {
      sprintf(tmp , " \"\"");
      tmp = tmp + strlen(tmp);
      sprintf(tmp , " \"\"");
      tmp = tmp + strlen(tmp);
      sprintf(tmp , " \"\"");
    }

  system(command);
}

void
jidentity_unload()
{
  jidentity_t *fr;
  if (eXosip.j_identitys==NULL) return;
  for (fr=eXosip.j_identitys; fr!=NULL; fr=eXosip.j_identitys)
    {
    REMOVE_ELEMENT(eXosip.j_identitys,fr);
    osip_free(fr->i_identity);
    osip_free(fr->i_registrar);
    osip_free(fr->i_realm);
    osip_free(fr->i_userid);
    osip_free(fr->i_pwd);
    osip_free(fr);
    }

  osip_free(eXosip.j_identitys);
  eXosip.j_identitys=NULL;
  return;
}

int
jidentity_load()
{
  FILE *file;
  char *s; 
  jidentity_t *fr;
  int pos;
  char *home;
  char filename[255];
  jidentity_unload();

  home = getenv("HOME");
  sprintf(filename, "%s/%s/%s", home, EXOSIP_ETC_DIR, "jm_identity");

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

      jidentity_init(&fr, tmp);
      if (fr!=NULL)
	{ ADD_ELEMENT(eXosip.j_identitys, fr); }
    }
  osip_free(s);
  fclose(file);

  return 0; /* ok */
}

char *
jidentity_get_registrar(int fid)
{
  jidentity_t *fr;
  for (fr = eXosip.j_identitys; fr!=NULL ; fr=fr->next)
    {
      if (fid==0)
	return osip_strdup(fr->i_registrar);
      fid--;
    }
  return NULL;
}

char *
jidentity_get_identity(int fid)
{
  jidentity_t *fr;
  for (fr = eXosip.j_identitys; fr!=NULL ; fr=fr->next)
    {
      if (fid==0)
	return osip_strdup(fr->i_identity);
      fid--;
    }
  return NULL;
}
