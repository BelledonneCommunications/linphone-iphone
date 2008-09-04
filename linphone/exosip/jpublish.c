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

extern eXosip_t eXosip;

int _eXosip_pub_update(eXosip_pub_t **pub, osip_transaction_t *tr, osip_message_t *answer)
{
  eXosip_pub_t *jpub;

  *pub = NULL;

  for (jpub = eXosip.j_pub; jpub!=NULL; jpub = jpub->next)
    {
      if (jpub->p_last_tr==NULL)
	{ /*bug? */ }
      else if (tr==jpub->p_last_tr)
	{
	  /* update the sip_etag parameter */
	  if (answer==NULL)
	    { /* bug? */
	    }
	  else if (MSG_IS_STATUS_2XX(answer))
	    {
	      osip_header_t *sip_etag=NULL;
	      osip_message_header_get_byname(answer, "SIP-ETag", 0, &sip_etag);
	      if (sip_etag!=NULL && sip_etag->hvalue!=NULL)
		snprintf(jpub->p_sip_etag, 64, "%s", sip_etag->hvalue);
	    }
	  *pub=jpub;
	  return 0;
	}
    }
  return -1;
}

int _eXosip_pub_find_by_aor(eXosip_pub_t **pub, const char *aor)
{
  eXosip_pub_t *jpub;
  eXosip_pub_t *ptr;
  time_t now;

  *pub = NULL;

  /* delete expired publications */
  now = time(NULL);
  ptr = eXosip.j_pub;
  for (jpub = ptr; jpub!=NULL; jpub = ptr)
    {
      ptr = jpub->next;
      if (now-jpub->p_expires>60)
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_WARNING, NULL,
		       "eXosip: removing expired publication!"));
	  REMOVE_ELEMENT(eXosip.j_pub, jpub);
	  _eXosip_pub_free(jpub);
	}
    }

  for (jpub = eXosip.j_pub; jpub!=NULL; jpub = jpub->next)
    {
      if (osip_strcasecmp(aor, jpub->p_aor)==0)
	{
	  *pub=jpub;
	  return 0;
	}
    }
  return -1;
}

int _eXosip_pub_init(eXosip_pub_t **pub, const char *aor, const char *exp)
{
  eXosip_pub_t *jpub;

  *pub = NULL;

  jpub = (eXosip_pub_t*) osip_malloc(sizeof(eXosip_pub_t));
  if (jpub==0)
    return -1;
  memset(jpub, 0, sizeof(eXosip_pub_t));
  snprintf(jpub->p_aor, 256, "%s", aor);

  jpub->p_expires = atoi(exp) + time(NULL);
  jpub->p_period = atoi(exp);

  *pub = jpub;
  return 0;
}

void _eXosip_pub_free(eXosip_pub_t *pub)
{
  if (pub->p_last_tr!=NULL)
    osip_list_add(eXosip.j_transactions, pub->p_last_tr, 0);
  osip_free(pub);
}

