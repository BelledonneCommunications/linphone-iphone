/*
 * conference.h
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

#ifndef _L_CONFERENCE_H_
#define _L_CONFERENCE_H_

#include "linphone/types.h"

#include "conference/conference-interface.h"
#include "conference/conference-listener.h"
#include "core/core-accessor.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class CallSession;
class CallSessionListener;
class CallSessionPrivate;
class ConferencePrivate;
class Content;
class ParticipantDevice;

class LINPHONE_PUBLIC Conference :
	public ConferenceInterface,
	public ConferenceListener,
	public CoreAccessor {
	friend class CallSessionPrivate;

public:
	~Conference();

	std::shared_ptr<Participant> getActiveParticipant () const;

	std::shared_ptr<Participant> findParticipant (const std::shared_ptr<const CallSession> &session) const;
	std::shared_ptr<ParticipantDevice> findParticipantDevice (const std::shared_ptr<const CallSession> &session) const;

	/* ConferenceInterface */
	void addParticipant (const IdentityAddress &addr, const CallSessionParams *params, bool hasMedia) override;
	void addParticipants (const std::list<IdentityAddress> &addresses, const CallSessionParams *params, bool hasMedia) override;
	bool canHandleParticipants () const override;
	std::shared_ptr<Participant> findParticipant (const IdentityAddress &addr) const override;
	const IdentityAddress &getConferenceAddress () const override;
	std::shared_ptr<Participant> getMe () const override;
	int getParticipantCount () const override;
	const std::list<std::shared_ptr<Participant>> &getParticipants () const override;
	const std::string &getSubject () const override;
	void join () override;
	void leave () override;
	void removeParticipant (const std::shared_ptr<Participant> &participant) override;
	void removeParticipants (const std::list<std::shared_ptr<Participant>> &participants) override;
	void setParticipantAdminStatus (const std::shared_ptr<Participant> &participant, bool isAdmin) override;
	void setSubject (const std::string &subject) override;

	std::string getResourceLists (const std::list<IdentityAddress> &addresses) const;
	static std::list<IdentityAddress> parseResourceLists (const Content &content);

protected:
	explicit Conference (
		ConferencePrivate &p,
		const std::shared_ptr<Core> &core,
		const IdentityAddress &myAddress,
		CallSessionListener *listener
	);

	bool isMe (const IdentityAddress &addr) const;

	ConferencePrivate *mPrivate = nullptr;

private:
	L_DECLARE_PRIVATE(Conference);
	L_DISABLE_COPY(Conference);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_CONFERENCE_H_
