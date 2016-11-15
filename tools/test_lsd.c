/*
linphone
Copyright (C) 2010 Simon MORLAT (simon.morlat@linphone.org)

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/* Linphone Sound Daemon: is a lightweight utility to play sounds to speaker during a conversation.
 This is useful for embedded platforms, where sound apis are not performant enough to allow
 simultaneous sound access.

 This file is a test program that plays several sound files and places a call simultatenously.
*/

#include "linphone/core_utils.h"

static void play_finished(LsdPlayer *p) {
	const char *filename = (const char *)lsd_player_get_user_pointer(p);
	ms_message("Playing of %s is finished.", filename);
	if (!lsd_player_loop_enabled(p)) {
		linphone_sound_daemon_release_player(lsd_player_get_daemon(p), p);
	}
}

static void wait_a_bit(LinphoneCore *lc, int seconds) {
	time_t orig = ms_time(NULL);
	while (ms_time(NULL) - orig < seconds) {
		/* we need to call iterate to receive notifications */
		linphone_core_iterate(lc);
		ms_usleep(50000);
	}
}

int main(int argc, char *argv[]) {
	LinphoneCore *lc;
	LinphoneCoreVTable vtable = {0};
	LinphoneSoundDaemon *lsd;
	LsdPlayer *p;

	linphone_core_enable_logs(stdout);
	lc = linphone_core_new(&vtable, NULL, NULL, NULL);
	lsd = linphone_sound_daemon_new(linphone_core_get_ms_factory(lc), NULL, 44100, 1);

	linphone_core_use_sound_daemon(lc, lsd);

	/* start a play */
	p = linphone_sound_daemon_get_player(lsd);
	lsd_player_set_callback(p, play_finished);
	lsd_player_set_user_pointer(p, "share/hello8000.wav");
	lsd_player_play(p, "share/hello8000.wav");
	wait_a_bit(lc, 2);

	/*start another one */
	p = linphone_sound_daemon_get_player(lsd);
	lsd_player_set_callback(p, play_finished);
	lsd_player_set_user_pointer(p, "share/hello16000.wav");
	lsd_player_enable_loop(p, TRUE);
	lsd_player_play(p, "share/hello16000.wav");

	/* after a few seconds decrease the volume */
	wait_a_bit(lc, 3);
	lsd_player_set_gain(p, 0.3);
	wait_a_bit(lc, 5);

	/*now play some stereo music*/
	p = linphone_sound_daemon_get_player(lsd);
	lsd_player_set_callback(p, play_finished);
	lsd_player_set_user_pointer(p, "share/rings/rock.wav");
	lsd_player_play(p, "share/rings/rock.wav");
	wait_a_bit(lc, 2);

	/*now play some stereo music at 22khz in order to test
	 stereo resampling */
	p = linphone_sound_daemon_get_player(lsd);
	lsd_player_set_callback(p, play_finished);
	lsd_player_set_user_pointer(p, "share/rings/bigben.wav");
	lsd_player_play(p, "share/rings/bigben.wav");
	wait_a_bit(lc, 6);

	/* now place an outgoing call if sip address argument is given */
	if (argc > 1) {
		linphone_core_invite(lc, argv[1]);
		wait_a_bit(lc, 10);
		linphone_core_terminate_call(lc, NULL);
	}
	linphone_core_use_sound_daemon(lc, NULL);
	linphone_sound_daemon_destroy(lsd);
	linphone_core_destroy(lc);

	return 0;
}
