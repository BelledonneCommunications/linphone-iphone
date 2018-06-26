/*
 * local-conference-event-handler-p.h
 * Copyright (C) 2010-2018 Belledonne Communications SARL
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _L_LOCAL_CONFERENCE_EVENT_HANDLER_P_H_
#define _L_LOCAL_CONFERENCE_EVENT_HANDLER_P_H_

#include <string>

#include "chat/chat-room/chat-room-id.h"
#include "local-conference-event-handler.h"
#include "object/object-p.h"
#include "xml/conference-info.h"

LINPHONE_BEGIN_NAMESPACE

class Participant;
class ParticipantDevice;

class LINPHONE_INTERNAL_PUBLIC LocalConferenceEventHandlerPrivate : public ObjectPrivate {
public:
	void notifyFullState (const std::string &notify, const std::shared_ptr<ParticipantDevice> &device);
	void notifyAllExcept (const std::string &notify, const std::shared_ptr<Participant> &exceptParticipant);
	void notifyAll (const std::string &notify);
	std::string createNotifyFullState (int notifyId = -1, bool oneToOne = false);
	std::string createNotifyMultipart (int notifyId);
	std::string createNotifyParticipantAdded (const Address &addr, int notifyId = -1);
	std::string createNotifyParticipantAdminStatusChanged (const Address &addr, bool isAdmin, int notifyId = -1);
	std::string createNotifyParticipantRemoved (const Address &addr, int notifyId = -1);
	std::string createNotifyParticipantDeviceAdded (const Address &addr, const Address &gruu, int notifyId = -1);
	std::string createNotifyParticipantDeviceRemoved (const Address &addr, const Address &gruu, int notifyId = -1);
	std::string createNotifySubjectChanged (int notifyId = -1);

	inline unsigned int getLastNotify () const { return lastNotify; };

private:
	ChatRoomId chatRoomId;

	LocalConference *conf = nullptr;
	unsigned int lastNotify = 1;

	static void notifyResponseCb (const LinphoneEvent *ev);

	std::string createNotify (Xsd::ConferenceInfo::ConferenceType confInfo, int notifyId = -1, bool isFullState = false);
	std::string createNotifySubjectChanged (const std::string &subject, int notifyId = -1);
	void notifyParticipant (const std::string &notify, const std::shared_ptr<Participant> &participant);
	void notifyParticipantDevice (const std::string &notify, const std::shared_ptr<ParticipantDevice> &device, bool multipart = false);

	L_DECLARE_PUBLIC(LocalConferenceEventHandler);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_LOCAL_CONFERENCE_EVENT_HANDLER_P_H_
