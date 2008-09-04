/***************************************************************************
                          propertybox.h  -  description
                             -------------------
    begin                : Wed Jan 30 2002
    copyright            : (C) 2002 by Simon Morlat
    email                : simon.morlat@linphone.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#include "interface.h"

struct _NetSection
{
	GtkWidget *interfaces;
	gint if_sel;
	GtkWidget *au_port;
	GtkWidget *audio_jittcomp;
	GtkWidget *enable_nat;
	GtkWidget *nat_label;
	GtkWidget *nat_address;
	GtkWidget *use_sipinfo;
	GtkWidget *enable_ipv6;
};
typedef struct _NetSection NetSection;

void net_section_init(NetSection *sec, GtkWidget *prop);
void net_section_apply(NetSection *sec, LinphoneCore *lp);

struct _SipSection
{
	GtkWidget *port;
	GtkWidget *username;
	GtkWidget *hostname;
	GtkWidget *proxy_list;
	GtkWidget *guess_hostname;
};

typedef struct _SipSection SipSection;

void sip_section_enable_registrar(SipSection *sec, LinphoneCore *lp, gboolean state);
void sip_section_fill(SipSection *sec, LinphoneCore *lp);

void sip_section_init(SipSection *sec, GtkWidget *prop);
void sip_section_apply(SipSection *sec, LinphoneCore *lp);

struct _CodecSection
{
	GtkWidget *au_codec_list;
	GtkWidget *vi_codec_list;
	GtkWidget *codec_info;
};

typedef struct _CodecSection CodecSection;

void codec_section_init(CodecSection *sec, GtkWidget *prop);
void codec_section_apply(CodecSection *sec, LinphoneCore *lc);

struct _SoundSection
{
	GtkWidget *source_entry;
	GtkWidget *autokill_button;
	GtkWidget *ringfileentry;
  GtkWidget *ringpreview;
};
typedef struct   _SoundSection  SoundSection;

void sound_section_init(SoundSection *sec,GtkWidget *prop);
void sound_section_apply(SoundSection *sec, LinphoneCore *lc);

struct _LinphonePropertyBox
{
	GtkWidget *prop;
	NetSection net;
	SipSection sip;
	CodecSection codec;
	SoundSection sound;
};

typedef struct _LinphonePropertyBox LinphonePropertyBox;

void linphone_property_box_init(LinphonePropertyBox *box);
//void linphone_property_box_apply(LinphonePropertyBox * box, LinphoneCore *lc, int page);
void linphone_property_box_apply(int page);
void linphone_property_box_uninit(LinphonePropertyBox *box);
