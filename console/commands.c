/****************************************************************************
 *
 *  $Id: commands.c,v 1.39 2008/07/03 15:08:34 smorlat Exp $
 *
 *  Copyright (C) 2006-2009  Sandro Santilli <strk@keybit.net>
 *  Copyright (C) 2004  Simon MORLAT <simon.morlat@linphone.org>
 *
****************************************************************************
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 ****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32_WCE
#include <errno.h>
#endif /*_WIN32_WCE*/
#include <limits.h>
#include <ctype.h>
#include <linphone/core.h>
#include "linphonec.h"
#include <linphone/lpconfig.h>

#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#endif

#define AUDIO 0
#define VIDEO 1

/***************************************************************************
 *
 *  Forward declarations
 *
 ***************************************************************************/

extern char *lpc_strip_blanks(char *input);

/* Command handlers */
static int lpc_cmd_help(LinphoneCore *, char *);
static int lpc_cmd_proxy(LinphoneCore *, char *);
static int lpc_cmd_call(LinphoneCore *, char *);
static int lpc_cmd_calls(LinphoneCore *, char *);
static int lpc_cmd_chat(LinphoneCore *, char *);
static int lpc_cmd_answer(LinphoneCore *, char *);
static int lpc_cmd_autoanswer(LinphoneCore *, char *);
static int lpc_cmd_terminate(LinphoneCore *, char *);
static int lpc_cmd_redirect(LinphoneCore *, char *);
static int lpc_cmd_call_logs(LinphoneCore *, char *);
static int lpc_cmd_ipv6(LinphoneCore *, char *);
static int lpc_cmd_transfer(LinphoneCore *, char *);
static int lpc_cmd_quit(LinphoneCore *, char *);
static int lpc_cmd_nat(LinphoneCore *, char *);
static int lpc_cmd_stun(LinphoneCore *, char *);
static int lpc_cmd_firewall(LinphoneCore *, char *);
static int lpc_cmd_friend(LinphoneCore *, char*);
static int lpc_cmd_soundcard(LinphoneCore *, char *);
static int lpc_cmd_webcam(LinphoneCore *, char *);
static int lpc_cmd_staticpic(LinphoneCore *, char *);
static int lpc_cmd_play(LinphoneCore *, char *);
static int lpc_cmd_record(LinphoneCore *, char *);
static int lpc_cmd_register(LinphoneCore *, char *);
static int lpc_cmd_unregister(LinphoneCore *, char *);
static int lpc_cmd_duration(LinphoneCore *lc, char *args);
static int lpc_cmd_status(LinphoneCore *lc, char *args);
static int lpc_cmd_ports(LinphoneCore *lc, char *args);
static int lpc_cmd_param(LinphoneCore *lc, char *args);
static int lpc_cmd_speak(LinphoneCore *lc, char *args);
static int lpc_cmd_acodec(LinphoneCore *lc, char *args);
static int lpc_cmd_vcodec(LinphoneCore *lc, char *args);
static int lpc_cmd_codec(int type, LinphoneCore *lc, char *args);
static int lpc_cmd_echocancellation(LinphoneCore *lc, char *args);
static int lpc_cmd_echolimiter(LinphoneCore *lc, char *args);
static int lpc_cmd_pause(LinphoneCore *lc, char *args);
static int lpc_cmd_resume(LinphoneCore *lc, char *args);
static int lpc_cmd_mute_mic(LinphoneCore *lc, char *args);
static int lpc_cmd_unmute_mic(LinphoneCore *lc, char *args);
static int lpc_cmd_playback_gain(LinphoneCore *lc, char *args);
static int lpc_cmd_rtp_no_xmit_on_audio_mute(LinphoneCore *lc, char *args);
#ifdef VIDEO_ENABLED
static int lpc_cmd_camera(LinphoneCore *lc, char *args);
static int lpc_cmd_video_window(LinphoneCore *lc, char *args);
static int lpc_cmd_preview_window(LinphoneCore *lc, char *args);
static int lpc_cmd_snapshot(LinphoneCore *lc, char *args);
static int lpc_cmd_preview_snapshot(LinphoneCore *lc, char *args);
static int lpc_cmd_vfureq(LinphoneCore *lc, char *arg);
#endif
static int lpc_cmd_states(LinphoneCore *lc, char *args);
static int lpc_cmd_identify(LinphoneCore *lc, char *args);
static int lpc_cmd_ringback(LinphoneCore *lc, char *args);
static int lpc_cmd_conference(LinphoneCore *lc, char *args);
static int lpc_cmd_zrtp_verified(LinphoneCore *lc, char *args);
static int lpc_cmd_zrtp_unverified(LinphoneCore *lc, char *args);

/* Command handler helpers */
static void linphonec_proxy_add(LinphoneCore *lc);
static void linphonec_proxy_display(LinphoneProxyConfig *lc);
static void linphonec_proxy_list(LinphoneCore *lc);
static void linphonec_proxy_remove(LinphoneCore *lc, int index);
static  int linphonec_proxy_use(LinphoneCore *lc, int index);
static void linphonec_proxy_show(LinphoneCore *lc,int index);
static void linphonec_friend_display(LinphoneFriend *fr);
static int linphonec_friend_list(LinphoneCore *lc, char *arg);
static void linphonec_display_command_help(LPC_COMMAND *cmd);
static int linphonec_friend_call(LinphoneCore *lc, unsigned int num);
#ifndef _WIN32
static int linphonec_friend_add(LinphoneCore *lc, const char *name, const char *addr);
#endif
static int linphonec_friend_delete(LinphoneCore *lc, int num);
static int linphonec_friend_delete(LinphoneCore *lc, int num);
static void linphonec_codec_list(int type, LinphoneCore *lc);
static void linphonec_codec_enable(int type, LinphoneCore *lc, int index);
static void linphonec_codec_disable(int type, LinphoneCore *lc, int index);
static void lpc_display_call_states(LinphoneCore *lc);

/* Command table management */
static LPC_COMMAND *lpc_find_command(const char *name);

void linphonec_out(const char *fmt,...);

VideoParams lpc_video_params={-1,-1,-1,-1,NULL,TRUE,FALSE};
VideoParams lpc_preview_params={-1,-1,-1,-1,NULL,TRUE,FALSE};

/***************************************************************************
 *
 *  Global variables
 *
 ***************************************************************************/

/*
 * Commands table.
 */
static LPC_COMMAND commands[] = {
	{ "help", lpc_cmd_help, "Print commands help.",
		"'help <command>'\t: displays specific help for command.\n"
		"'help advanced'\t: shows advanced commands.\n"
	},
	{ "answer", lpc_cmd_answer, "Answer a call",
		"'answer' : Answer the current incoming call\n"
		"'answer <call id>' : Answer the call with given id\n"
	},
	{ "autoanswer", lpc_cmd_autoanswer, "Show/set auto-answer mode",
		"'autoanswer'       \t: show current autoanswer mode\n"
		"'autoanswer enable'\t: enable autoanswer mode\n"
		"'autoanswer disable'\t: disable autoanswer mode\n"
	},
	{ "call", lpc_cmd_call, "Call a SIP uri or number",
#ifdef VIDEO_ENABLED
		"'call <sip-url or number>  [options]' \t: initiate a call to the specified destination.\n"
		"Options can be:\n"
		"--audio-only : initiate the call without video.\n"
		"--early-media : sends audio and video stream immediately when remote proposes early media.\n"
#else
		"'call <sip-url or number>' \t: initiate a call to the specified destination.\n"
#endif
		},
	{ "calls", lpc_cmd_calls, "Show all the current calls with their id and status.",
		NULL
	},
	{ "call-logs", lpc_cmd_call_logs, "Calls history", NULL
	},
#ifdef VIDEO_ENABLED
	{ "camera", lpc_cmd_camera, "Send camera output for current call.",
		"'camera on'\t: allow sending of local camera video to remote end.\n"
		"'camera off'\t: disable sending of local camera's video to remote end.\n"
	},
#endif
	{ "chat", lpc_cmd_chat, "Chat with a SIP uri",
		"'chat <sip-url> \"message\"' "
		": send a chat message \"message\" to the specified destination."
	},
	{ "conference", lpc_cmd_conference, "Create and manage an audio conference.",
		"'conference add <call id> : join the call with id 'call id' into the audio conference."
		"'conference rm <call id> : remove the call with id 'call id' from the audio conference."
	},
	{ "duration", lpc_cmd_duration, "Print duration in seconds of the last call.", NULL
	},
	{ "firewall", lpc_cmd_firewall, "Set firewall policy",
		"'firewall'        : show current firewall policy.\n"
		"'firewall none'   : use direct connection.\n"
		"'firewall nat'    : use nat address given with the 'nat' command.\n"
		"'firewall stun'   : use stun server given with the 'stun' command.\n"
		"'firewall ice'    : use ice.\n"
		"'firewall upnp'   : use uPnP IGD.\n"
	},
	{ "friend", lpc_cmd_friend, "Manage friends",
		"'friend list [<pattern>]'    : list friends.\n"
		"'friend call <index>'        : call a friend.\n"
		"'friend add <name> <addr>'   : add friend, <name> must be quoted to include\n"
	    "                               spaces, <addr> has \"sip:\" added if it isn't\n"
	    "                               there.  Don't use '<' '>' around <addr>.\n"
		"'friend delete <index>'      : remove friend, 'all' removes all\n"
	},
	{ "ipv6", lpc_cmd_ipv6, "Use IPV6",
		"'ipv6 status' : show ipv6 usage status.\n"
		"'ipv6 enable' : enable the use of the ipv6 network.\n"
		"'ipv6 disable' : do not use ipv6 network."
	},
	{ "mute", lpc_cmd_mute_mic,
		"Mute microphone and suspend voice transmission.",
		NULL
	},
	{ "nat", lpc_cmd_nat, "Set nat address",
		"'nat'        : show nat settings.\n"
		"'nat <addr>' : set nat address.\n"
	},
	{ "pause", lpc_cmd_pause, "pause a call",
		"'pause' : pause the current call\n"
	},
	{ "play", lpc_cmd_play, "play a wav file",
		"This command has two roles:\n"
		"Plays a file instead of capturing from soundcard - only available in file mode (see 'help soundcard')\n"
		"Specifies a wav file to be played to play music to far end when putting it on hold (pause)\n"
		"'play <wav file>'    : play a wav file."
	},
	{ "playbackgain", lpc_cmd_playback_gain,
		"Adjust playback gain.",
		NULL
	},
	{ "proxy", lpc_cmd_proxy, "Manage proxies",
		"'proxy list' : list all proxy setups.\n"
		"'proxy add' : add a new proxy setup.\n"
		"'proxy remove <index>' : remove proxy setup with number index.\n"
		"'proxy use <index>' : use proxy with number index as default proxy.\n"
		"'proxy unuse' : don't use a default proxy.\n"
		"'proxy show <index>' : show configuration and status of the proxy numbered by index.\n"
		"'proxy show default' : show configuration and status of the default proxy.\n"
	},
	{ "record", lpc_cmd_record, "record to a wav file",
		"This feature is available only in file mode (see 'help soundcard')\n"
		"'record <wav file>'    : record into wav file."
	},
	{ "resume", lpc_cmd_resume, "resume a call",
		"'resume' : resume the unique call\n"
		"'resume <call id>' : hold off the call with given id\n"
	},
	{ "soundcard", lpc_cmd_soundcard, "Manage soundcards",
		"'soundcard list' : list all sound devices.\n"
		"'soundcard show' : show current sound devices configuration.\n"
		"'soundcard use <index>' : select a sound device.\n"
		"'soundcard use files' : use .wav files instead of soundcard\n"
	},
	{ "stun", lpc_cmd_stun, "Set stun server address",
		"'stun'        : show stun settings.\n"
		"'stun <addr>' : set stun server address.\n"
	},
	{ "terminate", lpc_cmd_terminate, "Terminate a call",
		"'terminate' : Terminate the current call\n"
		"'terminate <call id>' : Terminate the call with supplied id\n"
		"'terminate <all>' : Terminate all the current calls\n"
	},
	{ "transfer", lpc_cmd_transfer,
		"Transfer a call to a specified destination.",
		"'transfer <sip-uri>' : transfers the current active call to the destination sip-uri\n"
		"'transfer <call id> <sip-uri>': transfers the call with 'id' to the destination sip-uri\n"
		"'transfer <call id1> --to-call <call id2>': transfers the call with 'id1' to the destination of call 'id2' (attended transfer)\n"
	},
	{ "unmute", lpc_cmd_unmute_mic,
		"Unmute microphone and resume voice transmission.",
		NULL
	},
	{ "webcam", lpc_cmd_webcam, "Manage webcams",
		"'webcam list' : list all known devices.\n"
		"'webcam use <index>' : select a video device.\n"
	},
	{ "quit", lpc_cmd_quit, "Exit linphonec", NULL
	},
	{ (char *)NULL, (lpc_cmd_handler)NULL, (char *)NULL, (char *)NULL
	}
};


