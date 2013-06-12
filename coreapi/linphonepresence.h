/*
linphonepresence.h
Copyright (C) 2010-2013  Belledonne Communications SARL

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

#ifndef LINPHONEPRESENCE_H_
#define LINPHONEPRESENCE_H_

#ifdef __cplusplus
extern "C" {
#endif


/** Basic status as defined in section 4.1.4 of RFC 3863 */
typedef enum LinphonePresenceBasicStatus {
	LinphonePresenceBasicStatusOpen,
	LinphonePresenceBasicStatusClosed
} LinphonePresenceBasicStatus;

/** Activities as defined in section 3.2 of RFC 4480 */
typedef enum LinphonePresenceActivity {
	LinphonePresenceActivityAppointment,
	LinphonePresenceActivityAway,
	LinphonePresenceActivityBreakfast,
	LinphonePresenceActivityBusy,
	LinphonePresenceActivityDinner,
	LinphonePresenceActivityHoliday,
	LinphonePresenceActivityInTransit,
	LinphonePresenceActivityLookingForWork,
	LinphonePresenceActivityLunch,
	LinphonePresenceActivityMeal,
	LinphonePresenceActivityMeeting,
	LinphonePresenceActivityOnThePhone,
	LinphonePresenceActivityOther,
	LinphonePresenceActivityPerformance,
	LinphonePresenceActivityPermanentAbsence,
	LinphonePresenceActivityPlaying,
	LinphonePresenceActivityPresentation,
	LinphonePresenceActivityShopping,
	LinphonePresenceActivitySleeping,
	LinphonePresenceActivitySpectator,
	LinphonePresenceActivitySteering,
	LinphonePresenceActivityTravel,
	LinphonePresenceActivityTV,
	LinphonePresenceActivityUnknown,
	LinphonePresenceActivityVacation,
	LinphonePresenceActivityWorking,
	LinphonePresenceActivityWorship
} LinphonePresenceActivity;

struct _LinphonePresenceModel;
typedef struct _LinphonePresenceModel LinphonePresenceModel;


LINPHONE_PUBLIC LinphonePresenceModel * linphone_presence_model_new(void);
LINPHONE_PUBLIC void linphone_presence_model_delete(LinphonePresenceModel *model);
LINPHONE_PUBLIC LinphonePresenceBasicStatus linphone_presence_model_get_basic_status(const LinphonePresenceModel *model);
LINPHONE_PUBLIC unsigned int linphone_presence_model_nb_activities(const LinphonePresenceModel *model);
LINPHONE_PUBLIC int linphone_presence_model_get_activity(const LinphonePresenceModel *model, unsigned int idx, LinphonePresenceActivity *activity, char **description);


#ifdef __cplusplus
}
#endif

#endif /* LINPHONEPRESENCE_H_ */
