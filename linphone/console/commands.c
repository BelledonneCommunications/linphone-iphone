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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 ****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <unistd.h>
#include <linphonecore.h>
#include "linphonec.h"
#include "private.h"
#include "lpconfig.h"

#ifndef WIN32
#include <sys/wait.h>
#endif

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
static int lpc_cmd_answer(LinphoneCore *, char *);
static int lpc_cmd_autoanswer(LinphoneCore *, char *);
static int lpc_cmd_terminate(LinphoneCore *, char *);
static int lpc_cmd_call_logs(LinphoneCore *, char *);
static int lpc_cmd_ipv6(LinphoneCore *, char *);
static int lpc_cmd_refer(LinphoneCore *, char *);
static int lpc_cmd_quit(LinphoneCore *, char *);
static int lpc_cmd_nat(LinphoneCore *, char *);
static int lpc_cmd_stun(LinphoneCore *, char *);
static int lpc_cmd_firewall(LinphoneCore *, char *);
static int lpc_cmd_friend(LinphoneCore *, char*);
static int lpc_cmd_soundcard(LinphoneCore *, char *);
static int lpc_cmd_play(LinphoneCore *, char *);
static int lpc_cmd_record(LinphoneCore *, char *);
static int lpc_cmd_register(LinphoneCore *, char *);
static int lpc_cmd_unregister(LinphoneCore *, char *);
static int lpc_cmd_duration(LinphoneCore *lc, char *args);
static int lpc_cmd_status(LinphoneCore *lc, char *args);
static int lpc_cmd_ports(LinphoneCore *lc, char *args);
static int lpc_cmd_speak(LinphoneCore *lc, char *args);
static int lpc_cmd_codec(LinphoneCore *lc, char *args);
static int lpc_cmd_echocancellation(LinphoneCore *lc, char *args);

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
#ifndef WIN32
static int linphonec_friend_add(LinphoneCore *lc, const char *name, const char *addr);
#endif
static int linphonec_friend_delete(LinphoneCore *lc, int num);
static int linphonec_friend_delete(LinphoneCore *lc, int num);
static void linphonec_codec_list(LinphoneCore *lc);
static void linphonec_codec_enable(LinphoneCore *lc, int index);
static void linphonec_codec_disable(LinphoneCore *lc, int index);



/* Command table management */
static LPC_COMMAND *lpc_find_command(const char *name);

void linphonec_out(const char *fmt,...);



/***************************************************************************
 *
 *  Global variables
 *
 ***************************************************************************/

/*
 * Commands table.
 */