static LPC_COMMAND advanced_commands[] = {
	 { "codec", lpc_cmd_acodec, "Audio codec configuration",
            "'codec list' : list audio codecs\n"
            "'codec enable <index>' : enable available audio codec\n"
            "'codec disable <index>' : disable audio codec" },
	{ "vcodec", lpc_cmd_vcodec, "Video codec configuration",
		"'vcodec list' : list video codecs\n"
		"'vcodec enable <index>' : enable available video codec\n"
		"'vcodec disable <index>' : disable video codec" },
	{ "ec", lpc_cmd_echocancellation, "Echo cancellation",
	    "'ec on [<delay>] [<tail>] [<framesize>]' : turn EC on with given delay, tail length and framesize\n"
	    "'ec off' : turn echo cancellation (EC) off\n"
	    "'ec show' : show EC status" },
	{ "el", lpc_cmd_echolimiter, "Echo limiter",
	    "'el on turns on echo limiter (automatic half duplex, for cases where echo canceller cannot work)\n"
	    "'el off' : turn echo limiter off\n"
	    "'el show' : show echo limiter status" },
	{ "nortp-on-audio-mute", lpc_cmd_rtp_no_xmit_on_audio_mute,
		  "Set the rtp_no_xmit_on_audio_mute configuration parameter",
		  "   If set to 1 then rtp transmission will be muted when\n"
		  "   audio is muted , otherwise rtp is always sent."
		},
#ifdef VIDEO_ENABLED
	{ "vwindow", lpc_cmd_video_window, "Control video display window",
		"'vwindow show': shows video window\n"
		"'vwindow hide': hides video window\n"
		"'vwindow pos <x> <y>': Moves video window to x,y pixel coordinates\n"
		"'vwindow size <width> <height>': Resizes video window\n"
		"'vwindow id <window id>': embeds video display into supplied window id."
	},
	{ "pwindow", lpc_cmd_preview_window, "Control local camera video display (preview window)",
		"'pwindow show': shows the local camera video display\n"
		"'pwindow hide': hides the local camera video display\n"
		"'pwindow pos <x> <y>': Moves preview window to x,y pixel coordinates\n"
		"'pwindow size <width> <height>': Resizes preview window\n"
		"'pwindow id <window id>': embeds preview display into supplied window id.\n"
		"'pwindow integrated': integrate preview display within the video window of current call.\n"
		"'pwindow standalone': use standalone window for preview display."
	},
	{ "snapshot", lpc_cmd_snapshot, "Take a snapshot of currently received video stream",
		"'snapshot <file path>': take a snapshot and records it in jpeg format into the supplied path\n"
	},
	{ "preview-snapshot", lpc_cmd_preview_snapshot, "Take a snapshot of currently captured video stream",
		"'preview-snapshot <file path>': take a snapshot and records it in jpeg format into the supplied path\n"
	},
	{ "vfureq", lpc_cmd_vfureq, "Request the other side to send VFU for the current call",
		NULL
	},
#endif
	{ "states", lpc_cmd_states, "Show internal states of liblinphone, registrations and calls, according to linphonecore.h definitions",
		"'states global': shows global state of liblinphone \n"
		"'states calls': shows state of calls\n"
		"'states proxies': shows state of proxy configurations"
	},
	{ "register", lpc_cmd_register, "Register in one line to a proxy" , "register <sip identity> <sip proxy> <password>"
},
	{ "unregister", lpc_cmd_unregister, "Unregister from default proxy", NULL	},
	{ "status", lpc_cmd_status, "Print various status information",
			"'status register'  \t: print status concerning registration\n"
			"'status autoanswer'\t: tell whether autoanswer mode is enabled\n"
			"'status hook'      \t: print hook status\n" },
	{ "ports", lpc_cmd_ports, "Network ports configuration",
			"'ports'  \t: prints current used ports.\n"
			"'ports sip <port number>'\t: Sets the sip port.\n" },
	{ "param", lpc_cmd_param, "parameter set or read as normally given in .linphonerc",
			"'param <section> <parameter> [<value>]'  \t: reads [sets] given parameter.\n"
			"NOTES: - changes may become effective after (re)establishing a sip connection.\n"
			"       - upon exit, .linphonerc will reflect the updated state.\n" },
	{ "speak", lpc_cmd_speak, "Speak a sentence using espeak TTS engine",
			"This feature is available only in file mode. (see 'help soundcard')\n"
			"'speak <voice name> <sentence>'	: speak a text using the specified espeak voice.\n"
			"Example for english voice: 'speak default Hello my friend !'"
	},
	{ "staticpic", lpc_cmd_staticpic, "Manage static pictures when nowebcam",
		"'staticpic set' : Set path to picture that should be used.\n"
		"'staticpic fps' : Get/set frames per seconds for picture emission.\n"
	},
	{ "identify", lpc_cmd_identify, "Returns the user-agent string of far end",
		"'identify' \t: returns remote user-agent string for current call.\n"
		"'identify <id>' \t: returns remote user-agent string for call with supplied id.\n"
	},
	{ "ringback", lpc_cmd_ringback, "Specifies a ringback tone to be played to remote end during incoming calls",
		"'ringback <path of mono .wav file>'\t: Specifies a ringback tone to be played to remote end during incoming calls\n"
		"'ringback disable'\t: Disable playing of ringback tone to callers\n"
	},
	{ "redirect", lpc_cmd_redirect, "Redirect an incoming call",
		"'redirect <id> <redirect-uri>'\t: Redirect the specified call to the <redirect-uri>\n"
		"'redirect all <redirect-uri>'\t: Redirect all pending incoming calls to the <redirect-uri>\n"
	},
	{ "zrtp-set-verified", lpc_cmd_zrtp_verified,"Set ZRTP SAS verified.",
		"'Set ZRTP SAS verified'\n"
	},
	{ "zrtp-set-unverified", lpc_cmd_zrtp_unverified,"Set ZRTP SAS not verified.",
		"'Set ZRTP SAS not verified'\n"
	},
	{	NULL,NULL,NULL,NULL}
};



/***************************************************************************
 *
 *  Public interface
 *
 ***************************************************************************/

/*
 * Main command dispatcher.
 * WARNING: modifies second argument!
 *
 * Always return 1 currently.
 */
int
linphonec_parse_command_line(LinphoneCore *lc, char *cl)
{
	char *ptr=cl;
	char *args=NULL;
	LPC_COMMAND *cmd;

	/* Isolate first word and args */
	while(*ptr && !isspace(*ptr)) ++ptr;
	if (*ptr)
	{
		*ptr='\0';
 		/* set args to first nonblank */
		args=ptr+1;
		while(*args && isspace(*args)) ++args;
	}

	/* Handle DTMF */
	if ( isdigit(*cl) || *cl == '#' || *cl == '*' )
	{
		while ( isdigit(*cl) || *cl == '#' || *cl == '*' )
		{
			if (linphone_core_get_current_call(lc))
				linphone_call_send_dtmf(linphone_core_get_current_call(lc), *cl);
			linphone_core_play_dtmf (lc,*cl,100);
			ms_sleep(1); // be nice
			++cl;
		}

		// discard spurious trailing chars
		return 1;
	}

	/* Handle other kind of commands */
	cmd=lpc_find_command(cl);
	if ( !cmd )
	{
		linphonec_out("'%s': Cannot understand this.\n", cl);
		return 1;
	}

	if ( ! cmd->func(lc, args) )
	{
		linphonec_out("Syntax error.\n");
		linphonec_display_command_help(cmd);
	}

	return 1;
}

/*
 * Generator function for command completion.
 * STATE let us know whether to start from scratch;
 * without any state (STATE==0), then we start at the
 * top of the list.
 */
char *
linphonec_command_generator(const char *text, int state)
{
	static int index, len, adv;
	char *name;

	if ( ! state )
	{
		index=0;
		adv=0;
		len=strlen(text);
	}
	/*
 	 * Return the next name which partially matches
	 * from the commands list
	 */
	if (adv==0){
		while ((name=commands[index].name))
		{
			++index; /* so next call get next command */

			if (strncmp(name, text, len) == 0)
			{
				return ortp_strdup(name);
			}
		}
		adv=1;
		index=0;
	}
	if (adv==1){
		while ((name=advanced_commands[index].name))
		{
			++index; /* so next call get next command */

			if (strncmp(name, text, len) == 0)
			{
				return ortp_strdup(name);
			}
		}
	}
	return NULL;
}


/***************************************************************************
 *
 *  Command handlers
 *
 ***************************************************************************/

static int
lpc_cmd_help(LinphoneCore *lc, char *arg)
{
	int i=0;
	LPC_COMMAND *cmd;

	if (!arg || !*arg)
	{
		linphonec_out("Commands are:\n");
		linphonec_out("---------------------------\n");

		while (commands[i].help)
		{
			linphonec_out("%10.10s\t%s\n", commands[i].name,
				commands[i].help);
			i++;
		}

		linphonec_out("---------------------------\n");
		linphonec_out("Type 'help <command>' for more details or\n");
		linphonec_out("     'help advanced' to list additional commands.\n");

		return 1;
	}

	if (strcmp(arg,"advanced")==0){
		linphonec_out("Advanced commands are:\n");
		linphonec_out("---------------------------\n");
		i=0;
		while (advanced_commands[i].help)
		{
			linphonec_out("%20.20s\t%s\n", advanced_commands[i].name,
				advanced_commands[i].help);
			i++;
		}

		linphonec_out("---------------------------\n");
		linphonec_out("Type 'help <command>' for more details.\n");

		return 1;
	}

	cmd=lpc_find_command(arg);
	if ( !cmd )
	{
		linphonec_out("No such command.\n");
		return 1;
	}

	linphonec_display_command_help(cmd);
	return 1;

}

static char callee_name[256]={0};
static char caller_name[256]={0};


