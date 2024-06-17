/*
 * Copyright (c) 2010-2024 Belledonne Communications SARL.
 *
 * This file is part of Linphone
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
import SwiftUI

class AccountModel: ObservableObject {
	let account: Account
	@Published var humanReadableRegistrationState: String = ""
	@Published var registrationStateAssociatedUIColor: Color = .clear
	@Published var notificationsCount: Int = 0
	
	init(account: Account, corePublisher: CoreDelegatePublisher?) {
		self.account = account
		update()
		account.publisher?.onRegistrationStateChanged?
			.postOnMainQueue { _ in
				self.update()
			}
		corePublisher?.onChatRoomRead?.postOnMainQueue(
			receiveValue: { _ in
				self.computeNotificationsCount()
		})
		corePublisher?.onMessagesReceived?.postOnMainQueue(
			receiveValue: { _ in
				self.computeNotificationsCount()
		})
		corePublisher?.onCallStateChanged?.postOnMainQueue(
			receiveValue: { _ in
				self.computeNotificationsCount()
		})
	}
	
	func update() {
		switch account.state {
		case .Cleared, .None:
			humanReadableRegistrationState = "drawer_menu_account_connection_status_cleared".localized()
			registrationStateAssociatedUIColor = .orangeWarning600
		case .Progress:
			humanReadableRegistrationState = "drawer_menu_account_connection_status_progress".localized()
			registrationStateAssociatedUIColor = .greenSuccess500
		case .Failed: 
			humanReadableRegistrationState = "drawer_menu_account_connection_status_failed".localized()
			registrationStateAssociatedUIColor = .redDanger500
		case .Ok:
			humanReadableRegistrationState = "drawer_menu_account_connection_status_connected".localized()
			registrationStateAssociatedUIColor = .greenSuccess500
		case .Refreshing: 
			humanReadableRegistrationState = "drawer_menu_account_connection_status_refreshing".localized()
			registrationStateAssociatedUIColor = .grayMain2c500
		}
		computeNotificationsCount()
	}
	
	func computeNotificationsCount() {
		notificationsCount = account.unreadChatMessageCount + account.missedCallsCount
	}
	
	func refreshRegiter() {
		CoreContext.shared.doOnCoreQueue { _ in
			self.account.refreshRegister()
		}
	}
}
