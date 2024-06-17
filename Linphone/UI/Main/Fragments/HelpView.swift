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
				ToastViewModel.shared.toastMessage = "Success_clear_logs"
				ToastViewModel.shared.displayToast = true
			}
		}
	}
	
	static func logout() {
		CoreContext.shared.doOnCoreQueue { core in
			if core.defaultAccount != nil {
				let authInfo = core.defaultAccount!.findAuthInfo()
				if authInfo != nil {
					Log.info("$TAG Found auth info for account, removing it")
					core.removeAuthInfo(info: authInfo!)
				} else {
					Log.warn("$TAG Failed to find matching auth info for account")
				}

				core.removeAccount(account: core.defaultAccount!)
				Log.info("$TAG Account has been removed")
				
				DispatchQueue.main.async {
					CoreContext.shared.hasDefaultAccount = false
					CoreContext.shared.loggedIn = false
				}
			}
		}
	}
}