static int
lpc_cmd_call(LinphoneCore *lc, char *args)
{
	if ( ! args || ! *args )
	{
		return 0;
	}
	{
		LinphoneCall *call;
		LinphoneCallParams *cp=linphone_core_create_call_params (lc, NULL);
		char *opt1,*opt2;
		if ( linphone_core_in_call(lc) )
		{
			linphonec_out("Terminate or hold on the current call first.\n");
			return 1;
		}
		opt1=strstr(args,"--audio-only");
		opt2=strstr(args,"--early-media");
		if (opt1){
			opt1[0]='\0';
			while(--opt1 > args && opt1[0]==' ') opt1[0]='\0';
			linphone_call_params_enable_video (cp,FALSE);
		}
		if (opt2){
			opt2[0]='\0';
			while(--opt2 > args && opt2[0]==' ') opt2[0]='\0';
			linphone_call_params_enable_early_media_sending(cp,TRUE);
		}
		if ( NULL == (call=linphone_core_invite_with_params(lc, args,cp)) )
		{
			linphonec_out("Error from linphone_core_invite.\n");
		}
		else
		{
			snprintf(callee_name,sizeof(callee_name),"%s",args);
		}
		linphone_call_params_unref(cp);
	}
	return 1;
}

static int
lpc_cmd_calls(LinphoneCore *lc, char *args){
	const bctbx_list_t *calls = linphone_core_get_calls(lc);
	if(calls)
	{
		lpc_display_call_states(lc);
	}else
	{
		linphonec_out("No active call.\n");
	}
	return 1;
}


static int
lpc_cmd_chat(LinphoneCore *lc, char *args)
{
	char *arg1 = args;
	char *arg2 = NULL;
	char *ptr = args;
	LinphoneChatRoom *cr;

	if (!args) return 0;

	/* Isolate first and second arg */
	while(*ptr && !isspace(*ptr)) ++ptr;
	if ( *ptr )
	{
		*ptr='\0';
		arg2=ptr+1;
		while(*arg2 && isspace(*arg2)) ++arg2;
	}
	else
	{
		/* missing one parameter */
		return 0;
	}
	cr = linphone_core_get_chat_room_from_uri(lc,arg1);
	linphone_chat_room_send_message(cr,arg2);
	return 1;
}

const char *linphonec_get_callee(void){
	return callee_name;
}

const char *linphonec_get_caller(void){
	return caller_name;
}

void linphonec_set_caller(const char *caller){
	snprintf(caller_name,sizeof(caller_name)-1,"%s",caller);
}

static int
lpc_cmd_transfer(LinphoneCore *lc, char *args)
{
	if (args){
		LinphoneCall *call;
		LinphoneCall *call2;
		const char *refer_to=NULL;
		char arg1[256]={0};
		char arg2[266]={0};
		long id2=0;
		int n=sscanf(args,"%255s %265s %li",arg1,arg2,&id2);
		if (n==1 || isalpha(*arg1)){
			call=linphone_core_get_current_call(lc);
			if (call==NULL && bctbx_list_size(linphone_core_get_calls(lc))==1){
				call=(LinphoneCall*)linphone_core_get_calls(lc)->data;
			}
			refer_to=args;
			if (call==NULL){
				linphonec_out("No active call, please specify a call id among the ones listed by 'calls' command.\n");
				return 0;
			}
			linphone_core_transfer_call(lc, call, refer_to);
		}else if (n==2){
			long id=atoi(arg1);
			refer_to=args+strlen(arg1)+1;
			call=linphonec_get_call(id);
			if (call==NULL) return 0;
			linphone_core_transfer_call(lc, call, refer_to);
		}else if (n==3){
			long id=atoi(arg1);
			call=linphonec_get_call(id);
			call2=linphonec_get_call(id2);
			if (call==NULL || call2==NULL) return 0;
			if (strcmp(arg2,"--to-call")!=0){
				return 0;
			}
			linphonec_out("Performing attended transfer of call %i to call %i",id,id2);
			linphone_core_transfer_call_to_another (lc,call,call2);
		}else return 0;
	}else{
		linphonec_out("Transfer command requires at least one argument\n");
		return 0;
	}
	return 1;
}

static int
lpc_cmd_terminate(LinphoneCore *lc, char *args)
{
	if (linphone_core_get_calls(lc)==NULL){
		linphonec_out("No active calls\n");
		return 1;
	}
	if (!args)
	{
		if ( -1 == linphone_core_terminate_call(lc, NULL) ){
			linphonec_out("Could not stop the active call.\n");
		}
		return 1;
	}

	if(strcmp(args,"all")==0){
		linphonec_out("We are going to stop all the calls.\n");
		linphone_core_terminate_all_calls(lc);
		return 1;
	}else{
		/*the argument is a linphonec call id */
		long id=atoi(args);
		LinphoneCall *call=linphonec_get_call(id);
		if (call){
			if (linphone_core_terminate_call(lc,call)==-1){
				linphonec_out("Could not stop the call with id %li\n",id);
			}
		}else return 0;
		return 1;
	}
	return 0;

}

static int
lpc_cmd_redirect(LinphoneCore *lc, char *args){
	const bctbx_list_t *elem;
	int didit=0;
	if (!args) return 0;
	if ((elem=linphone_core_get_calls(lc))==NULL){
		linphonec_out("No active calls.\n");
		return 1;
	}
	if (strncmp(args, "all ", 4) == 0) {
		while(elem!=NULL){
			LinphoneCall *call=(LinphoneCall*)elem->data;
			if (linphone_call_get_state(call)==LinphoneCallIncomingReceived){
				if (linphone_core_redirect_call(lc,call,args+4) != 0) {
					linphonec_out("Could not redirect call.\n");
					elem=elem->next;
				} else {
					didit=1;
					/*as the redirection closes the call, we need to re-check the call list that is invalidated.*/
					elem=linphone_core_get_calls(lc);
				}
			}else elem=elem->next;
		}
		if (didit==0){
			linphonec_out("There is no pending incoming call to redirect.\n");
		}
	} else {
		char space;
		long id;
		int charRead;
		if ( sscanf(args, "%li%c%n", &id, &space, &charRead) == 2 && space == ' ') {
			LinphoneCall * call = linphonec_get_call(id);
			if ( call != NULL ) {
				if (linphone_call_get_state(call)!=LinphoneCallIncomingReceived) {
					linphonec_out("The state of the call is not incoming, can't be redirected.\n");
				} else if (linphone_core_redirect_call(lc,call,args+charRead) != 0) {
					linphonec_out("Could not redirect call.\n");
				}
			}
		} else return 0;
	}
	return 1;
}

static int
lpc_cmd_answer(LinphoneCore *lc, char *args){
	if (!args)
	{
		int nb=bctbx_list_size(linphone_core_get_calls(lc));
		if (nb==1){
			//if just one call is present answer the only one in passing NULL to the linphone_core_accept_call ...
			if ( -1 == linphone_core_accept_call(lc, NULL) )
			{
				linphonec_out("Fail to accept incoming call\n");
			}
		}else if (nb==0){
			linphonec_out("There are no calls to answer.\n");
		}else{
			linphonec_out("Multiple calls in progress, please specify call id.\n");
			return 0;
		}
		return 1;
	}else{
		long id;
		if (sscanf(args,"%li",&id)==1){
			LinphoneCall *call=linphonec_get_call (id);
			if (linphone_core_accept_call (lc,call)==-1){
				linphonec_out("Fail to accept call %i\n",id);
			}
		}else return 0;
		return 1;
	}
	return 0;
}

static int
lpc_cmd_autoanswer(LinphoneCore *lc, char *args)
{
	if ( ! args )
	{
		if ( linphonec_get_autoanswer() ) {
			linphonec_out("Auto answer is enabled. Use 'autoanswer disable' to disable.\n");
		} else {
			linphonec_out("Auto answer is disabled. Use 'autoanswer enable' to enable.\n");
		}
		return 1;
	}

	if (strstr(args,"enable")){
		linphonec_set_autoanswer(TRUE);
		linphonec_out("Auto answer enabled.\n");
	}else if (strstr(args,"disable")){
		linphonec_set_autoanswer(FALSE);
		linphonec_out("Auto answer disabled.\n");
	}else return 0;
	return 1;
}

static int
lpc_cmd_quit(LinphoneCore *lc, char *args)
{
	linphonec_main_loop_exit();
	return 1;
}

static int
lpc_cmd_nat(LinphoneCore *lc, char *args)
{
	bool_t use;
	const char *nat;

	if ( args ) args=lpc_strip_blanks(args);

	if ( args && *args )
	{
		linphone_core_set_nat_address(lc, args);
		/* linphone_core_set_firewall_policy(lc,LINPHONE_POLICY_USE_NAT_ADDRESS); */
	}

	nat = linphone_core_get_nat_address(lc);
	use = linphone_core_get_firewall_policy(lc)==LinphonePolicyUseNatAddress;
	linphonec_out("Nat address: %s%s\n", nat ? nat : "unspecified" , use ? "" : " (disabled - use 'firewall nat' to enable)");

	return 1;
}

static int
lpc_cmd_stun(LinphoneCore *lc, char *args)
{
	bool_t use;
	const char *stun;

	if ( args ) args=lpc_strip_blanks(args);

	if ( args && *args )
	{
		linphone_core_set_stun_server(lc, args);
		/* linphone_core_set_firewall_policy(lc,LINPHONE_POLICY_USE_STUN); */
	}

	stun = linphone_core_get_stun_server(lc);
	use = linphone_core_get_firewall_policy(lc)==LinphonePolicyUseStun;
	linphonec_out("Stun server: %s%s\n", stun ? stun : "unspecified" , use? "" : " (disabled - use 'firewall stun' to enable)");

	return 1;
}

static int
lpc_cmd_firewall(LinphoneCore *lc, char *args)
{
	const char* setting=NULL;

	if ( args ) args=lpc_strip_blanks(args);

	if ( args && *args )
	{
		if (strcmp(args,"none")==0)
		{
			linphone_core_set_firewall_policy(lc,LinphonePolicyNoFirewall);
		}
		else if (strcmp(args,"upnp")==0)
		{
			linphone_core_set_firewall_policy(lc,LinphonePolicyUseUpnp);
		}
		else if (strcmp(args,"ice")==0)
		{
			setting = linphone_core_get_stun_server(lc);
			if ( ! setting )
			{
				linphonec_out("No stun server address is defined, use 'stun <address>' first\n");
				return 1;
			}
			linphone_core_set_firewall_policy(lc,LinphonePolicyUseIce);
		}
		else if (strcmp(args,"stun")==0)
		{
			setting = linphone_core_get_stun_server(lc);
			if ( ! setting )
			{
				linphonec_out("No stun server address is defined, use 'stun <address>' first\n");
				return 1;
			}
			linphone_core_set_firewall_policy(lc,LinphonePolicyUseStun);
		}
		else if (strcmp(args,"nat")==0)
		{
			setting = linphone_core_get_nat_address(lc);
			if ( ! setting )
			{
				linphonec_out("No nat address is defined, use 'nat <address>' first");
				return 1;
			}
			linphone_core_set_firewall_policy(lc,LinphonePolicyUseNatAddress);
		}
	}

	switch(linphone_core_get_firewall_policy(lc))
	{
		case LinphonePolicyNoFirewall:
			linphonec_out("No firewall\n");
			break;
		case LinphonePolicyUseStun:
			linphonec_out("Using stun server %s to discover firewall address\n", setting ? setting : linphone_core_get_stun_server(lc));
			break;
		case LinphonePolicyUseNatAddress:
			linphonec_out("Using supplied nat address %s.\n", setting ? setting : linphone_core_get_nat_address(lc));
			break;
		case LinphonePolicyUseIce:
			linphonec_out("Using ice with stun server %s to discover firewall address\n", setting ? setting : linphone_core_get_stun_server(lc));
			break;
		case LinphonePolicyUseUpnp:
			linphonec_out("Using uPnP IGD protocol\n");
			break;
	}
	return 1;
}

