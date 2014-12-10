/*
linphone
Copyright (C) 2010-2014  Belledonne Communications SARL

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

#define _XOPEN_SOURCE 700 /*required for strptime of GNU libc*/

#include <time.h>
#include "private.h"


/*******************************************************************************
 * Internal functions                                                          *
 ******************************************************************************/

/*prevent a gcc bug with %c*/
static size_t my_strftime(char *s, size_t max, const char  *fmt,  const struct tm *tm){
	return strftime(s, max, fmt, tm);
}

static time_t string_to_time(const char *date){
#ifndef WIN32
	struct tm tmtime={0};
	strptime(date,"%c",&tmtime);
	return mktime(&tmtime);
#else
	return 0;
#endif
}

static void set_call_log_date(LinphoneCallLog *cl, time_t start_time){
	struct tm loctime;
#ifdef WIN32
#if !defined(_WIN32_WCE)
	loctime=*localtime(&start_time);
	/*FIXME*/
#endif /*_WIN32_WCE*/
#else
	localtime_r(&start_time,&loctime);
#endif
	my_strftime(cl->start_date,sizeof(cl->start_date),"%c",&loctime);
}

/*******************************************************************************
 * Private functions                                                           *
 ******************************************************************************/

void call_logs_write_to_config_file(LinphoneCore *lc){
	MSList *elem;
	char logsection[32];
	int i;
	char *tmp;
	LpConfig *cfg=lc->config;

	if (linphone_core_get_global_state (lc)==LinphoneGlobalStartup) return;

	for(i=0,elem=lc->call_logs;elem!=NULL;elem=elem->next,++i){
		LinphoneCallLog *cl=(LinphoneCallLog*)elem->data;
		snprintf(logsection,sizeof(logsection),"call_log_%i",i);
		lp_config_clean_section(cfg,logsection);
		lp_config_set_int(cfg,logsection,"dir",cl->dir);
		lp_config_set_int(cfg,logsection,"status",cl->status);
		tmp=linphone_address_as_string(cl->from);
		lp_config_set_string(cfg,logsection,"from",tmp);
		ms_free(tmp);
		tmp=linphone_address_as_string(cl->to);
		lp_config_set_string(cfg,logsection,"to",tmp);
		ms_free(tmp);
		if (cl->start_date_time)
			lp_config_set_int64(cfg,logsection,"start_date_time",(int64_t)cl->start_date_time);
		else lp_config_set_string(cfg,logsection,"start_date",cl->start_date);
		lp_config_set_int(cfg,logsection,"duration",cl->duration);
		if (cl->refkey) lp_config_set_string(cfg,logsection,"refkey",cl->refkey);
		lp_config_set_float(cfg,logsection,"quality",cl->quality);
		lp_config_set_int(cfg,logsection,"video_enabled", cl->video_enabled);
		lp_config_set_string(cfg,logsection,"call_id",cl->call_id);
	}
	for(;i<lc->max_call_logs;++i){
		snprintf(logsection,sizeof(logsection),"call_log_%i",i);
		lp_config_clean_section(cfg,logsection);
	}
}

void call_logs_read_from_config_file(LinphoneCore *lc){
	char logsection[32];
	int i;
	const char *tmp;
	uint64_t sec;
	LpConfig *cfg=lc->config;
	for(i=0;;++i){
		snprintf(logsection,sizeof(logsection),"call_log_%i",i);
		if (lp_config_has_section(cfg,logsection)){
			LinphoneCallLog *cl;
			LinphoneAddress *from=NULL,*to=NULL;
			tmp=lp_config_get_string(cfg,logsection,"from",NULL);
			if (tmp) from=linphone_address_new(tmp);
			tmp=lp_config_get_string(cfg,logsection,"to",NULL);
			if (tmp) to=linphone_address_new(tmp);
			if (!from || !to)
				continue;
			cl=linphone_call_log_new(lp_config_get_int(cfg,logsection,"dir",0),from,to);
			cl->status=lp_config_get_int(cfg,logsection,"status",0);
			sec=lp_config_get_int64(cfg,logsection,"start_date_time",0);
			if (sec) {
				/*new call log format with date expressed in seconds */
				cl->start_date_time=(time_t)sec;
				set_call_log_date(cl,cl->start_date_time);
			}else{
				tmp=lp_config_get_string(cfg,logsection,"start_date",NULL);
				if (tmp) {
					strncpy(cl->start_date,tmp,sizeof(cl->start_date));
					cl->start_date_time=string_to_time(cl->start_date);
				}
			}
			cl->duration=lp_config_get_int(cfg,logsection,"duration",0);
			tmp=lp_config_get_string(cfg,logsection,"refkey",NULL);
			if (tmp) cl->refkey=ms_strdup(tmp);
			cl->quality=lp_config_get_float(cfg,logsection,"quality",-1);
			cl->video_enabled=lp_config_get_int(cfg,logsection,"video_enabled",0);
			tmp=lp_config_get_string(cfg,logsection,"call_id",NULL);
			if (tmp) cl->call_id=ms_strdup(tmp);
			lc->call_logs=ms_list_append(lc->call_logs,cl);
		}else break;
	}
}


/*******************************************************************************
 * Public functions                                                            *
 ******************************************************************************/

const char *linphone_call_log_get_call_id(const LinphoneCallLog *cl){
	return cl->call_id;
}

