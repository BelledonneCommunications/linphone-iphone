/*
linphone
Copyright (C) 2000  Simon MORLAT (simon.morlat@linphone.org)
Copyright (C) 2010  Belledonne Communications SARL

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

#include "linphone/linphonecore.h"

LinphoneRingtonePlayer* linphone_ringtoneplayer_ios_new();
void linphone_ringtoneplayer_ios_destroy(LinphoneRingtonePlayer* rp);
int linphone_ringtoneplayer_ios_start_with_cb(LinphoneRingtonePlayer* rp, const char* ringtone, int loop_pause_ms, LinphoneRingtonePlayerFunc end_of_ringtone, void * user_data);
bool_t linphone_ringtoneplayer_ios_is_started(LinphoneRingtonePlayer* rp);
int linphone_ringtoneplayer_ios_stop(LinphoneRingtonePlayer* rp);