#ifndef _WIN32
/* Helper function for processing freind names */
static int
lpc_friend_name(char **args, char **name)
{
	/* Use space as a terminator unless quoted */
	if (('"' == **args) || ('\'' == **args)){
		char *end;
		char delim = **args;
		(*args)++;
		end = (*args);
		while ((delim != *end) && ('\0' != *end)) end++;
		if ('\0' == *end) {
			fprintf(stderr, "Mismatched quotes\n");
			return 0;
		}
		*name = *args;
		*end = '\0';
		*args = ++end;
	} else {
		*name = strsep(args, " ");

		if (NULL == *args) { /* Means there was no separator */
			fprintf(stderr, "Either name or address is missing\n");
			return 0;
		}
		if (NULL == *name) return 0;
	}
	return 1;
}
#endif

static int
lpc_cmd_friend(LinphoneCore *lc, char *args)
{
	int friend_num;

	if ( args ) args=lpc_strip_blanks(args);

	if ( ! args || ! *args ) return 0;

	if ( !strncmp(args, "list", 4) )
	{
		return linphonec_friend_list(lc, args+4);
		return 1;
	}
	else if ( !strncmp(args, "call", 4) )
	{
		args+=4;
		if ( ! *args ) return 0;
		friend_num = strtol(args, NULL, 10);
#ifndef _WIN32_WCE
		if ( errno == ERANGE ) {
			linphonec_out("Invalid friend number\n");
			return 0;
		}
#endif /*_WIN32_WCE*/
		linphonec_friend_call(lc, friend_num);
		return 1;
	}
	else if ( !strncmp(args, "delete", 6) )
	{
		args+=6;
		if ( ! *args ) return 0;
		while (*args == ' ') args++;
		if ( ! *args ) return 0;
		if (!strncmp(args, "all", 3))
		{
			friend_num = -1;
		}
		else
		{
			friend_num = strtol(args, NULL, 10);
#ifndef _WIN32_WCE
			if ( errno == ERANGE ) {
				linphonec_out("Invalid friend number\n");
				return 0;
			}
#endif /*_WIN32_WCE*/
		}
		linphonec_friend_delete(lc, friend_num);
		return 1;
	}
	else if ( !strncmp(args, "add", 3) )
	{
#ifndef _WIN32
		char  *name;
		char  addr[80];
		char *addr_p = addr;
		char *addr_orig;

		args+=3;
		if ( ! *args ) return 0;
		while (*args == ' ') args++;
		if ( ! *args ) return 0;

		if (!lpc_friend_name(&args,  &name)) return 0;

		while (*args == ' ') args++;
		if ( ! *args ) return 0;
		if (isdigit(*args)) {
			strcpy (addr, "sip:");
			addr_p = addr + strlen("sip:");
		}
		addr_orig = strsep(&args, " ");
		if (1 >= strlen(addr_orig)) {
			fprintf(stderr, "A single-digit address is not valid\n");
			return 0;
		}
		strcpy(addr_p, addr_orig);
		linphonec_friend_add(lc, name, addr);
#else
		LinphoneFriend *new_friend;
		new_friend = linphone_core_create_friend_with_address(lc, args);
		linphone_core_add_friend(lc, new_friend);
#endif
		return 1;
	}
	return 0;
}

static int lpc_cmd_play(LinphoneCore *lc, char *args){
	if ( args ) args=lpc_strip_blanks(args);
	if ( ! args || ! *args ) return 0;
	linphone_core_set_play_file(lc,args);
	return 1;
}

static int lpc_cmd_record(LinphoneCore *lc, char *args){
	if ( args ) args=lpc_strip_blanks(args);
	if ( ! args || ! *args ) return 0;
	linphone_core_set_record_file(lc,args);
	return 1;
}

/*
 * Modified input
 */
static int
lpc_cmd_proxy(LinphoneCore *lc, char *args)
{
	char *arg1 = args;
	char *arg2 = NULL;
	char *ptr = args;
	int proxynum;

	if ( ! arg1 ) return 0;

	/* Isolate first and second arg */
	while(*ptr && !isspace(*ptr)) ++ptr;
	if ( *ptr )
	{
		*ptr='\0';
		arg2=ptr+1;
		while(*arg2 && isspace(*arg2)) ++arg2;
	}

	if (strcmp(arg1,"add")==0)
	{
#ifdef HAVE_READLINE
		rl_inhibit_completion=1;
#endif
		linphonec_proxy_add(lc);
#ifdef HAVE_READLINE
		rl_inhibit_completion=0;
#endif
	}
	else if (strcmp(arg1,"list")==0)
	{
		linphonec_proxy_list(lc);
	}
	else if (strcmp(arg1,"remove")==0)
	{
		if (arg2==NULL) return 0;
		linphonec_proxy_remove(lc,atoi(arg2));
	}
	else if (strcmp(arg1,"use")==0)
	{
		if ( arg2 && *arg2 )
		{
			proxynum=atoi(arg2);
			if ( linphonec_proxy_use(lc, proxynum) )
				linphonec_out("Default proxy set to %d.\n", proxynum);
		}
		else
		{
			proxynum=linphone_core_get_default_proxy(lc, NULL);
			if ( proxynum == -1 ) linphonec_out("No default proxy.\n");
			else linphonec_out("Current default proxy is %d.\n", proxynum);
		}
	}else if (strcmp(arg1, "unuse")==0){
		linphone_core_set_default_proxy(lc, NULL);
		linphonec_out("Use no proxy.\n");
	}

	else if (strcmp(arg1, "show")==0)
	{
		if (arg2 && *arg2)
		{
			if (strstr(arg2,"default"))
			{
				proxynum=linphone_core_get_default_proxy(lc, NULL);
				if ( proxynum < 0 ) {
					linphonec_out("No default proxy defined\n");
					return 1;
				}
				linphonec_proxy_show(lc,proxynum);
			}
			else
			{
				linphonec_proxy_show(lc, atoi(arg2));
			}
		}
		else return 0; /* syntax error */
	}

	else
	{
		return 0; /* syntax error */
	}

	return 1;
}

static int
lpc_cmd_call_logs(LinphoneCore *lc, char *args)
{
	const bctbx_list_t *elem=linphone_core_get_call_logs(lc);
	for (;elem!=NULL;elem=bctbx_list_next(elem))
	{
		LinphoneCallLog *cl=(LinphoneCallLog*)elem->data;
		char *str=linphone_call_log_to_str(cl);
		linphonec_out("%s\n",str);
		ms_free(str);
	}
	return 1;
}

static int
lpc_cmd_ipv6(LinphoneCore *lc, char *arg1)
{
	if ( ! arg1 )
	{
		return 0; /* syntax error */
	}

	if (strcmp(arg1,"status")==0)
	{
		linphonec_out("ipv6 use enabled: %s\n",linphone_core_ipv6_enabled(lc) ? "true":"false");
	}
	else if (strcmp(arg1,"enable")==0)
	{
		linphone_core_enable_ipv6(lc,TRUE);
		linphonec_out("ipv6 use enabled.\n");
	}
	else if (strcmp(arg1,"disable")==0)
	{
		linphone_core_enable_ipv6(lc,FALSE);
		linphonec_out("ipv6 use disabled.\n");
	}
	else
	{
		return 0; /* syntax error */
	}
	return 1;
}

static int devname_to_index(LinphoneCore *lc, const char *devname){
	const char **p;
	int i;
	for(i=0,p=linphone_core_get_sound_devices(lc);*p!=NULL;++p,++i){
		if (strcmp(devname,*p)==0) return i;
	}
	return -1;
}

static const char *index_to_devname(LinphoneCore *lc, int index){
	const char **p;
	int i;
	for(i=0,p=linphone_core_get_sound_devices(lc);*p!=NULL;++p,++i){
		if (i==index) return *p;
	}
	return NULL;
}

static int lpc_cmd_soundcard(LinphoneCore *lc, char *args)
{
	int i, index;
	const char **dev;
	char *arg1 = args;
	char *arg2 = NULL;
	char *ptr = args;

	if (!args) return 0; /* syntax error */

	/* Isolate first and second arg */
	while(*ptr && !isspace(*ptr)) ++ptr;
	if ( *ptr )
	{
		*ptr='\0';
		arg2=ptr+1;
		while(*arg2 && isspace(*arg2)) ++arg2;
	}

	if (strcmp(arg1, "list")==0)
	{
		dev=linphone_core_get_sound_devices(lc);
		for(i=0; dev[i]!=NULL; ++i){
			linphonec_out("%i: %s\n",i,dev[i]);
		}
		return 1;
	}

	if (strcmp(arg1, "show")==0)
	{
		if (linphone_core_get_use_files(lc)) {
			linphonec_out("Using files.\n");
		} else {
			linphonec_out("Ringer device: %s\n",
				linphone_core_get_ringer_device(lc));
			linphonec_out("Playback device: %s\n",
				linphone_core_get_playback_device(lc));
			linphonec_out("Capture device: %s\n",
				linphone_core_get_capture_device(lc));
		}
		return 1;
	}

	if (strcmp(arg1, "use")==0 && arg2)
	{
		if (strcmp(arg2, "files")==0)
		{
			linphonec_out("Using wav files instead of soundcard.\n");
			linphone_core_use_files(lc,TRUE);
			return 1;
		}

		linphone_core_use_files(lc,FALSE);

		dev=linphone_core_get_sound_devices(lc);
		index=atoi(arg2); /* FIXME: handle not-a-number */
		for(i=0;dev[i]!=NULL;i++)
		{
			if (i!=index) continue;

			linphone_core_set_ringer_device(lc,dev[i]);
			linphone_core_set_playback_device(lc,dev[i]);
			linphone_core_set_capture_device(lc,dev[i]);
			linphonec_out("Using sound device %s\n",dev[i]);
			return 1;
		}
		linphonec_out("No such sound device\n");
		return 1;
	}

	if (strcmp(arg1, "capture")==0)
	{
		const char *devname=linphone_core_get_capture_device(lc);
		if (!arg2){
			linphonec_out("Using capture device #%i (%s)\n",
					devname_to_index(lc,devname),devname);
		}else{
			index=atoi(arg2); /* FIXME: handle not-a-number */
			devname=index_to_devname(lc,index);
			if (devname!=NULL){
				linphone_core_set_capture_device(lc,devname);
				linphonec_out("Using capture sound device %s\n",devname);
				return 1;
			}
			linphonec_out("No such sound device\n");
		}
		return 1;
	}
	if (strcmp(arg1, "playback")==0)
	{
		const char *devname=linphone_core_get_playback_device(lc);
		if (!arg2){
			linphonec_out("Using playback device #%i (%s)\n",
					devname_to_index(lc,devname),devname);
		}else{
			index=atoi(arg2); /* FIXME: handle not-a-number */
			devname=index_to_devname(lc,index);
			if (devname!=NULL){
				linphone_core_set_playback_device(lc,devname);
				linphonec_out("Using playback sound device %s\n",devname);
				return 1;
			}
			linphonec_out("No such sound device\n");
		}
		return 1;
	}
	if (strcmp(arg1, "ring")==0)
	{
		const char *devname=linphone_core_get_ringer_device(lc);
		if (!arg2){
			linphonec_out("Using ring device #%i (%s)\n",
					devname_to_index(lc,devname),devname);
		}else{
			index=atoi(arg2); /* FIXME: handle not-a-number */
			devname=index_to_devname(lc,index);
			if (devname!=NULL){
				linphone_core_set_ringer_device(lc,devname);
				linphonec_out("Using ring sound device %s\n",devname);
				return 1;
			}
			linphonec_out("No such sound device\n");
		}
		return 1;
	}
	return 0; /* syntax error */
}

