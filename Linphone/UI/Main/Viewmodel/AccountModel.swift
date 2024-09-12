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
import Combine

class AccountModel: ObservableObject {
	let account: Account
	@Published var humanReadableRegistrationState: String = ""
	@Published var registrationStateAssociatedUIColor: Color = .clear
	@Published var notificationsCount: Int = 0
	@Published var isDefaultAccount: Bool = false
	@Published var displayName: String = ""
	@Published var address: String = ""
	
	private var accountDelegate: AccountDelegate?
	private var coreDelegate: CoreDelegate?
	
	init(account: Account, core: Core) {
		self.account = account
		
		accountDelegate = AccountDelegateStub(onRegistrationStateChanged: { (_: Account, _: RegistrationState, _: String) in
			self.update()
		})
		account.addDelegate(delegate: accountDelegate!)
		
		coreDelegate = CoreDelegateStub(onCallStateChanged: { (_: Core, _: Call, _: Call.State, _: String) in
			self.computeNotificationsCount()
		}, onMessagesReceived: { (_: Core, _: ChatRoom, _: [ChatMessage]) in
			self.computeNotificationsCount()
		}, onChatRoomRead: { (_: Core, _: ChatRoom) in
			self.computeNotificationsCount()
		})
		core.addDelegate(delegate: coreDelegate!)
		
		CoreContext.shared.doOnCoreQueue { _ in
			self.update()
		}
	}
	
	deinit {
		if let delegate = accountDelegate {
			account.removeDelegate(delegate: delegate)
		}
		if let delegate = coreDelegate {
			CoreContext.shared.doOnCoreQueue { core in
				core.removeDelegate(delegate: delegate)
			}
		}
	}
	
	private func update() {
		let state = account.state
		var isDefault: Bool = false
		if let defaultAccount = account.core?.defaultAccount {
			isDefault = (defaultAccount == account)
		}
		let displayName = account.displayName()
		let address = account.params?.identityAddress?.asString()
		DispatchQueue.main.async { [self] in
			switch state {
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
			isDefaultAccount = isDefault
			self.displayName = displayName
			address.map {self.address = $0}
		}
	}
	
	private func computeNotificationsCount() {
		let count = account.unreadChatMessageCount + account.missedCallsCount
		DispatchQueue.main.async { [self] in
			notificationsCount = count
		}
	}
	
	func refreshRegiter() {
		CoreContext.shared.doOnCoreQueue { _ in
			self.account.refreshRegister()
		}
	}
}
