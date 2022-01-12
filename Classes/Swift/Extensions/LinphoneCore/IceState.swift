/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

import Foundation
import linphonesw

extension IceState {
	func toString()->String {
		switch (self) {
		case .NotActivated: return NSLocalizedString("Not activated", tableName:"ICE has not been activated for this call",comment : "")
		case .Failed: return NSLocalizedString("Failed",  tableName:"ICE processing has failed",comment :"")
		case .InProgress: return NSLocalizedString("In progress",  tableName:"ICE process is in progress",comment :"")
		case .HostConnection: return NSLocalizedString("Direct connection",  tableName:"ICE has established a direct connection to the remote host",comment :"")
		case .ReflexiveConnection: return NSLocalizedString( "NAT(s) connection", tableName:"ICE has established a connection to the remote host through one or several NATs",comment :"")
		case .RelayConnection: return NSLocalizedString("Relay connection",  tableName:"ICE has established a connection through a relay",comment :"")
		}
		
	}
}