static int lpc_cmd_webcam(LinphoneCore *lc, char *args)
{
	int i, index;
	const char **dev;
	char *arg1 = args;
	char *arg2 = NULL;
	char *ptr = args;

	if (!args) return 0; /* syntax error */

	/* Isolate first and second arg */
	while(*ptr && !isspace(*ptr)) ++ptr;
	if ( *ptr )
	{
		*ptr='\0';
		arg2=ptr+1;
		while(*arg2 && isspace(*arg2)) ++arg2;
	}

	if (strcmp(arg1, "list")==0)
	{
		dev=linphone_core_get_video_devices(lc);
		for(i=0; dev[i]!=NULL; ++i){
			linphonec_out("%i: %s\n",i,dev[i]);
		}
		return 1;
	}

	if (strcmp(arg1, "use")==0 && arg2)
	{
		dev=linphone_core_get_video_devices(lc);
		index=atoi(arg2); /* FIXME: handle not-a-number */
		for(i=0;dev[i]!=NULL;i++)
		{
			if (i!=index) continue;

			linphone_core_set_video_device(lc, dev[i]);
			linphonec_out("Using video device %s\n",dev[i]);
			return 1;
		}
		linphonec_out("No such video device\n");
		return 1;
	}
	return 0; /* syntax error */
}

static int
lpc_cmd_staticpic(LinphoneCore *lc, char *args)
{
	char *arg1 = args;
	char *arg2 = NULL;
	char *ptr = args;

	if (!args) return 0;  /* Syntax error */

	/* Isolate first and second arg */
	while(*ptr && !isspace(*ptr)) ++ptr;
	if ( *ptr )
	{
		*ptr='\0';
		arg2=ptr+1;
		while(*arg2 && isspace(*arg2)) ++arg2;
	}

	if (strcmp(arg1, "set")==0 && arg2) {
		linphone_core_set_static_picture(lc, arg2);
		return 1;
	}

	if (strcmp(arg1, "fps")==0) {
	  if (arg2) {
	        float fps = (float)atof(arg2); /* FIXME: Handle not-a-float */
		linphone_core_set_static_picture_fps(lc, fps);
		return 1;
	  } else {
		float fps;
		fps = linphone_core_get_static_picture_fps(lc);
		linphonec_out("Current FPS %f\n", fps);
		return 1;
	  }
	}

	return 0; /* Syntax error */
}

static int lpc_cmd_pause(LinphoneCore *lc, char *args){

	if(linphone_core_in_call(lc))
	{
		linphone_core_pause_call(lc,linphone_core_get_current_call(lc));
		return 1;
	}
	linphonec_out("you can only pause when a call is in process\n");
    return 0;
}

static int lpc_cmd_resume(LinphoneCore *lc, char *args){

	if(linphone_core_in_call(lc))
	{
		linphonec_out("There is already a call in process pause or stop it first");
		return 1;
	}
	if (args)
	{
		long id;
		int n = sscanf(args, "%li", &id);
		if (n == 1){
			LinphoneCall *call=linphonec_get_call (id);
			if (call){
				if(linphone_core_resume_call(lc,call)==-1){
					linphonec_out("There was a problem to resume the call check the remote address you gave %s\n",args);
				}
			}
			return 1;
		}else return 0;
	}
	else
	{
		const bctbx_list_t *calls = linphone_core_get_calls(lc);
		int nbcalls=bctbx_list_size(calls);
		if( nbcalls == 1)
		{
			if(linphone_core_resume_call(lc,calls->data) < 0)
			{
				linphonec_out("There was a problem to resume the unique call.\n");
			}
			return 1;
		}else if (nbcalls==0){
			linphonec_out("There is no calls at this time.\n");
			return 1;
		}else{
			linphonec_out("There are %i calls at this time, please specify call id as given with 'calls' command.\n");
		}
	}
	return 0;

}

static int lpc_cmd_conference(LinphoneCore *lc, char *args){
	long id;
	char subcommand[32]={0};
	int n;
	if (args==NULL) return 0;
	n=sscanf(args, "%31s %li", subcommand,&id);
	if (n == 2){
		LinphoneCall *call=linphonec_get_call(id);
		if (call==NULL) return 1;
		if (strcmp(subcommand,"add")==0){
			linphone_core_add_to_conference(lc,call);
			return 1;
		}else if (strcmp(subcommand,"rm")==0){
			linphone_core_remove_from_conference(lc,call);
			return 1;
		}else if (strcmp(subcommand,"enter")==0){
			linphone_core_enter_conference(lc);
			return 1;
		}else if (strcmp(subcommand,"leave")==0){
			linphone_core_leave_conference(lc);
			return 1;
		}
	}
	return 0;
}

/***************************************************************************
 *
 *  Commands helper functions
 *
 ***************************************************************************/


static void
linphonec_proxy_add(LinphoneCore *lc)
{
	bool_t enable_register=FALSE;
	LinphoneProxyConfig *cfg;

	linphonec_out("Adding new proxy setup. Hit ^D to abort.\n");

	/*
	 * SIP Proxy address
	 */
	while (1)
	{
		char *input=linphonec_readline("Enter proxy sip address: ");
		char *clean;

		if ( ! input ) {
			linphonec_out("Aborted.\n");
			return;
		}

		/* Strip blanks */
		clean=lpc_strip_blanks(input);
		if ( ! *clean ) {
			free(input);
			continue;
		}

		cfg=linphone_core_create_proxy_config(lc);
		if (linphone_proxy_config_set_server_addr(cfg,clean)<0)
		{
			linphonec_out("Invalid sip address (sip:sip.domain.tld).\n");
			free(input);
			linphone_proxy_config_destroy(cfg);
			continue;
		}
		free(input);
		break;
	}

	/*
	 * SIP Proxy identity
	 */
	while (1)
	{
		char *input=linphonec_readline("Your identity for this proxy: ");
		char *clean;

		if ( ! input ) {
			linphonec_out("Aborted.\n");
			linphone_proxy_config_destroy(cfg);
			return;
		}

		/* Strip blanks */
		clean=lpc_strip_blanks(input);
		if ( ! *clean ) {
			free(input);
			continue;
		}

		linphone_proxy_config_set_identity(cfg, clean);
		if ( ! linphone_proxy_config_get_identity (cfg))
		{
			linphonec_out("Invalid identity (sip:name@sip.domain.tld).\n");
			free(input);
			continue;
		}
		free(input);
		break;
	}

	/*
	 * SIP Proxy enable register
	 */
	while (1)
	{
		char *input=linphonec_readline("Do you want to register on this proxy (yes/no): ");
		char *clean;

		if ( ! input ) {
			linphonec_out("Aborted.\n");
			linphone_proxy_config_destroy(cfg);
			return;
		}

		/* Strip blanks */
		clean=lpc_strip_blanks(input);
		if ( ! *clean ) {
			free(input);
			continue;
		}

		if ( ! strcmp(clean, "yes") ) enable_register=TRUE;
		else if ( ! strcmp(clean, "no") ) enable_register=FALSE;
		else {
			linphonec_out("Please answer with 'yes' or 'no'\n");
			free(input);
			continue;
		}
		linphone_proxy_config_enableregister(cfg, enable_register);
		free(input);
		break;
	}

	/*
	 * SIP Proxy registration expiration
	 */
	if ( enable_register==TRUE )
	{
		int expires=0;
		while (1)
		{
			char *input=linphonec_readline("Specify register expiration time"
				" in seconds (default is 600): ");

			if ( ! input ) {
				linphonec_out("Aborted.\n");
				linphone_proxy_config_destroy(cfg);
				return;
			}

			expires=atoi(input);
			if (expires==0) expires=600;

			linphone_proxy_config_set_expires(cfg, expires);
			linphonec_out("Expiration: %d seconds\n", linphone_proxy_config_get_expires (cfg));

			free(input);
			break;
		}
	}

	/*
	 * SIP proxy route
	 */
	while (1)
	{
		char *input=linphonec_readline("Specify route if needed: ");
		char *clean;

		if ( ! input ) {
			linphonec_out("Aborted.\n");
			linphone_proxy_config_destroy(cfg);
			return;
		}

		/* Strip blanks */
		clean=lpc_strip_blanks(input);
		if ( ! *clean ) {
			free(input);
			linphonec_out("No route specified.\n");
			break;
		}

		linphone_proxy_config_set_route(cfg, clean);
		if ( ! linphone_proxy_config_get_route(cfg) )
		{
			linphonec_out("Invalid route.\n");
			free(input);
			continue;
		}

		free(input);
		break;
	}

	/*
	 * Final confirmation
	 */
	while (1)
	{
		char *input;
		char *clean;

		linphonec_out("--------------------------------------------\n");
		linphonec_proxy_display(cfg);
		linphonec_out("--------------------------------------------\n");
		input=linphonec_readline("Accept the above proxy configuration (yes/no) ?: ");


		if ( ! input ) {
			linphonec_out("Aborted.\n");
			linphone_proxy_config_destroy(cfg);
			return;
		}

		/* Strip blanks */
		clean=lpc_strip_blanks(input);
		if ( ! *clean ) {
			free(input);
			continue;
		}

		if ( ! strcmp(clean, "yes") ) break;
		else if ( ! strcmp(clean, "no") )
		{
			linphonec_out("Declined.\n");
			linphone_proxy_config_destroy(cfg);
			free(input);
			return;
		}

		linphonec_out("Please answer with 'yes' or 'no'\n");
		free(input);
		continue;
	}


	linphone_core_add_proxy_config(lc,cfg);

	/* automatically set the last entered proxy as the default one */
	linphone_core_set_default_proxy(lc,cfg);

	linphonec_out("Proxy added.\n");
}

static void
linphonec_proxy_display(LinphoneProxyConfig *cfg)
{
	const char *route=linphone_proxy_config_get_route(cfg);
	const char *identity=linphone_proxy_config_get_identity(cfg);
	linphonec_out("sip address: %s\nroute: %s\nidentity: %s\nregister: %s\nexpires: %i\nregistered: %s\n",
			linphone_proxy_config_get_addr(cfg),
			(route!=NULL)? route:"",
			(identity!=NULL)?identity:"",
			linphone_proxy_config_register_enabled (cfg)?"yes":"no",
			linphone_proxy_config_get_expires (cfg),
			linphone_proxy_config_is_registered(cfg) ? "yes" : "no");
}

static void linphonec_proxy_show(LinphoneCore *lc, int index)
{
	const bctbx_list_t *elem;
	int i;
	for(elem=linphone_core_get_proxy_config_list(lc),i=0;elem!=NULL;elem=elem->next,++i){
		if (index==i){
			LinphoneProxyConfig *cfg=(LinphoneProxyConfig *)elem->data;
			linphonec_proxy_display(cfg);
			return;
		}
	}
	linphonec_out("No proxy with index %i\n", index);
}

