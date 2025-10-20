/*
 * Copyright (c) 2010-2023 Belledonne Communications SARL.
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

// swiftlint:disable line_length
// swiftlint:disable cyclomatic_complexity
// swiftlint:disable identifier_name

import linphonesw
import Combine
import UniformTypeIdentifiers
import Network
import SwiftUI

#if USE_CRASHLYTICS
import Firebase
#endif

class CoreContext: ObservableObject {
    
    static let shared = CoreContext()
    
	var pipViewModel = PIPViewModel()
	
	var coreVersion: String = Core.getVersion
	@Published var loggedIn: Bool = false
	@Published var loggingInProgress: Bool = false
	@Published var coreHasStartedOnce: Bool = false
	@Published var coreIsStarted: Bool = false
	@Published var accounts: [AccountModel] = []
	@Published var shortcuts: [ShortcutModel] = []
	var mCore: Core!
	
	var bearerAuthInfoPendingPasswordUpdate: AuthInfo?
	var imdnToEverybodyThreshold: Bool = true
	
	let monitor = NWPathMonitor()
	var networkStatusIsConnected: Bool = true // updated on core queue
	
	private var mCoreDelegate: CoreDelegate!
	private var actionsToPerformOnCoreQueueWhenCoreIsStarted: [((Core) -> Void)] = []
	private var callStateCallBacks: [((Call.State) -> Void)] = []
	private var configuringStateCallBacks: [((ConfiguringState) -> Void)] = []
	
	var digestAuthInfoPendingPasswordUpdate: AuthInfo?
	
	private init() {
		do {
			try initialiseCore()
		} catch {
			
		}
	}
	
	func doOnCoreQueue(synchronous: Bool = false, lambda: @escaping (Core) -> Void) {
		let isOnQueue = DispatchQueue.getSpecific(key: coreQueueKey) != nil

		let execute = {
			guard self.mCore.globalState != .Off else {
				Log.warn("Skipped execution: core is off")
				return
			}
			lambda(self.mCore)
		}

		switch (synchronous, isOnQueue) {
		case (true, true), (false, true):
			// Already on the queue → run directly
			execute()
		case (true, false):
			coreQueue.sync(execute: execute)
		case (false, false):
			coreQueue.async(execute: execute)
		}
	}
	
	private let coreQueueKey: DispatchSpecificKey<Void> = {
		let key = DispatchSpecificKey<Void>()
		coreQueue.setSpecific(key: key, value: ())
		return key
	}()
	
	func initialiseCore() throws {
		Log.info("Initialising core")
#if USE_CRASHLYTICS
		FirebaseApp.configure()
#endif
		monitor.pathUpdateHandler = { path in
			let isConnected = path.status == .satisfied
			if self.networkStatusIsConnected != isConnected {
				DispatchQueue.main.async {
					if isConnected {
						Log.info("Network is now satisfied")
						ToastViewModel.shared.toastMessage = "Success_toast_network_connected"
					} else {
						Log.error("Network is now \(path.status)")
						ToastViewModel.shared.toastMessage = "Unavailable_network"
					}
					ToastViewModel.shared.displayToast = true
				}
				self.networkStatusIsConnected = isConnected
			}
			
		}
		monitor.start(queue: coreQueue)
		
		coreQueue.async {
			LoggingService.Instance.logLevel = LogLevel.Debug
			Factory.Instance.logCollectionPath = Factory.Instance.getConfigDir(context: nil)
			Factory.Instance.enableLogCollection(state: LogCollectionState.Enabled)
			
			Log.info("Checking if linphonerc file exists already. If not, creating one as a copy of linphonerc-default")
			if let rcDir = FileManager.default.containerURL(forSecurityApplicationGroupIdentifier: Config.appGroupName)?
				.appendingPathComponent("Library/Preferences/linphone") {
				let rcFileUrl = rcDir.appendingPathComponent("linphonerc")
				if !FileManager.default.fileExists(atPath: rcFileUrl.path) {
					do {
						try FileManager.default.createDirectory(at: rcDir, withIntermediateDirectories: true)
						if let pathToDefaultConfig = Bundle.main.path(forResource: "linphonerc-default", ofType: nil) {
							try FileManager.default.copyItem(at: URL(fileURLWithPath: pathToDefaultConfig), to: rcFileUrl)
							Log.info("Successfully copied linphonerc-default configuration")
						}
					} catch let error {
						Log.error("Failed to copy default linphonerc file: \(error.localizedDescription)")
					}
				} else {
					Log.info("Found existing linphonerc file, skip copying of linphonerc-default configuration")
				}
			}
			
			self.mCore = try? Factory.Instance.createSharedCoreWithConfig(config: Config.get(), systemContext: Unmanaged.passUnretained(coreQueue).toOpaque(), appGroupId: Config.appGroupName, mainCore: true)
			
			self.mCore.callkitEnabled = true
			self.mCore.pushNotificationEnabled = true
			
			let appName = Bundle.main.infoDictionary?["CFBundleName"] as? String
			let version = Bundle.main.infoDictionary?["CFBundleShortVersionString"] as? String
			
			let userAgent = "LinphoneiOS/\(version ?? "6.0.0") (\(UIDevice.current.localizedModel.replacingOccurrences(of: "'", with: ""))) LinphoneSDK"
			self.mCore.setUserAgent(name: userAgent, version: self.coreVersion)
			self.mCore.videoCaptureEnabled = true
			self.mCore.videoDisplayEnabled = true
			self.mCore.videoPreviewEnabled = false
			self.mCore.fecEnabled = true
			
			// Migration
			self.mCore.config!.setBool(section: "sip", key: "auto_answer_replacing_calls", value: false)
			self.mCore.config!.setBool(section: "sip", key: "deliver_imdn", value: false)
			self.mCore.config!.setString(section: "misc", key: "log_collection_upload_server_url", value: "https://files.linphone.org:443/http-file-transfer-server/hft.php")
			self.mCore.config!.setString(section: "misc", key: "file_transfer_server_url", value: "https://files.linphone.org:443/http-file-transfer-server/hft.php")
			self.mCore.config!.setString(section: "misc", key: "version_check_url_root", value: "https://download.linphone.org/releases")
			
			self.mCore.imdnToEverybodyThreshold = 1
			self.imdnToEverybodyThreshold = self.mCore.imdnToEverybodyThreshold == 1
			//self.copyDatabaseFileToDocumentsDirectory()
			
			let shortcutsCount = self.mCore.config!.getInt(section: "ui", key: "shortcut_count", defaultValue: 0)
			if shortcutsCount > 0 {
				var shortcuts: [ShortcutModel] = []
				for i in 0...shortcutsCount {
					let shortcutSection = "shortcut_\(i)"
					let link = self.mCore.config!.getString(section: shortcutSection, key: "link", defaultString: "")
					let linkUrl = URL(string: link)
					let name = self.mCore.config!.getString(section: shortcutSection, key: "name", defaultString: "")
					let iconLink = self.mCore.config!.getString(section: shortcutSection, key: "icon", defaultString: "")
					let iconLinkUrl =  URL(string: iconLink)
					
					if linkUrl == nil {
						Log.error("Could not add shortcut #\(i) pointing to \(name) because the link URL '\(link)' is invalid")
						continue
					}
					if iconLinkUrl == nil {
						Log.error("Could not add shortcut #\(i) pointing to \(name) because the icon link URL '\(iconLink)' is invalid")
						continue
					}
					shortcuts.append(ShortcutModel(linkUrl: linkUrl!, name: name, iconLinkUrl: iconLinkUrl!))
				}
				
				DispatchQueue.main.async {
					self.shortcuts = shortcuts
				}
			}
			
			for acc in self.mCore.accountList {
				self.forceRemotePushToMatchVoipPushSettings(account: acc)
			}
			
			self.mCoreDelegate = CoreDelegateStub(onGlobalStateChanged: { (core: Core, state: GlobalState, _: String) in
				if state == GlobalState.On {
#if DEBUG
					let pushEnvironment = ".dev"
#else
					let pushEnvironment = ""
#endif
					for account in core.accountList {
						if account.params?.pushNotificationConfig?.provider != ("apns" + pushEnvironment) {
							let newParams = account.params?.clone()
							
							Log.info("Account \(String(describing: newParams?.identityAddress?.asStringUriOnly())) - updating apple push provider from \(String(describing: newParams?.pushNotificationConfig?.provider)) to apns\(pushEnvironment)")
							newParams?.pushNotificationConfig?.provider = "apns" + pushEnvironment
							
							account.params = newParams
						}
					}
					
					// TODO: Temporary workaround until SDK fixs
					if CorePreferences.defaultDomain == "" {
						CorePreferences.defaultDomain = core.defaultAccount?.params?.domain ?? ""
					}
					
					self.actionsToPerformOnCoreQueueWhenCoreIsStarted.forEach {	$0(core) }
					self.actionsToPerformOnCoreQueueWhenCoreIsStarted.removeAll()
					
					var accountModels: [AccountModel] = []
					for account in self.mCore.accountList {
						accountModels.append(AccountModel(account: account, core: self.mCore))
					}
					DispatchQueue.main.async {
						self.coreHasStartedOnce = true
						self.coreIsStarted = true
						self.accounts = accountModels
					}
				} else {
					DispatchQueue.main.async {
						self.coreIsStarted = state == GlobalState.On
					}
				}
				
			}, onCallStateChanged: { (core: Core, call: Call, cstate: Call.State, message: String) in
				TelecomManager.shared.onCallStateChanged(core: core, call: call, state: cstate, message: message)
				
				if core.calls.isEmpty {
					DispatchQueue.main.asyncAfter(deadline: .now() + 2) {
						if UIApplication.shared.applicationState == .background {
							Log.info("[CoreContext] core is currently in background with no calls, triggering onEnterBackground procedure")
							self.onEnterBackground()
						}
					}
				}
			}, onAuthenticationRequested: { (core: Core, authInfo: AuthInfo, method: AuthMethod) in
				if method == .Bearer {
					if let server = authInfo.authorizationServer, !server.isEmpty {
						Log.info("Authentication requested method is Bearer, starting Single Sign On activity with server URL \(server) and username \(authInfo.username ?? "")")
						self.bearerAuthInfoPendingPasswordUpdate = authInfo
						SingleSignOnManager.shared.setUp(ssoUrl: server, user: authInfo.username ?? "")
					}
				}
				
				if method == .HttpDigest {
					guard let username = authInfo.username, let domain = authInfo.domain, let realm = authInfo.realm else {
						Log.error("Authentication requested but either username [\(String(describing: authInfo.username))], domain [\(String(describing: authInfo.domain))] or server [\(String(describing: authInfo.authorizationServer))] is nil or empty!")
						return
					}
					
					guard let accountFound = core.accountList.first(where: {
						$0.params?.identityAddress?.username == authInfo.username &&
						$0.params?.identityAddress?.domain == authInfo.domain
					}) else {
						Log.info("[CoreContext] Failed to find account matching auth info, aborting auth dialog")
						return
					}

					let identity = "\(authInfo.username ?? "username")@\(authInfo.domain ?? "domain")"
					Log.info("[CoreContext] Authentication requested method is HttpDigest, showing dialog asking user for password for identity [\(identity)]")
					
					DispatchQueue.main.async {
						NotificationCenter.default.post(
							name: NSNotification.Name("PasswordUpdate"),
							object: nil,
							userInfo: ["address": "sip:" + identity]
						)
					}
					
					self.digestAuthInfoPendingPasswordUpdate = authInfo
				}
			}, onTransferStateChanged: { (_: Core, transferred: Call, callState: Call.State) in
				Log.info("[CoreContext] Transferred call \(transferred.remoteAddress!.asStringUriOnly()) state changed \(callState)")
				DispatchQueue.main.async {
					if callState == Call.State.Connected {
						ToastViewModel.shared.toastMessage = "Success_toast_call_transfer_successful"
						ToastViewModel.shared.displayToast = true
					} else if callState == Call.State.OutgoingProgress {
						ToastViewModel.shared.toastMessage = "Success_toast_call_transfer_in_progress"
						ToastViewModel.shared.displayToast = true
					} else if callState == Call.State.End || callState == Call.State.Error {
						ToastViewModel.shared.toastMessage = "Failed_toast_call_transfer_failed"
						ToastViewModel.shared.displayToast = true
					}
				}
			}, onConfiguringStatus: { (_: Core, status: ConfiguringState, message: String) in
				Log.info("New configuration state is \(status) = \(message)\n")
				let themeMainColor = CorePreferences.themeMainColor
				DispatchQueue.main.async {
					if status == ConfiguringState.Successful {
						var accountModels: [AccountModel] = []
						for account in self.mCore.accountList {
							accountModels.append(AccountModel(account: account, core: self.mCore))
						}
						self.accounts = accountModels
						ThemeManager.shared.applyTheme(named: themeMainColor)
					}
				}
			}, onLogCollectionUploadStateChanged: { (_: Core, _: Core.LogCollectionUploadState, info: String) in
				if info.starts(with: "https") {
					DispatchQueue.main.async {
						UIPasteboard.general.setValue(info, forPasteboardType: UTType.plainText.identifier)
						ToastViewModel.shared.toastMessage = "Success_send_logs"
						ToastViewModel.shared.displayToast = true
					}
				}
			}, onAccountRegistrationStateChanged: { (core: Core, account: Account, state: RegistrationState, message: String) in
				// If account has been configured correctly, we will go through Progress and Ok states
				// Otherwise, we will be Failed.
				Log.info("New registration state is \(state) for user id " +
						 "\( String(describing: account.params?.identityAddress?.asString())) = \(message)\n")
				
				switch state {
				case .Ok:
					DispatchQueue.main.async {
						NotificationCenter.default.post(name: NSNotification.Name("CoreStarted"), object: nil)
					}
					ContactsManager.shared.fetchContacts()
					
					if let defaultAccountParams = self.mCore.defaultAccount?.params, defaultAccountParams.publishEnabled == false {
						let params = defaultAccountParams
						let clonedParams = params.clone()
						clonedParams?.publishEnabled = true
						self.mCore.defaultAccount?.params = clonedParams
					}
					
					if self.mCore.consolidatedPresence !=  ConsolidatedPresence.Online {
						self.updatePresence(core: self.mCore, presence: ConsolidatedPresence.Online)
					}
				case .Cleared:
					Log.info("[CoreContext][onAccountRegistrationStateChanged] Account \(account.displayName()) registration was cleared.")
				case .Failed:
					Log.error("[CoreContext][onAccountRegistrationStateChanged] Account \(account.displayName()) registration failed.")
				default:
					break
				}
				
				TelecomManager.shared.onAccountRegistrationStateChanged(core: core, account: account, state: state, message: message)
				
				DispatchQueue.main.async {
					if state == .Ok {
						self.loggingInProgress = false
						self.loggedIn = true
					} else if state == .Progress || state == .Refreshing {
						self.loggingInProgress = true
					} else if state == .Cleared {
						self.loggingInProgress = false
						self.loggedIn = false
					} else {
						self.loggingInProgress = false
						self.loggedIn = false
						if self.networkStatusIsConnected {
							// If network is disconnected, a toast message with key "Unavailable_network" should already be displayed
							ToastViewModel.shared.toastMessage = "Registration_failed"
							ToastViewModel.shared.displayToast = true
						}
						
					}
				}
			}, onDefaultAccountChanged: { (_: Core, account: Account?) in
				if let account = account, self.mCore.globalState == GlobalState.On {
					Log.info("[CoreContext][onDefaultAccountChanged] Default account set to: \(account.displayName())")
					DispatchQueue.main.async {
						for accountModel in self.accounts {
							accountModel.isDefaultAccount = accountModel.account == account
						}
						
						NotificationCenter.default.post(name: NSNotification.Name("DefaultAccountChanged"), object: nil)
					}
					
					ContactsManager.shared.fetchContacts()
				}
			}, onAccountAdded: { (_: Core, acc: Account) in
				self.forceRemotePushToMatchVoipPushSettings(account: acc)
				
				var accountModels: [AccountModel] = []
				for account in self.mCore.accountList {
					accountModels.append(AccountModel(account: account, core: self.mCore))
				}
				DispatchQueue.main.async {
					self.accounts = accountModels
				}
			}, onAccountRemoved: { (_: Core, acc: Account) in
				var accountModels: [AccountModel] = []
				for account in self.mCore.accountList {
					accountModels.append(AccountModel(account: account, core: self.mCore))
				}
				DispatchQueue.main.async {
					self.accounts = accountModels
				}
			})
			
			self.mCore.addDelegate(delegate: self.mCoreDelegate)
			
			self.mCore.autoIterateEnabled = true
			
			try? self.mCore.start()
		}
	}
	
	func updatePresence(core: Core, presence: ConsolidatedPresence) {
		if core.config!.getBool(section: "app", key: "publish_presence", defaultValue: true) {
			core.consolidatedPresence = presence
		}
	}
	
	func onEnterForeground() {
		coreQueue.async {
			Log.info("[onEnterForegroundOrBackground] Entering foreground")
			
			try? self.mCore.start()
		}
	}

	func onEnterBackground() {
		coreQueue.async {
			Log.info("[onEnterForegroundOrBackground] Entering background, un-PUBLISHING presence info")
			
			self.updatePresence(core: self.mCore, presence: .Offline)
			self.mCore.iterate()
			
			if self.mCore.currentCall == nil && self.mCore.globalState == .On {
				Log.info("[onEnterForegroundOrBackground] Stopping core because no active calls")
				self.mCore.stop()
			} else {
				Log.info("[onEnterForegroundOrBackground] Skipped stop: core not fully On or active call in progress")
			}
		}
	}
	
	func crashForCrashlytics() {
		fatalError("Crashing app to test crashlytics")
	}
	
	func performActionOnCoreQueueWhenCoreIsStarted(action: @escaping (_ core: Core) -> Void ) {
		if coreIsStarted {
			doOnCoreQueue { core in
				action(core)
			}
		} else {
			actionsToPerformOnCoreQueueWhenCoreIsStarted.append(action)
		}
	}
	
	func addCoreDelegateStub(delegate: CoreDelegateStub) {
		mCore.addDelegate(delegate: delegate)
	}
	func removeCoreDelegateStub(delegate: CoreDelegateStub) {
		mCore.removeDelegate(delegate: delegate)
	}
	
	func forceRemotePushToMatchVoipPushSettings(account: Account) {
		if let params = account.params, params.pushNotificationAllowed && !params.remotePushNotificationAllowed {
			Log.warn("account \(account.displayName()): VOIP and REMOTE push setting mismatch, force \(params.pushNotificationAllowed ? "enabling" : "disabling") of REMOTE Push")
			let newParams = params.clone()
			newParams?.remotePushNotificationAllowed = params.pushNotificationAllowed
			account.params = newParams
		}
	}
	
	func copyDatabaseFileToDocumentsDirectory() {
		if let rcDir = FileManager.default.containerURL(forSecurityApplicationGroupIdentifier: Config.appGroupName)?
			.appendingPathComponent("Library/Application Support/linphone") {
			let rcFileUrl = rcDir.appendingPathComponent("linphone.db")
			let directory = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first
			if directory != nil {
				do {
					if FileManager.default.fileExists(atPath: directory!.appendingPathComponent("linphone.db").path) {
						try FileManager.default.removeItem(at: directory!.appendingPathComponent("linphone.db"))
					}
					try FileManager.default.copyItem(at: rcFileUrl, to: directory!.appendingPathComponent("linphone.db"))
				} catch {
					print("Error: ", error)
				}
			}
		}
	}
}

// swiftlint:enable line_length
// swiftlint:enable cyclomatic_complexity
// swiftlint:enable identifier_name
