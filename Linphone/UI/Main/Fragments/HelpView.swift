/*
 * Copyright (c) 2010-2024 Belledonne Communications SARL.
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

class HelpView { // TODO (basic debug moved here until halp view is implemented)
	
	static func sendLogs() {
		CoreContext.shared.doOnCoreQueue { core in
			core.uploadLogCollection()
		}
	}
	
	static func clearLogs() {
		CoreContext.shared.doOnCoreQueue { _ in
			Core.resetLogCollection()
			DispatchQueue.main.async {
				ToastViewModel.shared.toastMessage = "help_troubleshooting_debug_logs_cleaned_toast_message"
				ToastViewModel.shared.displayToast = true
			}
		}
	}
	
	static func logout() {
		CoreContext.shared.doOnCoreQueue { core in
			if let account = core.defaultAccount {
				Log.info("Account \(account.displayName()) has been removed")
				core.removeAccount(account: account) // UI update and auth info removal moved into onRegistrationChanged core callback, in CoreContext
			}
		}
	}
}