static void
linphonec_proxy_list(LinphoneCore *lc)
{
	const bctbx_list_t *proxies;
	int n;
	int def=linphone_core_get_default_proxy(lc,NULL);

	proxies=linphone_core_get_proxy_config_list(lc);
	for(n=0;proxies!=NULL;proxies=bctbx_list_next(proxies),n++){
		if (n==def)
			linphonec_out("****** Proxy %i - this is the default one - *******\n",n);
		else
			linphonec_out("****** Proxy %i *******\n",n);
		linphonec_proxy_display((LinphoneProxyConfig*)proxies->data);
	}
	if ( ! n ) linphonec_out("No proxies defined\n");
}

static void
linphonec_proxy_remove(LinphoneCore *lc, int index)
{
	const bctbx_list_t *proxies;
	LinphoneProxyConfig *cfg;
	proxies=linphone_core_get_proxy_config_list(lc);
	cfg=(LinphoneProxyConfig*)bctbx_list_nth_data(proxies,index);
	if (cfg==NULL){
		linphonec_out("No such proxy.\n");
		return;
	}
	linphone_core_remove_proxy_config(lc,cfg);
	linphonec_out("Proxy %s removed.\n", linphone_proxy_config_get_addr(cfg));
}

static int
linphonec_proxy_use(LinphoneCore *lc, int index)
{
	const bctbx_list_t *proxies;
	LinphoneProxyConfig *cfg;
	proxies=linphone_core_get_proxy_config_list(lc);
	cfg=(LinphoneProxyConfig*)bctbx_list_nth_data(proxies,index);
	if (cfg==NULL){
		linphonec_out("No such proxy (try 'proxy list').");
		return 0;
	}
	linphone_core_set_default_proxy(lc,cfg);
	return 1;
}

static void
linphonec_friend_display(LinphoneFriend *fr)
{
	const LinphoneAddress *addr = linphone_friend_get_address(fr);
	char *str = NULL;

	linphonec_out("name: %s\n", linphone_friend_get_name(fr));
	if (addr) str = linphone_address_as_string_uri_only(addr);
	linphonec_out("address: %s\n", str);
	if (str) ms_free(str);
}

static int
linphonec_friend_list(LinphoneCore *lc, char *pat)
{
	const bctbx_list_t *friend;
	int n;

	if (pat) {
		pat=lpc_strip_blanks(pat);
		if (!*pat) pat = NULL;
	}

	friend = linphone_core_get_friend_list(lc);
	for(n=0; friend!=NULL; friend=bctbx_list_next(friend), ++n )
	{
		if ( pat ) {
			const char *name = linphone_friend_get_name((LinphoneFriend *)friend->data);
			if (name && !strstr(name, pat)) continue;
		}
		linphonec_out("****** Friend %i *******\n",n);
		linphonec_friend_display((LinphoneFriend*)friend->data);
	}

	return 1;
}

static int
linphonec_friend_call(LinphoneCore *lc, unsigned int num)
{
	const bctbx_list_t *friend = linphone_core_get_friend_list(lc);
	unsigned int n;
	char *addr_str;

	for(n=0; friend!=NULL; friend=bctbx_list_next(friend), ++n )
	{
		if ( n == num )
		{
			int ret;
			const LinphoneAddress *addr = linphone_friend_get_address((LinphoneFriend*)friend->data);
			if (addr) {
				addr_str = linphone_address_as_string(addr);
				ret=lpc_cmd_call(lc, addr_str);
				ms_free(addr_str);
				return ret;
			} else {
				linphonec_out("Friend %u does not have an address\n", num);
			}
		}
	}
	linphonec_out("No such friend %u\n", num);
	return 1;
}

#ifndef _WIN32
static int
linphonec_friend_add(LinphoneCore *lc, const char *name, const char *addr)
{
	LinphoneFriend *newFriend;

	char url[PATH_MAX];

	snprintf(url, PATH_MAX, "%s <%s>", name, addr);
	newFriend = linphone_core_create_friend_with_address(lc, url);
	linphone_core_add_friend(lc, newFriend);
	return 0;
}
#endif

static int
linphonec_friend_delete(LinphoneCore *lc, int num)
{
	const bctbx_list_t *friend = linphone_core_get_friend_list(lc);
	int n;

	for(n=0; friend!=NULL; friend=bctbx_list_next(friend), ++n )
	{
		if ( n == num )
		{
			linphone_core_remove_friend(lc, friend->data);
			return 0;
		}
	}

	if (-1 == num)
	{
		int i;
		for (i = 0 ; i < n ; i++)
			linphonec_friend_delete(lc, 0);
		return 0;
	}

	linphonec_out("No such friend %i\n", num);
	return 1;
}

static void
linphonec_display_command_help(LPC_COMMAND *cmd)
{
	if ( cmd->doc ) linphonec_out ("%s\n", cmd->doc);
	else linphonec_out("%s\n", cmd->help);
}


static int lpc_cmd_register(LinphoneCore *lc, char *args){
	char identity[512];
	char proxy[512];
	char passwd[512];
	LinphoneProxyConfig *cfg;
	const bctbx_list_t *elem;

	if (!args)
		{
			/* it means that you want to register the default proxy */
			LinphoneProxyConfig *cfg = linphone_core_get_default_proxy_config(lc);
			if (cfg)
			{
				if(!linphone_proxy_config_is_registered(cfg)) {
				linphone_proxy_config_enable_register(cfg,TRUE);
				linphone_proxy_config_done(cfg);
			}else{
				linphonec_out("default proxy already registered\n");
			}
			}else{
				linphonec_out("we do not have a default proxy\n");
				return 0;
			}
			return 1;
		}
	passwd[0]=proxy[0]=identity[0]='\0';
	sscanf(args,"%511s %511s %511s",identity,proxy,passwd);
	if (proxy[0]=='\0' || identity[0]=='\0'){
		linphonec_out("Missing parameters, see help register\n");
		return 1;
	}
	if (passwd[0]!='\0'){
		LinphoneAddress *from;
		LinphoneAuthInfo *info;
		if ((from=linphone_address_new(identity))!=NULL){
			info=linphone_auth_info_new(linphone_address_get_username(from),NULL,passwd,NULL,NULL,linphone_address_get_username(from));
			linphone_core_add_auth_info(lc,info);
			linphone_address_unref(from);
			linphone_auth_info_destroy(info);
		}
	}
	elem=linphone_core_get_proxy_config_list(lc);
	if (elem) {
		cfg=(LinphoneProxyConfig*)elem->data;
		linphone_proxy_config_edit(cfg);
	}
	else cfg=linphone_core_create_proxy_config(lc);
	linphone_proxy_config_set_identity(cfg,identity);
	linphone_proxy_config_set_server_addr(cfg,proxy);
	linphone_proxy_config_enable_register(cfg,TRUE);
	if (elem) linphone_proxy_config_done(cfg);
	else linphone_core_add_proxy_config(lc,cfg);
	linphone_core_set_default_proxy(lc,cfg);
	return 1;
}

static int lpc_cmd_unregister(LinphoneCore *lc, char *args){
	LinphoneProxyConfig *cfg = linphone_core_get_default_proxy_config(lc);
	if (cfg && linphone_proxy_config_is_registered(cfg)) {
		linphone_proxy_config_edit(cfg);
		linphone_proxy_config_enable_register(cfg,FALSE);
		linphone_proxy_config_done(cfg);
	}else{
		linphonec_out("unregistered\n");
	}
	return 1;
}

static int lpc_cmd_duration(LinphoneCore *lc, char *args){
	LinphoneCallLog *cl;
	const bctbx_list_t *elem=linphone_core_get_call_logs(lc);
	for(;elem!=NULL;elem=elem->next){
		if (elem->next==NULL){
			cl=(LinphoneCallLog*)elem->data;
			linphonec_out("%i seconds\n",linphone_call_log_get_duration(cl));
		}
	}
	return 1;
}

static int lpc_cmd_status(LinphoneCore *lc, char *args)
{
	LinphoneProxyConfig *cfg;

	if ( ! args ) return 0;
	cfg = linphone_core_get_default_proxy_config(lc);
	if (strstr(args,"register"))
	{
		if (cfg)
		{
			if (linphone_proxy_config_is_registered(cfg)){
				linphonec_out("registered, identity=%s duration=%i\n",
					linphone_proxy_config_get_identity(cfg),
					linphone_proxy_config_get_expires(cfg));
			}else if (linphone_proxy_config_register_enabled(cfg)){
				linphonec_out("registered=-1\n");
			}else linphonec_out("registered=0\n");
		}
		else linphonec_out("registered=0\n");
	}
	else if (strstr(args,"autoanswer"))
	{
		if (cfg && linphone_proxy_config_is_registered(cfg))
			linphonec_out("autoanswer=%i\n",linphonec_get_autoanswer());
		else linphonec_out("unregistered\n");
	}
	else if (strstr(args,"hook"))
	{
		LinphoneCall *call=linphone_core_get_current_call (lc);
		LinphoneCallState call_state=LinphoneCallIdle;
		if (call) call_state=linphone_call_get_state(call);

		switch(call_state){
			case LinphoneCallOutgoingInit:
				linphonec_out("hook=outgoing_init sip:%s\n",linphonec_get_callee());
			break;
			case LinphoneCallOutgoingProgress:
				linphonec_out("hook=dialing sip:%s\n",linphonec_get_callee());
			break;
			case LinphoneCallOutgoingRinging:
				linphonec_out("hook=ringing sip:%s\n",linphonec_get_callee());
			break;
			case LinphoneCallPaused:
				linphonec_out("hook=paused sip:%s\n",linphonec_get_callee());
			break;
			case LinphoneCallIdle:
				linphonec_out("hook=on-hook\n");
			break;
			case LinphoneCallStreamsRunning:
			case LinphoneCallConnected:
				if (linphone_call_get_dir(call)==LinphoneCallOutgoing){
					linphonec_out("Call out, hook=%s duration=%i, muted=%s rtp-xmit-muted=%s\n", linphonec_get_callee(),
					      linphone_core_get_current_call_duration(lc),
					      linphone_core_mic_enabled(lc) ? "no" : "yes",
					      linphone_core_is_rtp_muted(lc) ? "yes"  : "no");
				}else{
					linphonec_out("hook=answered duration=%i %s\n" ,
						linphone_core_get_current_call_duration(lc), linphonec_get_caller());
		 		}
				break;
			case LinphoneCallIncomingReceived:
				linphonec_out("Incoming call from %s\n",linphonec_get_caller());
				break;
			default:
				break;
		}

	}
	else return 0;

	return 1;
}

static int lpc_cmd_ports(LinphoneCore *lc, char *args)
{
	int port;
	if ( ! args ){
		linphonec_out("sip port = %i\naudio rtp port = %i\nvideo rtp port = %i\n",
			linphone_core_get_sip_port(lc),
			linphone_core_get_audio_port(lc),
			linphone_core_get_video_port(lc));
		return 1;
	}
	if (sscanf(args,"sip %i",&port)==1){
		linphonec_out("Setting sip port to %i\n",port);
		linphone_core_set_sip_port(lc,port);
	}else return 0;

	return 1;
}

static int lpc_cmd_param(LinphoneCore *lc, char *args)
{
	char section[20], param[20], value[50];
	const char *string;

	if (args == NULL) {
		return 0;
	}
	switch (sscanf(args,"%19s %19s %49s",section,param,value)) {
		// case 1 might show all current settings under a section
		case 2:
			string = lp_config_get_string(linphone_core_get_config(lc), section, param, "(undef)");
			linphonec_out("current value: %s\n", string);
			break;
		case 3:
			if (lp_config_get_string(linphone_core_get_config(lc), section, param, NULL) != NULL) {
				lp_config_set_string(linphone_core_get_config(lc), section, param, value);
			// no indication of existence
				linphonec_out("updated value: %s\n", value);
			} else {
				linphonec_out("only update of existing variables are allowed\n");
			}
			break;
		default:
			return 0;
    }
	return 1;
}

