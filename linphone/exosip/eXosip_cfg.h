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

#ifndef __EXOSIP_CFG_H__
#define __EXOSIP_CFG_H__

/**
 * @defgroup eXosip_cfg eXosip Configuration Management
 * @ingroup eXosip
 * @{
 */

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct jfriend_t jfriend_t;

struct jfriend_t {
  int            f_id;
  char          *f_nick;
  char          *f_home;
  char          *f_work;
  char          *f_email;
  char          *f_e164;

  jfriend_t     *next;
  jfriend_t     *parent;
};

typedef struct jidentity_t jidentity_t;

struct jidentity_t {
  int            i_id;
  char          *i_identity;
  char          *i_registrar;
  char          *i_realm;
  char          *i_userid;
  char          *i_pwd;

  jidentity_t   *next;
  jidentity_t   *parent;
};

typedef struct jsubscriber_t jsubscriber_t;

struct jsubscriber_t {
  int            s_id;
  char          *s_nick;
  char          *s_uri;
  char          *s_allow;

  jsubscriber_t   *next;
  jsubscriber_t   *parent;
};

jfriend_t *jfriend_get(void);
void jfriend_remove(jfriend_t *fr);

jsubscriber_t *jsubscriber_get(void);
jidentity_t *jidentity_get(void);

int   jfriend_load(void);
void  jfriend_unload(void);
void  jfriend_add(char *nickname, char *home,
		  char *work, char *email, char *e164);
char *jfriend_get_home(int fid);

int   jsubscriber_load(void);
void  jsubscriber_unload(void);
void  subscribers_add(char *nickname, char *uri, int black_list);
char *jsubscriber_get_uri(int fid);

int   jidentity_load(void);
void  jidentity_unload(void);
void  identitys_add(char *identity, char *registrar, char *realm,
		    char *userid, char *password);
char *jidentity_get_identity(int fid);
char *jidentity_get_registrar(int fid);

#define REMOVE_ELEMENT(first_element, element)   \
       if (element->parent==NULL)                \
	{ first_element = element->next;         \
          if (first_element!=NULL)               \
          first_element->parent = NULL; }        \
       else \
        { element->parent->next = element->next; \
          if (element->next!=NULL)               \
	element->next->parent = element->parent; \
	element->next = NULL;                    \
	element->parent = NULL; }

#define ADD_ELEMENT(first_element, element) \
   if (first_element==NULL)                 \
    {                                       \
      first_element   = element;            \
      element->next   = NULL;               \
      element->parent = NULL;               \
    }                                       \
  else                                      \
    {                                       \
      element->next   = first_element;      \
      element->parent = NULL;               \
      element->next->parent = element;      \
      first_element = element;              \
    }

#define APPEND_ELEMENT(type_of_element_t, first_element, element) \
  if (first_element==NULL)                            \
    { first_element = element;                        \
      element->next   = NULL; /* useless */           \
      element->parent = NULL; /* useless */ }         \
  else                                                \
    { type_of_element_t *f;                           \
      for (f=first_element; f->next!=NULL; f=f->next) \
         { }                                          \
      f->next    = element;                           \
      element->parent = f;                            \
      element->next   = NULL;                         \
    }

#ifdef __cplusplus
}
#endif

/** @} */

#endif
