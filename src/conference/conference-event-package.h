/*
 * conference-event-package.h
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

#ifndef _CONFERENCE_EVENT_PACKAGE_H_
#define _CONFERENCE_EVENT_PACKAGE_H_

#include <string>

#include "object/object.h"
#include "conference-listener.h"
#include "linphone/core.h"

namespace LinphonePrivate {
  namespace Conference {
    class ConferenceEventPackagePrivate;

    // -------------------------------------------------------------------------
    // ConferenceEventPackage.
    // -------------------------------------------------------------------------
    class ConferenceEventPackage : public Object {
    public:
      ConferenceEventPackage(LinphoneCore *lc, ConferenceListener *listener, LinphoneAddress *confAddr);
      ~ConferenceEventPackage();
      void subscribe(std::string confId);
      void notifyReceived(const char *xmlBody);
      void unsubscribe();
      std::string getConfId();

    private:
      L_DECLARE_PRIVATE(ConferenceEventPackage);
      L_DISABLE_COPY(ConferenceEventPackage);
    };
  }
}

#endif // ifndef _CONFERENCE_EVENT_PACKAGE_H_