static int lpc_cmd_speak(LinphoneCore *lc, char *args){
#ifndef _WIN32
	char voice[64];
	char *sentence;
	char cl[128];
	char wavfile[128]="/tmp/linphonec-espeak-XXXXXX";
	int status;
	FILE *file;

    if (!args) return 0;
	memset(voice,0,sizeof(voice));
	sscanf(args,"%63s",voice);
	sentence=args+strlen(voice);

#ifdef __APPLE__
	mktemp(wavfile);
#else
	if (mkstemp(wavfile)==-1){
		ms_error("Could not create temporary filename: %s", strerror(errno));
		linphonec_out("An error occured, please consult logs for details.");
		return 1;
	}
#endif

	snprintf(cl,sizeof(cl),"espeak -v %s -s 100 -w %s --stdin",voice,wavfile);
	file=popen(cl,"w");
	if (file==NULL){
		ms_error("Could not open pipe to espeak !");
		return 1;
	}
	fprintf(file,"%s",sentence);
	status=pclose(file);
	if (WEXITSTATUS(status)==0){
		linphone_core_set_play_file(lc,wavfile);
	}else{
		linphonec_out("espeak command failed.");
	}
#else
	linphonec_out("Sorry, this command is not implemented in windows version.");
#endif
	return 1;
}

static int lpc_cmd_acodec(LinphoneCore *lc, char *args){
    return lpc_cmd_codec(AUDIO, lc, args);
}

static int lpc_cmd_vcodec(LinphoneCore *lc, char *args){
    return lpc_cmd_codec(VIDEO, lc, args);
}

static int lpc_cmd_codec(int type, LinphoneCore *lc, char *args){
	char *arg1 = args;
	char *arg2 = NULL;
	char *ptr = args;

	if (!args) return 0;

	/* Isolate first and second arg */
	while(*ptr && !isspace(*ptr)) ++ptr;
	if ( *ptr )
	{
		*ptr='\0';
		arg2=ptr+1;
		while(*arg2 && isspace(*arg2)) ++arg2;
	}

	if (strcmp(arg1,"enable")==0)
	{
#ifdef HAVE_READLINE
		rl_inhibit_completion=1;
#endif
        if (!strcmp(arg2,"all")) linphonec_codec_enable(type,lc,-1);
        else linphonec_codec_enable(type,lc,atoi(arg2));
#ifdef HAVE_READLINE
		rl_inhibit_completion=0;
#endif
	}
	else if (strcmp(arg1,"list")==0)
	{
		linphonec_codec_list(type,lc);
	}
	else if (strcmp(arg1,"disable")==0)
	{
        if (!strcmp(arg2,"all")) linphonec_codec_disable(type,lc,-1);
        else linphonec_codec_disable(type,lc,atoi(arg2));
	}
	else
	{
		return 0; /* syntax error */
	}

	return 1;
}

static void linphonec_codec_list(int type, LinphoneCore *lc){
	PayloadType *pt;
	int index=0;
	const bctbx_list_t *node=NULL;

    if (type == AUDIO) {
      node=linphone_core_get_audio_codecs(lc);
    } else if(type==VIDEO) {
      node=linphone_core_get_video_codecs(lc);
    }

	for(;node!=NULL;node=bctbx_list_next(node)){
		pt=(PayloadType*)(node->data);
        linphonec_out("%2d: %s (%d) %s\n", index, pt->mime_type, pt->clock_rate,
		    linphone_core_payload_type_enabled(lc,pt) ? "enabled" : "disabled");
		index++;
	}
}

static void linphonec_codec_enable(int type, LinphoneCore *lc, int sel_index){
	PayloadType *pt;
	int index=0;
	const bctbx_list_t *node=NULL;

	if (type == AUDIO) {
		node=linphone_core_get_audio_codecs(lc);
	} else if(type==VIDEO) {
		node=linphone_core_get_video_codecs(lc);
	}

    for(;node!=NULL;node=bctbx_list_next(node)){
        if (index == sel_index || sel_index == -1) {
		    pt=(PayloadType*)(node->data);
            linphone_core_enable_payload_type (lc,pt,TRUE);
            linphonec_out("%2d: %s (%d) %s\n", index, pt->mime_type, pt->clock_rate, "enabled");
        }
		index++;
	}
}

static void linphonec_codec_disable(int type, LinphoneCore *lc, int sel_index){
	PayloadType *pt;
	int index=0;
	const bctbx_list_t *node=NULL;

	if (type == AUDIO) {
		node=linphone_core_get_audio_codecs(lc);
	} else if(type==VIDEO) {
		node=linphone_core_get_video_codecs(lc);
	}

	for(;node!=NULL;node=bctbx_list_next(node)){
		if (index == sel_index || sel_index == -1) {
			pt=(PayloadType*)(node->data);
			linphone_core_enable_payload_type (lc,pt,FALSE);
			linphonec_out("%2d: %s (%d) %s\n", index, pt->mime_type, pt->clock_rate, "disabled");
		}
		index++;
	}
}

static int lpc_cmd_echocancellation(LinphoneCore *lc, char *args){
	char *arg1 = args;
	char *arg2 = NULL;
	char *ptr = args;
	LpConfig *config=linphone_core_get_config(lc);

	if (!args) return 0;

	/* Isolate first and second arg */
	while(*ptr && !isspace(*ptr)) ++ptr;
	if ( *ptr )
	{
		*ptr='\0';
		arg2=ptr+1;
		while(*arg2 && isspace(*arg2)) ++arg2;
	}

	if (strcmp(arg1,"on")==0){
        int delay, tail_len, frame_size;
        int n;

        linphone_core_enable_echo_cancellation(lc,1);

        if (arg2 != 0) {
            n = sscanf(arg2, "%d %d %d", &delay, &tail_len, &frame_size);

            if (n == 1) {
                lp_config_set_int(config,"sound","ec_delay",delay);
            }
            else if (n == 2) {
                lp_config_set_int(config,"sound","ec_delay",delay);
                lp_config_set_int(config,"sound","ec_tail_len",tail_len);
            }
            else if (n == 3) {
                lp_config_set_int(config,"sound","ec_delay",delay);
                lp_config_set_int(config,"sound","ec_tail_len",tail_len);
                lp_config_set_int(config,"sound","ec_framesize",frame_size);
            }
        }
    }
    else if (strcmp(arg1,"off")==0){
        linphone_core_enable_echo_cancellation(lc,0);
    }
    else if (strcmp(arg1,"show")==0){
        linphonec_out("echo cancellation is %s; delay %d, tail length %d, frame size %d\n",
            linphone_core_echo_cancellation_enabled(lc) ? "on" : "off",
            lp_config_get_int(config,"sound","ec_delay",0),
            lp_config_get_int(config,"sound","ec_tail_len",0),
            lp_config_get_int(config,"sound","ec_framesize",0));
    }
    else {
        return 0;
    }

    return 1;
}

static int lpc_cmd_echolimiter(LinphoneCore *lc, char *args){
	if (args){
		if (strcmp(args,"on")==0){
			linphone_core_enable_echo_limiter (lc,TRUE);
		}else if (strcmp(args,"off")==0){
			linphone_core_enable_echo_limiter (lc,FALSE);
		}
	}
	linphonec_out("Echo limiter is now %s.\n",linphone_core_echo_limiter_enabled (lc) ? "on":"off");
	return 1;
}

static int lpc_cmd_mute_mic(LinphoneCore *lc, char *args)
{
	linphone_core_enable_mic(lc, 0);
	return 1;
}

static int lpc_cmd_unmute_mic(LinphoneCore *lc, char *args){
	linphone_core_enable_mic(lc, 1);
	return 1;
}

static int lpc_cmd_playback_gain(LinphoneCore *lc, char *args)
{
	if (args){
	        linphone_core_set_playback_gain_db(lc, (float)atof(args));
        	return 1;
	}
	return 0;
}

static int lpc_cmd_rtp_no_xmit_on_audio_mute(LinphoneCore *lc, char *args)
{
	bool_t rtp_xmit_off=FALSE;
	char *status;

	if(args){
		if(strstr(args,"1"))rtp_xmit_off=TRUE;
		if(linphone_core_get_current_call (lc)==NULL)
			linphone_core_set_rtp_no_xmit_on_audio_mute(lc,rtp_xmit_off);
		else
			linphonec_out("nortp-on-audio-mute: call in progress - cannot change state\n");
	}
	rtp_xmit_off=linphone_core_get_rtp_no_xmit_on_audio_mute(lc);
	if (rtp_xmit_off) status="off";
	else status="on";
	linphonec_out("rtp transmit %s when audio muted\n",status);
	return 1;
}

#ifdef VIDEO_ENABLED
static int _lpc_cmd_video_window(LinphoneCore *lc, char *args, bool_t is_preview){
	char subcommand[64];
	long a,b;
	int err;
	VideoParams *params=is_preview ? &lpc_preview_params : &lpc_video_params;

	if (!args) return 0;
	err=sscanf(args,"%63s %ld %ld",subcommand,&a,&b);
	if (err>=1){
		if (strcmp(subcommand,"pos")==0){
			if (err<3) return 0;
			params->x=a;
			params->y=b;
			params->refresh=TRUE;
		}else if (strcmp(subcommand,"size")==0){
			if (err<3) return 0;
			params->w=a;
			params->h=b;
			params->refresh=TRUE;
		}else if (strcmp(subcommand,"show")==0){
			params->show=TRUE;
			params->refresh=TRUE;
			if (is_preview) linphone_core_enable_video_preview (lc,TRUE);
		}else if (strcmp(subcommand,"hide")==0){
			params->show=FALSE;
			params->refresh=TRUE;
			if (is_preview) linphone_core_enable_video_preview (lc,FALSE);
		}else if (strcmp(subcommand,"id")==0){
			if (err == 1){
				linphonec_out("vwindow id: 0x%p\n",is_preview ? linphone_core_get_native_preview_window_id (lc) :
				              linphone_core_get_native_video_window_id (lc));
				return 1;
			} else if (err != 2) return 0;
			params->wid=(void *)a;
			if (is_preview)
				linphone_core_set_native_preview_window_id(lc, (void *)a);
			else
				linphone_core_set_native_video_window_id(lc, (void *)a);
		}else if (is_preview==TRUE){
			if (strcmp(subcommand,"integrated")==0){
				linphone_core_use_preview_window (lc,FALSE);
			}else if (strcmp(subcommand,"standalone")==0){
				linphone_core_use_preview_window(lc,TRUE);
			}else return 0;
		}else return 0;
	}
	return 1;
}

static int lpc_cmd_video_window(LinphoneCore *lc, char *args){
	return _lpc_cmd_video_window(lc, args, FALSE);
}

static int lpc_cmd_preview_window(LinphoneCore *lc, char *args){
	return _lpc_cmd_video_window(lc, args, TRUE);
}
#endif

static void lpc_display_global_state(LinphoneCore *lc){
	linphonec_out("Global liblinphone state\n%s\n",
	              linphone_global_state_to_string(linphone_core_get_global_state(lc)));
}

