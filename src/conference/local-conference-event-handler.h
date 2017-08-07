/*
 * local-conference-event-handler.h
 * Copyright (C) 2017  Belledonne Communications SARL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _LOCAL_CONFERENCE_EVENT_HANDLER_H_
#define _LOCAL_CONFERENCE_EVENT_HANDLER_H_

#include <map>
#include <list>
#include <string>

#include "object/object.h"
#include "linphone/core.h"
#include "linphone/event.h"

namespace LinphonePrivate {
  namespace Conference {
    // -------------------------------------------------------------------------
    // LocalConferenceEventHandler.
    // -------------------------------------------------------------------------
    class LocalConference;
    class LocalConferenceEventHandlerPrivate;
    class LocalConferenceEventHandler : public Object {
    public:
      LocalConferenceEventHandler(LinphoneCore *core, LocalConference* localConf);
      ~LocalConferenceEventHandler();
      std::string subscribeReceived(LinphoneEvent *lev);
      std::string notifyParticipantAdded(LinphoneAddress *addr);
      std::string notifyParticipantRemoved(LinphoneAddress *addr);
      std::string notifyParticipantSetAdmin(LinphoneAddress *addr, bool isAdmin);

    private:
      L_DECLARE_PRIVATE(LocalConferenceEventHandler);
      L_DISABLE_COPY(LocalConferenceEventHandler);
    };

    class Participant {
    public:
      Participant(LinphoneAddress *addr, bool admin);
      ~Participant();
      bool isAdmin() const;
      const LinphoneAddress *getAddress() const;

    private:
      LinphoneAddress *mAddr;
      bool mAdmin;
    };

    class LocalConference {
    public:
      LocalConference(LinphoneCore *lc, LinphoneAddress *confAddr);
      ~LocalConference();
      const LinphoneAddress *getAddress() const;
      std::list<Participant> getParticipants() const;
      const std::shared_ptr<LocalConferenceEventHandler> getHandler() const;
      void addParticipant(Participant p);

    private:
      LinphoneAddress *mConfAddr;
      std::shared_ptr<LocalConferenceEventHandler> mHandler;
      std::list<Participant> mParticipants;
    };
  }
}


// -------- Conference::LocalConference public methods ---------
LinphonePrivate::Conference::LocalConference::LocalConference(LinphoneCore *core, LinphoneAddress *confAddr) {
  mConfAddr = confAddr;
  mHandler = std::make_shared<LinphonePrivate::Conference::LocalConferenceEventHandler>(core, this);
}

LinphonePrivate::Conference::LocalConference::~LocalConference() {
  //linphone_address_unref(mConfAddr);
}

const LinphoneAddress* LinphonePrivate::Conference::LocalConference::getAddress() const {
  return mConfAddr;
}


const std::shared_ptr<LinphonePrivate::Conference::LocalConferenceEventHandler> LinphonePrivate::Conference::LocalConference::getHandler() const {
  return mHandler;
}

std::list<LinphonePrivate::Conference::Participant> LinphonePrivate::Conference::LocalConference::getParticipants() const {
  return mParticipants;
}

void LinphonePrivate::Conference::LocalConference::addParticipant(LinphonePrivate::Conference::Participant p) {
  mParticipants.push_back(p);
}

LinphonePrivate::Conference::Participant::Participant(LinphoneAddress *addr, bool admin) {
  mAddr = addr;
  mAdmin = admin;
}

LinphonePrivate::Conference::Participant::~Participant() {
  //linphone_address_unref(mAddr);
}

bool LinphonePrivate::Conference::Participant::isAdmin() const {
  return mAdmin;
}

const LinphoneAddress* LinphonePrivate::Conference::Participant::getAddress() const {
  return mAddr;
}

#endif // ifndef _LOCAL_CONFERENCE_EVENT_HANDLER_H_