LPC_COMMAND commands[] = {
	{ "help", lpc_cmd_help, "Print commands help", NULL },
	{ "call", lpc_cmd_call, "Call a SIP uri",
		"'call <sip-url>' "
		": initiate a call to the specified destination."
		},
	{ "terminate", lpc_cmd_terminate, "Terminate the current call",
		NULL },
	{ "answer", lpc_cmd_answer, "Answer a call",
		"Accept an incoming call."
	},
	{ "autoanswer", lpc_cmd_autoanswer, "Show/set auto-answer mode",
		"'autoanswer'       \t: show current autoanswer mode\n"
		"'autoanswer enable'\t: enable autoanswer mode\n"
		"'autoanswer disable'\t: disable autoanswer mode \n"},
	{ "proxy", lpc_cmd_proxy, "Manage proxies",
		"'proxy list' : list all proxy setups.\n"
		"'proxy add' : add a new proxy setup.\n"
		"'proxy remove <index>' : remove proxy setup with number index.\n"
		"'proxy use <index>' : use proxy with number index as default proxy.\n"
		"'proxy unuse' : don't use a default proxy.\n"
		"'proxy show <index>' : show configuration and status of the proxy numbered by index.\n"
		"'proxy show default' : show configuration and status of the default proxy.\n"
	},
	{ "soundcard", lpc_cmd_soundcard, "Manage soundcards",
		"'soundcard list' : list all sound devices.\n"
		"'soundcard show' : show current sound devices configuration.\n"
		"'soundcard use <index>' : select a sound device.\n"
		"'soundcard use files' : use .wav files instead of soundcard\n"
	},
	{ "ipv6", lpc_cmd_ipv6, "Use IPV6",
		"'ipv6 status' : show ipv6 usage status.\n"
		"'ipv6 enable' : enable the use of the ipv6 network.\n"
		"'ipv6 disable' : do not use ipv6 network."
	},
	{ "refer", lpc_cmd_refer,
		"Refer the current call to the specified destination.",
		"'refer <sip-url>' or 'r <sip-url>' "
		": refer the current call to the specified destination."
	},
	{ "nat", lpc_cmd_nat, "Set nat address",
		"'nat'        : show nat settings.\n"
		"'nat <addr>' : set nat address.\n"
	},
	{ "stun", lpc_cmd_stun, "Set stun server address",
		"'stun'        : show stun settings.\n"
		"'stun <addr>' : set stun server address.\n"
	},
	{ "firewall", lpc_cmd_firewall, "Set firewall policy",
		"'firewall'        : show current firewall policy.\n"
		"'firewall none'   : use direct connection.\n"
		"'firewall nat'    : use nat address given with the 'nat' command.\n"
		"'firewall stun'   : use stun server given with the 'stun' command.\n"
	},
	{ "call-logs", lpc_cmd_call_logs, "Calls history", NULL },
	{ "friend", lpc_cmd_friend, "Manage friends",
		"'friend list [<pattern>]'    : list friends.\n"
		"'friend call <index>'        : call a friend.\n"
		"'friend add <name> <addr>'   : add friend, <name> must be quoted to include\n"
	    "                               spaces, <addr> has \"sip:\" added if it isn't\n"
	    "                               there.  Don't use '<' '>' around <addr>.\n"
		"'friend delete <index>'      : remove friend, 'all' removes all\n"
	},
	{ "play", lpc_cmd_play, "play from a wav file",
		"This feature is available only in file mode (see 'help soundcard')\n"
		"'play <wav file>'    : play a wav file."
	},
	{ "record", lpc_cmd_record, "record to a wav file",
		"This feature is available only in file mode (see 'help soundcard')\n"
		"'record <wav file>'    : record into wav file."
	},
	{ "quit", lpc_cmd_quit, "Exit linphonec", NULL },
	{ "register", lpc_cmd_register, "Register in one line to a proxy" , "register <sip identity> <sip proxy> <password>"},
	{ "unregister", lpc_cmd_unregister, "Unregister from default proxy", NULL	},
	{ "duration", lpc_cmd_duration, "Print duration in seconds of the last call.", NULL },
	{ "status", lpc_cmd_status, "Print various status information", 
			"'status register'  \t: print status concerning registration\n"
			"'status autoanswer'\t: tell whether autoanswer mode is enabled\n"
			"'status hook'      \t: print hook status\n" },
	{ "ports", lpc_cmd_ports, "Network ports configuration", 
			"'ports'  \t: prints current used ports.\n"
			"'ports sip <port number>'\t: Sets the sip port.\n" },
	{ "speak", lpc_cmd_speak, "Speak a sentence using espeak TTS engine",
			"This feature is available only in file mode. (see 'help soundcard')\n"
			"'speak <voice name> <sentence>'	: speak a text using the specified espeak voice.\n"
			"Example for english voice: 'speak default Hello my friend !'"
	},
    { "codec", lpc_cmd_codec, "Codec configuration",
            "'codec list' : list codecs\n"  
            "'codec enable <index>' : enable available codec\n"  
            "'codec disable <index>' : disable codecs" }, 
    { "ec", lpc_cmd_echocancellation, "Echo cancellation",
            "'ec on [<delay>] [<tail>] [<framesize>]' : turn EC on with given delay, tail length and framesize\n"
            "'ec off' : turn echo cancellation (EC) off\n"
            "'ec show' : show EC status" },
	{ (char *)NULL, (lpc_cmd_handler)NULL, (char *)NULL, (char *)NULL }
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
			linphone_core_send_dtmf(lc, *cl);
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
	static int index, len;
	char *name;

	if ( ! state )
	{
		index=0;
		len=strlen(text);
	}

	/*
 	 * Return the next name which partially matches
	 * from the commands list
	 */
	while ((name=commands[index].name))
	{
		++index; /* so next call get next command */

		if (strncmp(name, text, len) == 0)
		{
			return strdup(name);
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

	if ( lc->call != NULL )
	{
		linphonec_out("Terminate current call first.\n");
	}
	else
	{
		if ( -1 == linphone_core_invite(lc, args) )
		{
			linphonec_out("Error from linphone_core_invite.\n");
		}
		else
		{
			snprintf(callee_name,sizeof(callee_name),"%s",args);
		}
	}
	return 1;
}

const char *linphonec_get_callee(){
	return callee_name;
}

const char *linphonec_get_caller(){
	return caller_name;
}

void linphonec_set_caller(const char *caller){
	snprintf(caller_name,sizeof(caller_name)-1,"%s",caller);
}

static int
lpc_cmd_refer(LinphoneCore *lc, char *args)
{
	if (args)
		linphone_core_refer(lc, args);
	else{
		linphonec_out("refer needs an argument\n");
	}
	return 1;
}

static int
lpc_cmd_terminate(LinphoneCore *lc, char *args)
{
	if ( -1 == linphone_core_terminate_call(lc, NULL) )
	{
		linphonec_out("No active call.\n");
	}
	return 1;
}

static int
lpc_cmd_answer(LinphoneCore *lc, char *args)
{
	if ( -1 == linphone_core_accept_call(lc, NULL) )
	{
		linphonec_out("No incoming call.\n");
	}
	return 1;
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
	use = linphone_core_get_firewall_policy(lc)==LINPHONE_POLICY_USE_NAT_ADDRESS;
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
	use = linphone_core_get_firewall_policy(lc)==LINPHONE_POLICY_USE_STUN;
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
			linphone_core_set_firewall_policy(lc,LINPHONE_POLICY_NO_FIREWALL);
		}
		else if (strcmp(args,"stun")==0)
		{
			setting = linphone_core_get_stun_server(lc);
			if ( ! setting )
			{
				linphonec_out("No stun server address is defined, use 'stun <address>' first\n");
				return 1;
			}
			linphone_core_set_firewall_policy(lc,LINPHONE_POLICY_USE_STUN);
		}
		else if (strcmp(args,"nat")==0)
		{
			setting = linphone_core_get_nat_address(lc);
			if ( ! setting )
			{
				linphonec_out("No nat address is defined, use 'nat <address>' first");
				return 1;
			}
			linphone_core_set_firewall_policy(lc,LINPHONE_POLICY_USE_NAT_ADDRESS);
		}
	}

	switch(linphone_core_get_firewall_policy(lc))
	{
		case LINPHONE_POLICY_NO_FIREWALL:
			linphonec_out("No firewall\n");
			break;
		case LINPHONE_POLICY_USE_STUN:
			linphonec_out("Using stun server %s to discover firewall address\n", setting ? setting : linphone_core_get_stun_server(lc));
			break;
		case LINPHONE_POLICY_USE_NAT_ADDRESS:
			linphonec_out("Using supplied nat address %s.\n", setting ? setting : linphone_core_get_nat_address(lc));
			break;
	}
	return 1;
}