static void lpc_display_call_states(LinphoneCore *lc){
	LinphoneCall *call;
	const bctbx_list_t *elem;
	char *tmp;
	linphonec_out("Call states\n"
	              "Id |            Destination              |      State      |    Flags   |\n"
	              "------------------------------------------------------------------------\n");
	elem=linphone_core_get_calls(lc);
	if (elem==NULL){
		linphonec_out("(empty)\n");
	}else{
		for(;elem!=NULL;elem=elem->next){
			const char *flag;
			bool_t in_conference;
			call=(LinphoneCall*)elem->data;
			in_conference=(linphone_call_get_conference(call) != NULL);
			tmp=linphone_call_get_remote_address_as_string (call);
			flag=in_conference ? "conferencing" : "";
			flag=linphone_call_has_transfer_pending(call) ? "transfer pending" : flag;
			linphonec_out("%-2i | %-35s | %-15s | %s\n",(int)(long)linphone_call_get_user_pointer(call),
						  tmp,linphone_call_state_to_string(linphone_call_get_state(call))+strlen("LinphoneCall"),flag);
			ms_free(tmp);
		}
	}
}

static void lpc_display_proxy_states(LinphoneCore *lc){
	const bctbx_list_t *elem;
	linphonec_out("Proxy registration states\n"
	              "           Identity                      |      State\n"
	              "------------------------------------------------------------\n");
	elem=linphone_core_get_proxy_config_list (lc);
	if (elem==NULL) linphonec_out("(empty)\n");
	else {
		for(;elem!=NULL;elem=elem->next){
			LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)elem->data;
			linphonec_out("%-40s | %s\n",linphone_proxy_config_get_identity (cfg),
						  linphone_registration_state_to_string(linphone_proxy_config_get_state(cfg)));
		}
	}
}

static int lpc_cmd_states(LinphoneCore *lc, char *args){
	if (args==NULL) {
		lpc_display_global_state(lc);
		lpc_display_call_states(lc);
		lpc_display_proxy_states(lc);
		return 1;
	}
	if (strcmp(args,"global")==0){
		lpc_display_global_state(lc);
		return 1;
	}
	if (strcmp(args,"proxies")==0){
		lpc_display_proxy_states(lc);
		return 1;
	}
	if (strcmp(args,"calls")==0){
		lpc_display_call_states(lc);
		return 1;
	}
	return 0;
}

#ifdef VIDEO_ENABLED
static int lpc_cmd_camera(LinphoneCore *lc, char *args){
	LinphoneCall *call=linphone_core_get_current_call(lc);
	bool_t activated=FALSE;

	if (linphone_core_video_enabled (lc)==FALSE){
		linphonec_out("Video is disabled, re-run linphonec with -V option.");
		return 1;
	}

	if (args){
		if (strcmp(args,"on")==0)
			activated=TRUE;
		else if (strcmp(args,"off")==0)
			activated=FALSE;
		else
			return 0;
	}

	if (call==NULL){
		if (args){
			linphonec_camera_enabled=activated;
		}
		if (linphonec_camera_enabled){
			linphonec_out("Camera is enabled. Video stream will be setup immediately for outgoing and incoming calls.\n");
		}else{
			linphonec_out("Camera is disabled. Calls will be established with audio-only, with the possibility to later add video using 'camera on'.\n");
		}
	}else{
		const LinphoneCallParams *cp=linphone_call_get_current_params (call);
		if (args){
			linphone_call_enable_camera(call,activated);
			if (linphone_call_get_state(call)==LinphoneCallStreamsRunning){
				if ((activated && !linphone_call_params_video_enabled (cp))){
					/*update the call to add the video stream*/
					LinphoneCallParams *ncp=linphone_call_params_copy(cp);
					linphone_call_params_enable_video(ncp,TRUE);
					linphone_core_update_call(lc,call,ncp);
					linphone_call_params_unref (ncp);
					linphonec_out("Trying to bring up video stream...\n");
				}
			}
		}
		if (linphone_call_camera_enabled (call))
				linphonec_out("Camera is allowed for current call.\n");
		else linphonec_out("Camera is dis-allowed for current call.\n");
	}
	return 1;
}

static int lpc_cmd_snapshot(LinphoneCore *lc, char *args){
	LinphoneCall *call;
	if (!args) return 0;
	call=linphone_core_get_current_call(lc);
	if (call!=NULL){
		linphone_call_take_video_snapshot(call,args);
		linphonec_out("Taking video snapshot in file %s\n", args);
	}else linphonec_out("There is no active call.\n");
	return 1;
}

static int lpc_cmd_preview_snapshot(LinphoneCore *lc, char *args){
	LinphoneCall *call;
	if (!args) return 0;
	call=linphone_core_get_current_call(lc);
	if (call!=NULL){
		linphone_call_take_preview_snapshot(call,args);
		linphonec_out("Taking video preview snapshot in file %s\n", args);
	}else linphonec_out("There is no active call.\n");
	return 1;
}

static int lpc_cmd_vfureq(LinphoneCore *lc, char *arg){
	LinphoneCall *call;
	call=linphone_core_get_current_call(lc);
	if (call!=NULL){
		linphone_call_send_vfu_request(call);
		linphonec_out("VFU request sent\n");
	}else linphonec_out("There is no active call.\n");
	return 1;
}
#endif

static int lpc_cmd_identify(LinphoneCore *lc, char *args){
	LinphoneCall *call;
	const char *remote_ua;
	if (args==NULL){
		call=linphone_core_get_current_call(lc);
		if (call==NULL) {
			linphonec_out("There is currently running call. Specify call id.\n");
			return 0;
		}
	}else{
		call=linphonec_get_call(atoi(args));
		if (call==NULL){
			return 0;
		}
	}
	remote_ua=linphone_call_get_remote_user_agent(call);
	if (remote_ua){
		linphonec_out("Remote user agent string is: %s\n",remote_ua);
	}
	return 1;
}

static int lpc_cmd_ringback(LinphoneCore *lc, char *args){
	if (!args) return 0;
	if (strcmp(args,"disable")==0){
		linphone_core_set_remote_ringback_tone(lc,NULL);
		linphonec_out("Disabling ringback tone.\n");
		return 1;
	}
	linphone_core_set_remote_ringback_tone (lc,args);
	linphonec_out("Using %s as ringback tone to be played to callers.",args);
	return 1;
}

static int zrtp_set_verified(LinphoneCore *lc, char *args, bool_t verified){
	LinphoneCall *call=linphone_core_get_current_call(lc);
	if (linphone_call_params_get_media_encryption(linphone_call_get_current_params(call))==LinphoneMediaEncryptionZRTP){
		linphone_call_set_authentication_token_verified(call,verified);
	}
	return 1;
}
static int lpc_cmd_zrtp_verified(LinphoneCore *lc, char *args){
	return zrtp_set_verified(lc,args,TRUE);
}
static int lpc_cmd_zrtp_unverified(LinphoneCore *lc, char *args){
	return zrtp_set_verified(lc,args,FALSE);
}

/***************************************************************************
 *
 *  Command table management funx
 *
 ***************************************************************************/

/*
 * Find a command given its name
 */
static LPC_COMMAND *
lpc_find_command(const char *name)
{
	int i;

	for (i=0; commands[i].name; ++i)
	{
		if (strcmp(name, commands[i].name) == 0)
			return &commands[i];
	}

	for (i=0; advanced_commands[i].name; ++i)
	{
		if (strcmp(name, advanced_commands[i].name) == 0)
			return &advanced_commands[i];
	}

	return (LPC_COMMAND *)NULL;
}


/****************************************************************************
 *
 * $Log: commands.c,v $
 * Revision 1.39  2008/07/03 15:08:34  smorlat
 * api cleanups, interface in progress.
 *
 * Revision 1.38  2008/06/17 20:38:59  smorlat
 * added missing file.
 *
 * Revision 1.37  2008/04/09 09:26:00  smorlat
 * merge various patches
 * H264 support.
 *
 * Revision 1.36  2007/08/01 14:47:53  strk
 *         * console/commands.c: Clean up commands 'nat', 'stun'
 *           and 'firewall' to be more intuitive.
 *
 * Revision 1.35  2007/06/27 09:01:25  smorlat
 * logging improvements.
 *
 * Revision 1.34  2007/02/20 10:17:13  smorlat
 * linphonec friends patch2
 *
 * Revision 1.31  2006/09/22 07:22:47  smorlat
 * linphonecore api changes.
 *
 * Revision 1.30  2006/09/08 15:32:57  smorlat
 * support for using files instead of soundcard (used by linphonec only)
 *
 * Revision 1.29  2006/08/28 14:29:07  smorlat
 * fix bug.
 *
 * Revision 1.28  2006/08/21 12:49:59  smorlat
 * merged several little patches.
 *
 * Revision 1.27  2006/07/17 18:45:00  smorlat
 * support for several event queues in ortp.
 * glib dependency removed from coreapi/ and console/
 *
 * Revision 1.26  2006/04/14 15:16:36  smorlat
 * soundcard use did nothing !
 *
 * Revision 1.25  2006/04/06 20:09:33  smorlat
 * add linphonec command to see and select sound devices.
 *
 * Revision 1.24  2006/03/04 11:17:10  smorlat
 * mediastreamer2 in progress.
 *
 * Revision 1.23  2006/02/20 21:14:01  strk
 * Handled syntax errors with 'friend' command
 *
 * Revision 1.22  2006/02/20 10:20:29  strk
 * Added substring-based filter support for command 'friend list'
 *
 * Revision 1.21  2006/02/02 15:39:18  strk
 * - Added 'friend list' and 'friend call' commands
 * - Allowed for multiple DTFM send in a single line
 * - Added status-specific callback (bare version)
 *
 * Revision 1.20  2006/01/26 11:54:34  strk
 * More robust 'nat' command handler (strip blanks in args)
 *
 * Revision 1.19  2006/01/26 09:48:05  strk
 * Added limits.h include
 *
 * Revision 1.18  2006/01/26 02:18:05  strk
 * Added new commands 'nat use' and 'nat unuse'.
 * These will required a pending patch to linphonecore.c
 * in order to work.
 *
 * Revision 1.17  2006/01/20 14:12:33  strk
 * Added linphonec_init() and linphonec_finish() functions.
 * Handled SIGINT and SIGTERM to invoke linphonec_finish().
 * Handling of auto-termination (-t) moved to linphonec_finish().
 * Reworked main (input read) loop to not rely on 'terminate'
 * and 'run' variable (dropped). configfile_name allocated on stack
 * using PATH_MAX limit. Changed print_usage signature to allow
 * for an exit_status specification.
 *
 * Revision 1.16  2006/01/18 09:25:32  strk
 * Command completion inhibited in proxy addition and auth request prompts.
 * Avoided use of linphonec_readline's internal filename completion.
 *
 * Revision 1.15  2006/01/14 13:29:32  strk
 * Reworked commands interface to use a table structure,
 * used by command line parser and help function.
 * Implemented first level of completion (commands).
 * Added notification of invalid "answer" and "terminate"
 * commands (no incoming call, no active call).
 * Forbidden "call" intialization when a call is already active.
 * Cleaned up all commands, adding more feedback and error checks.
 *
 * Revision 1.14  2006/01/13 13:00:29  strk
 * Added linphonec.h. Code layout change (added comments, forward decl,
 * globals on top, copyright notices and Logs). Handled out-of-memory
 * condition on history management. Removed assumption on sizeof(char).
 * Fixed bug in authentication prompt (introduced by linphonec_readline).
 * Added support for multiple authentication requests (up to MAX_PENDING_AUTH).
 *
 *
 ****************************************************************************/