LinphoneCallDir linphone_call_log_get_dir(LinphoneCallLog *cl){
	return cl->dir;
}

int linphone_call_log_get_duration(LinphoneCallLog *cl){
	return cl->duration;
}

LinphoneAddress *linphone_call_log_get_from_address(LinphoneCallLog *cl){
	return cl->from;
}

const rtp_stats_t *linphone_call_log_get_local_stats(const LinphoneCallLog *cl){
	return &cl->local_stats;
}

float linphone_call_log_get_quality(LinphoneCallLog *cl){
	return cl->quality;
}

const char *linphone_call_log_get_ref_key(const LinphoneCallLog *cl){
	return cl->refkey;
}

LinphoneAddress *linphone_call_log_get_remote_address(LinphoneCallLog *cl){
	return (cl->dir == LinphoneCallIncoming) ? cl->from : cl->to;
}

const rtp_stats_t *linphone_call_log_get_remote_stats(const LinphoneCallLog *cl){
	return &cl->remote_stats;
}

time_t linphone_call_log_get_start_date(LinphoneCallLog *cl){
	return cl->start_date_time;
}

LinphoneCallStatus linphone_call_log_get_status(LinphoneCallLog *cl){
	return cl->status;
}

LinphoneAddress *linphone_call_log_get_to_address(LinphoneCallLog *cl){
	return cl->to;
}

void linphone_call_log_set_ref_key(LinphoneCallLog *cl, const char *refkey){
	if (cl->refkey!=NULL){
		ms_free(cl->refkey);
		cl->refkey=NULL;
	}
	if (refkey) cl->refkey=ms_strdup(refkey);
}

char * linphone_call_log_to_str(LinphoneCallLog *cl){
	char *status;
	char *tmp;
	char *from=linphone_address_as_string (cl->from);
	char *to=linphone_address_as_string (cl->to);
	switch(cl->status){
		case LinphoneCallAborted:
			status=_("aborted");
			break;
		case LinphoneCallSuccess:
			status=_("completed");
			break;
		case LinphoneCallMissed:
			status=_("missed");
			break;
		default:
			status=_("unknown");
	}
	tmp=ortp_strdup_printf(_("%s at %s\nFrom: %s\nTo: %s\nStatus: %s\nDuration: %i mn %i sec\n"),
			(cl->dir==LinphoneCallIncoming) ? _("Incoming call") : _("Outgoing call"),
			cl->start_date,
			from,
			to,
			status,
			cl->duration/60,
			cl->duration%60);
	ms_free(from);
	ms_free(to);
	return tmp;
}

bool_t linphone_call_log_video_enabled(LinphoneCallLog *cl) {
	return cl->video_enabled;
}


/*******************************************************************************
 * Reference and user data handling functions                                  *
 ******************************************************************************/

void *linphone_call_log_get_user_data(const LinphoneCallLog *cl) {
	return cl->user_data;
}

void linphone_call_log_set_user_data(LinphoneCallLog *cl, void *ud) {
	cl->user_data = ud;
}

LinphoneCallLog * linphone_call_log_ref(LinphoneCallLog *cl) {
	belle_sip_object_ref(cl);
	return cl;
}

void linphone_call_log_unref(LinphoneCallLog *cl) {
	belle_sip_object_unref(cl);
}

/*******************************************************************************
 * Constructor and destructor functions                                        *
 ******************************************************************************/

static void _linphone_call_log_destroy(LinphoneCallLog *cl){
	if (cl->from!=NULL) linphone_address_destroy(cl->from);
	if (cl->to!=NULL) linphone_address_destroy(cl->to);
	if (cl->refkey!=NULL) ms_free(cl->refkey);
	if (cl->call_id) ms_free(cl->call_id);
	if (cl->reporting.reports[LINPHONE_CALL_STATS_AUDIO]!=NULL) linphone_reporting_destroy(cl->reporting.reports[LINPHONE_CALL_STATS_AUDIO]);
	if (cl->reporting.reports[LINPHONE_CALL_STATS_VIDEO]!=NULL) linphone_reporting_destroy(cl->reporting.reports[LINPHONE_CALL_STATS_VIDEO]);
}

LinphoneCallLog * linphone_call_log_new(LinphoneCallDir dir, LinphoneAddress *from, LinphoneAddress *to){
	LinphoneCallLog *cl=belle_sip_object_new(LinphoneCallLog);
	cl->dir=dir;
	cl->start_date_time=time(NULL);
	set_call_log_date(cl,cl->start_date_time);
	cl->from=from;
	cl->to=to;
	cl->status=LinphoneCallAborted; /*default status*/
	cl->quality=-1;

	cl->reporting.reports[LINPHONE_CALL_STATS_AUDIO]=linphone_reporting_new();
	cl->reporting.reports[LINPHONE_CALL_STATS_VIDEO]=linphone_reporting_new();
	cl->connected_date_time=0;
	return cl;
}

/* DEPRECATED */
void linphone_call_log_destroy(LinphoneCallLog *cl) {
	belle_sip_object_unref(cl);
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneCallLog);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneCallLog, belle_sip_object_t,
	(belle_sip_object_destroy_t)_linphone_call_log_destroy,
	NULL, // clone
	NULL, // marshal
	FALSE
);