#ifndef WIN32
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
		if ( errno == ERANGE ) {
			linphonec_out("Invalid friend number\n");
			return 0;
		}
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
			if ( errno == ERANGE ) {
				linphonec_out("Invalid friend number\n");
				return 0;
			}
		}
		linphonec_friend_delete(lc, friend_num);
		return 1;
	}
	else if ( !strncmp(args, "add", 3) )
	{
#ifndef WIN32
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
		new_friend = linphone_friend_new_with_addr(args);
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
	const MSList *elem=linphone_core_get_call_logs(lc);
	for (;elem!=NULL;elem=ms_list_next(elem))
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
		linphonec_out("Ringer device: %s\n",
			linphone_core_get_ringer_device(lc));
		linphonec_out("Playback device: %s\n",
			linphone_core_get_playback_device(lc));
		linphonec_out("Capture device: %s\n",
			linphone_core_get_capture_device(lc));
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

		cfg=linphone_proxy_config_new();
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
		if ( ! cfg->reg_identity )
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
		long int expires=0;
		while (1)
		{
			char *input=linphonec_readline("Specify register expiration time"
				" in seconds (default is 600): ");

			if ( ! input ) {
				linphonec_out("Aborted.\n");
				linphone_proxy_config_destroy(cfg);
				return;
			}

			expires=strtol(input, (char **)NULL, 10);
			if ( expires == LONG_MIN || expires == LONG_MAX )
			{
				linphonec_out("Invalid value: %s\n", strerror(errno));
				free(input);
				continue;
			}

			linphone_proxy_config_expires(cfg, expires);
			linphonec_out("Expiration: %d seconds\n", cfg->expires);

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
		if ( ! cfg->reg_route )
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
	linphonec_out("sip address: %s\nroute: %s\nidentity: %s\nregister: %s\nexpires: %i\nregistered: %s\n",
			cfg->reg_proxy,
			(cfg->reg_route!=NULL)?cfg->reg_route:"",
			(cfg->reg_identity!=NULL)?cfg->reg_identity:"",
			(cfg->reg_sendregister)?"yes":"no",
			cfg->expires,
			linphone_proxy_config_is_registered(cfg) ? "yes" : "no");
}

static void linphonec_proxy_show(LinphoneCore *lc, int index)
{
	const MSList *elem;
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
	const MSList *proxies;
	int n;
	int def=linphone_core_get_default_proxy(lc,NULL);
	
	proxies=linphone_core_get_proxy_config_list(lc);
	for(n=0;proxies!=NULL;proxies=ms_list_next(proxies),n++){
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
	const MSList *proxies;
	LinphoneProxyConfig *cfg;
	proxies=linphone_core_get_proxy_config_list(lc);
	cfg=(LinphoneProxyConfig*)ms_list_nth_data(proxies,index);
	if (cfg==NULL){
		linphonec_out("No such proxy.\n");
		return;
	}
	linphone_core_remove_proxy_config(lc,cfg);
	linphonec_out("Proxy %s removed.\n", cfg->reg_proxy);
	linphone_proxy_config_destroy(cfg);
}

static int
linphonec_proxy_use(LinphoneCore *lc, int index)
{
	const MSList *proxies;
	LinphoneProxyConfig *cfg;
	proxies=linphone_core_get_proxy_config_list(lc);
	cfg=(LinphoneProxyConfig*)ms_list_nth_data(proxies,index);
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
	LinphoneAddress *uri=linphone_address_clone(linphone_friend_get_uri(fr));
	char *str;
	
	linphonec_out("name: %s\n", linphone_address_get_display_name(uri));
	linphone_address_set_display_name(uri,NULL);
	str=linphone_address_as_string(uri);
	linphonec_out("address: %s\n", str);
}

static int
linphonec_friend_list(LinphoneCore *lc, char *pat)
{
	const MSList *friend;
	int n;

	if (pat) {
		pat=lpc_strip_blanks(pat);
		if (!*pat) pat = NULL;
	}

	friend = linphone_core_get_friend_list(lc);
	for(n=0; friend!=NULL; friend=ms_list_next(friend), ++n )
	{
		if ( pat ) {
			const char *name = linphone_address_get_display_name(
			    linphone_friend_get_uri((LinphoneFriend*)friend->data));
			if (name && ! strstr(name, pat) ) continue;
		}
		linphonec_out("****** Friend %i *******\n",n);
		linphonec_friend_display((LinphoneFriend*)friend->data);
	}

	return 1;
}

static int
linphonec_friend_call(LinphoneCore *lc, unsigned int num)
{
	const MSList *friend = linphone_core_get_friend_list(lc);
	unsigned int n;
	char *addr;

	for(n=0; friend!=NULL; friend=ms_list_next(friend), ++n )
	{
		if ( n == num )
		{
			int ret;
			addr = linphone_address_as_string(linphone_friend_get_uri((LinphoneFriend*)friend->data));
			ret=lpc_cmd_call(lc, addr);
			ms_free(addr);
			return ret;
		}
	}
	linphonec_out("No such friend %u\n", num);
	return 1;
}

#ifndef WIN32
static int
linphonec_friend_add(LinphoneCore *lc, const char *name, const char *addr)
{
	LinphoneFriend *newFriend;

	char url[PATH_MAX];

	snprintf(url, PATH_MAX, "%s <%s>", name, addr);
	newFriend = linphone_friend_new_with_addr(url);
	linphone_core_add_friend(lc, newFriend);
	return 0;
}
#endif

static int
linphonec_friend_delete(LinphoneCore *lc, int num)
{
	const MSList *friend = linphone_core_get_friend_list(lc);
	unsigned int n;

	for(n=0; friend!=NULL; friend=ms_list_next(friend), ++n )
	{
		if ( n == num )
		{
			linphone_core_remove_friend(lc, friend->data);
			return 0;
		}
	}

	if (-1 == num) 
	{
		unsigned int i;
		for (i = 0 ; i < n ; i++)
			linphonec_friend_delete(lc, 0);
		return 0;
	}

	linphonec_out("No such friend %u\n", num);
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
	const MSList *elem;
    if (!args) return 0;
	passwd[0]=proxy[0]=identity[0]='\0';
	sscanf(args,"%s %s %s",identity,proxy,passwd);
	if (proxy[0]=='\0' || identity[0]=='\0'){
		linphonec_out("Missing parameters, see help register\n");
		return 1;
	}
	if (passwd[0]!='\0'){
		osip_from_t *from;
		LinphoneAuthInfo *info;
		osip_from_init(&from);
		if (osip_from_parse(from,identity)==0){
			char realm[128];
			snprintf(realm,sizeof(realm)-1,"\"%s\"",from->url->host);
			info=linphone_auth_info_new(from->url->username,NULL,passwd,NULL,NULL);
			linphone_core_add_auth_info(lc,info);
		}
		osip_from_free(from);
	}
	elem=linphone_core_get_proxy_config_list(lc);
	if (elem) {
		cfg=(LinphoneProxyConfig*)elem->data;
		linphone_proxy_config_edit(cfg);
	}
	else cfg=linphone_proxy_config_new();
	linphone_proxy_config_set_identity(cfg,identity);
	linphone_proxy_config_set_server_addr(cfg,proxy);
	linphone_proxy_config_enable_register(cfg,TRUE);
	if (elem) linphone_proxy_config_done(cfg);
	else linphone_core_add_proxy_config(lc,cfg);
	linphone_core_set_default_proxy(lc,cfg);
	return 1;
}

static int lpc_cmd_unregister(LinphoneCore *lc, char *args){
	LinphoneProxyConfig *cfg=NULL;
	linphone_core_get_default_proxy(lc,&cfg);
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
	const MSList *elem=linphone_core_get_call_logs(lc);
	for(;elem!=NULL;elem=elem->next){
		if (elem->next==NULL){
			cl=(LinphoneCallLog*)elem->data;
			linphonec_out("%i seconds\n",cl->duration);
		}
	}
	return 1;
}

static int lpc_cmd_status(LinphoneCore *lc, char *args)
{
	if ( ! args ) return 0;

	LinphoneProxyConfig *cfg;
	linphone_core_get_default_proxy(lc,&cfg);
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
		gstate_t call_state=linphone_core_get_state(lc,GSTATE_GROUP_CALL);
/*
		if (!cfg || !linphone_proxy_config_is_registered(cfg)){
			linphonec_out("unregistered\n");
			return 1;
		}
 */
 		switch(call_state){
			case GSTATE_CALL_OUT_INVITE:
				linphonec_out("hook=dialing\n");
			break;
			case GSTATE_CALL_IDLE:
				linphonec_out("hook=offhook\n");
			break;
			case GSTATE_CALL_OUT_CONNECTED:
				linphonec_out("Call out, hook=%s duration=%i\n", linphonec_get_callee(),
					linphone_core_get_current_call_duration(lc));
			break;
			case GSTATE_CALL_IN_CONNECTED:
				linphonec_out("hook=answered duration=%i\n" ,
					linphone_core_get_current_call_duration(lc));
				break;
			case GSTATE_CALL_IN_INVITE:
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

static int lpc_cmd_speak(LinphoneCore *lc, char *args){
#ifndef WIN32
	char voice[64];
	char *sentence;
	char cl[128];
	char *wavfile;
	int status;
	FILE *file;
    if (!args) return 0;
	memset(voice,0,sizeof(voice));
	sscanf(args,"%s63",voice);
	sentence=args+strlen(voice);
	wavfile=tempnam("/tmp/","linphonec-espeak-");
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

static int lpc_cmd_codec(LinphoneCore *lc, char *args){
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
        if (!strcmp(arg2,"all")) linphonec_codec_enable(lc,-1);
        else linphonec_codec_enable(lc,atoi(arg2));
#ifdef HAVE_READLINE
		rl_inhibit_completion=0;
#endif
	}
	else if (strcmp(arg1,"list")==0)
	{
		linphonec_codec_list(lc);
	}
	else if (strcmp(arg1,"disable")==0)
	{
        if (!strcmp(arg2,"all")) linphonec_codec_disable(lc,-1);
        else linphonec_codec_disable(lc,atoi(arg2));
	}
	else
	{
		return 0; /* syntax error */
	}

	return 1;
}

static void linphonec_codec_list(LinphoneCore *lc){
	PayloadType *pt;
    codecs_config_t *config=&lc->codecs_conf;
	int index=0;
	MSList *node;
	for(node=config->audio_codecs;node!=NULL;node=ms_list_next(node)){
		pt=(PayloadType*)(node->data);
        linphonec_out("%2d: %s (%d) %s\n", index, pt->mime_type, pt->clock_rate, payload_type_enabled(pt) ? "enabled" : "disabled");
		index++;
	}
}

static void linphonec_codec_enable(LinphoneCore *lc, int sel_index){
	PayloadType *pt;
    codecs_config_t *config=&lc->codecs_conf;
	int index=0;
	MSList *node;
    for(node=config->audio_codecs;node!=NULL;node=ms_list_next(node)){
        if (index == sel_index || sel_index == -1) {
		    pt=(PayloadType*)(node->data);
            pt->flags|=PAYLOAD_TYPE_ENABLED;
            linphonec_out("%2d: %s (%d) %s\n", index, pt->mime_type, pt->clock_rate, "enabled");
        }
		index++;
	}
}

static void linphonec_codec_disable(LinphoneCore *lc, int sel_index){
	PayloadType *pt;
    codecs_config_t *config=&lc->codecs_conf;
	int index=0;
	MSList *node;
	for(node=config->audio_codecs;node!=NULL;node=ms_list_next(node)){
        if (index == sel_index || sel_index == -1) {
    		pt=(PayloadType*)(node->data);
            pt->flags&=~PAYLOAD_TYPE_ENABLED;
            linphonec_out("%2d: %s (%d) %s\n", index, pt->mime_type, pt->clock_rate, "disabled");
        }
		index++;
	}
}

static int lpc_cmd_echocancellation(LinphoneCore *lc, char *args){
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

	if (strcmp(arg1,"on")==0){
        int delay, tail_len, frame_size;
        int n;

        linphone_core_enable_echo_cancellation(lc,1);

        if (arg2 != 0) {
            n = sscanf(arg2, "%d %d %d", &delay, &tail_len, &frame_size);

            if (n == 1) {   
                lp_config_set_int(lc->config,"sound","ec_delay",delay);
            }
            else if (n == 2) {
                lp_config_set_int(lc->config,"sound","ec_delay",delay);
                lp_config_set_int(lc->config,"sound","ec_tail_len",tail_len);
            }
            else if (n == 3) {
                lp_config_set_int(lc->config,"sound","ec_delay",delay);
                lp_config_set_int(lc->config,"sound","ec_tail_len",tail_len);
                lp_config_set_int(lc->config,"sound","ec_framesize",frame_size);
            }
        }
    }
    else if (strcmp(arg1,"off")==0){
        linphone_core_enable_echo_cancellation(lc,0);
    }
    else if (strcmp(arg1,"show")==0){
        linphonec_out("echo cancellation is %s; delay %d, tail length %d, frame size %d\n", 
            linphone_core_echo_cancellation_enabled(lc) ? "on" : "off",
            lp_config_get_int(lc->config,"sound","ec_delay",0),
            lp_config_get_int(lc->config,"sound","ec_tail_len",0),
            lp_config_get_int(lc->config,"sound","ec_framesize",0));        
    }
    else {
        return 0;
    }

    return 1;
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
