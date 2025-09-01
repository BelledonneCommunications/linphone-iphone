/*
 * Copyright (c) 2010-2023 Belledonne Communications SARL.
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

import linphonesw
import SwiftUI

class HelpViewModel: ObservableObject {
	private let TAG = "[HelpViewModel]"

	@Published var logcat: Bool = false
	@Published var logText: String = ""
	@Published var version: String = ""
	@Published var appVersion: String = ""
	@Published var sdkVersion: String = ""
	@Published var firebaseProjectId: String = ""
	@Published var checkUpdateAvailable: Bool = false
	@Published var uploadLogsAvailable: Bool = false
	@Published var logsUploadInProgress: Bool = false
	@Published var versionAvailable: String = ""
	@Published var urlVersionAvailable: String = ""
	
	private var coreDelegate: CoreDelegate?

	init() {
		let appName = Bundle.main.infoDictionary?["CFBundleName"] as? String
		let versionTmp = Bundle.main.infoDictionary?["CFBundleShortVersionString"] as? String
		let build = Bundle.main.infoDictionary?["CFBundleVersion"] as? String
		
		self.version = (versionTmp ?? "6.0.0")
		
		self.sdkVersion = Core.getVersion
		
		if let path = Bundle.main.path(forResource: "GoogleService-Info", ofType: "plist"),
		   let plist = NSDictionary(contentsOfFile: path) as? [String: Any],
		   let projectID = plist["PROJECT_ID"] as? String {
			firebaseProjectId = projectID
		}
		
		CoreContext.shared.doOnCoreQueue { core in
			self.coreDelegate = CoreDelegateStub(
				onLogCollectionUploadStateChanged: {(
							core: Core,
							state: Core.LogCollectionUploadState,
							info: String
						) in
					if info.starts(with: "https") {
						DispatchQueue.main.async {
							self.logsUploadInProgress = false
							self.logText = info
						}
					}
				},
				onVersionUpdateCheckResultReceived: {(
					core: Core,
					result: VersionUpdateCheckResult,
					version: String?,
					url: String?
				) in
					switch result {
					case .NewVersionAvailable:
						if let version = version, let url = url {
							Log.info("\(self.TAG): Update available, version [\(version)], url [\(url)]")
							DispatchQueue.main.async {
								self.versionAvailable = version
								self.urlVersionAvailable = url
								self.checkUpdateAvailable = true
							}
						}
					case .UpToDate:
						Log.info("\(self.TAG): This version is up-to-date")
						DispatchQueue.main.async {
							ToastViewModel.shared.toastMessage = "Success_version_up_to_date"
							ToastViewModel.shared.displayToast = true
						}
					default:
						Log.info("\(self.TAG): Can't check for update, an error happened [\(result)]")
						DispatchQueue.main.async {
							ToastViewModel.shared.toastMessage = "Error"
							ToastViewModel.shared.displayToast = true
						}
					}
				}
			)
			core.addDelegate(delegate: self.coreDelegate!)
		}
	}
	
	deinit {
		if let delegate = coreDelegate {
			CoreContext.shared.doOnCoreQueue { core in
				core.removeDelegate(delegate: delegate)
			}
		}
	}
	
	/*
	func toggleLogcat() {
		let newValue = !self.logcat
		CoreContext.shared.doOnCoreQueue { core in
			CorePreferences.printLogsInLogcat = newValue
			Factory.Instance.enableLogcatLogs(newValue)
			self.logcat = newValue
		}
	}
	*/
	
	func shareLogs() {
		CoreContext.shared.doOnCoreQueue { core in
			Log.info("\(self.TAG) Uploading debug logs for sharing")
			core.uploadLogCollection()
			DispatchQueue.main.async {
				self.logsUploadInProgress = true
			}
		}
	}
	
	func cleanLogs() {
		CoreContext.shared.doOnCoreQueue { _ in
			Core.resetLogCollection()
			Log.info("\(self.TAG) Debug logs have been cleaned")
			DispatchQueue.main.async {
				ToastViewModel.shared.toastMessage = "Success_clear_logs"
				ToastViewModel.shared.displayToast = true
			}
		}
	}

	func checkForUpdate() {
		let currentVersion = Bundle.main.infoDictionary?["CFBundleShortVersionString"] as? String
		CoreContext.shared.doOnCoreQueue { core in
			Log.info("\(self.TAG) Checking for update using current version \(currentVersion ?? "6.0.0")")
			core.checkForUpdate(currentVersion: currentVersion ?? "6.0.0")
		}
	}

	/*
	func showConfigFile() {
		CoreContext.shared.doOnCoreQueue { core in
			Log.i("\(self.TAG) Dumping & displaying Core's config")
			let config = core.config.dump()
			let file = FileUtils.getFileStorageCacheDir(
				"linphonerc.txt",
				overrideExisting: true
			)
			DispatchQueue.main.async {
				if FileUtils.dumpStringToFile(config, file: file) {
					Log.i("\(self.TAG) .linphonerc string saved as file in cache folder")
					self.showConfigFileEvent = Event(value: file.absolutePath)
				} else {
					Log.e("\(self.TAG) Failed to save .linphonerc string as file in cache folder")
				}
			}
		}
	}

	func clearNativeFriendsDatabase() {
		CoreContext.shared.doOnCoreQueue { core in
			if let list = core.getFriendListByName(NATIVE_ADDRESS_BOOK_FRIEND_LIST) {
				let friends = list.friends
				Log.i("\(self.TAG) Friend list to remove found with [\(friends.count)] friends")
				for friend in friends {
					list.removeFriend(friend)
				}
				core.removeFriendList(list)
				Log.i("\(self.TAG) Friend list [\(NATIVE_ADDRESS_BOOK_FRIEND_LIST)] removed")
			}
		}
	}
	*/
}
