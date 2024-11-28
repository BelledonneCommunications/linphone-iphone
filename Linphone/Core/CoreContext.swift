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
import linphone // needed for unwrapped function linphone_core_set_push_and_app_delegate_dispatch_queue
import Combine
import UniformTypeIdentifiers
import Network

#if USE_CRASHLYTICS
import Firebase
#endif

final class CoreContext: ObservableObject {
	
	static let shared = CoreContext()
	private var sharedMainViewModel = SharedMainViewModel.shared
	
	var coreVersion: String = Core.getVersion
	@Published var loggedIn: Bool = false
	@Published var loggingInProgress: Bool = false
	@Published var coreIsStarted: Bool = false
	@Published var accounts: [AccountModel] = []
	@Published var enteredForeground = false
	@Published var shortcuts: [ShortcutModel] = []
	private var mCore: Core!
	private var mIterateSuscription: AnyCancellable?
	
	var bearerAuthInfoPendingPasswordUpdate: AuthInfo?
	
	let monitor = NWPathMonitor()
	var networkStatusIsConnected: Bool = true // updated on core queue
	
	private var mCoreDelegate: CoreDelegate!
	private var actionsToPerformOnCoreQueueWhenCoreIsStarted: [((Core) -> Void)] = []
	private var callStateCallBacks: [((Call.State) -> Void)] = []
	private var configuringStateCallBacks: [((ConfiguringState) -> Void)] = []

	private init() {
		do {
			try initialiseCore()
		} catch {
			
		}
	}
	
	func doOnCoreQueue(synchronous: Bool = false, lambda: @escaping (Core) -> Void) {
		if synchronous {
			coreQueue.sync {
				lambda(self.mCore)
			}
		} else {
			coreQueue.async {
				if self.mCore.globalState != .Off {
					lambda(self.mCore)
				} else {
					Log.warn("Doesn't run the asynchronous function because the core is off")
				}
			}
		}
	}
	
	func initialiseCore() throws {
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
			
			Log.info("Initialising core")
			self.mCore = try? Factory.Instance.createSharedCoreWithConfig(config: Config.get(), systemContext: nil, appGroupId: Config.appGroupName, mainCore: true)
			
			linphone_core_set_push_and_app_delegate_dispatch_queue(self.mCore.getCobject, Unmanaged.passUnretained(coreQueue).toOpaque())
			self.mCore.autoIterateEnabled = false
			self.mCore.callkitEnabled = true
			self.mCore.pushNotificationEnabled = true
			
			let appName = Bundle.main.infoDictionary?["CFBundleName"] as? String
			let version = Bundle.main.infoDictionary?["CFBundleShortVersionString"] as? String
			
			let userAgent = "\(appName ?? "Linphone")iOS/\(version ?? "6.0.0") Beta (\(UIDevice.current.localizedModel.replacingOccurrences(of: "'", with: ""))) LinphoneSDK"
			self.mCore.setUserAgent(name: userAgent, version: self.coreVersion)
			self.mCore.videoCaptureEnabled = true
			self.mCore.videoDisplayEnabled = true
			self.mCore.videoPreviewEnabled = false
			self.mCore.fecEnabled = true
			self.mCore.friendListSubscriptionEnabled = true
			self.mCore.maxSizeForAutoDownloadIncomingFiles = 0
			self.mCore.config!.setBool(section: "sip", key: "auto_answer_replacing_calls", value: false)
			self.mCore.config!.setBool(section: "sip", key: "deliver_imdn", value: false)
			
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
			
			self.mCoreDelegate = CoreDelegateStub(onGlobalStateChanged: { (core: Core, state: GlobalState, _: String) in
				if state == GlobalState.On {
#if DEBUG
					let pushEnvironment = ".dev"
#else
					let pushEnvironment = ""
#endif
					for account in core.accountList where account.params?.pushNotificationConfig?.provider != ("apns" + pushEnvironment) {
						let newParams = account.params?.clone()
						Log.info("Account \(String(describing: newParams?.identityAddress?.asStringUriOnly())) - updating apple push provider from \(String(describing: newParams?.pushNotificationConfig?.provider)) to apns\(pushEnvironment)")
						newParams?.pushNotificationConfig?.provider = "apns" + pushEnvironment
						account.params = newParams
					}
					
					self.actionsToPerformOnCoreQueueWhenCoreIsStarted.forEach {	$0(core) }
					self.actionsToPerformOnCoreQueueWhenCoreIsStarted.removeAll()
					
					var accountModels: [AccountModel] = []
					for account in self.mCore.accountList {
						accountModels.append(AccountModel(account: account, core: self.mCore))
					}
					DispatchQueue.main.async {
						self.coreIsStarted = true
						self.accounts = accountModels
					}
				}
			}, onCallStateChanged: { (core: Core, call: Call, cstate: Call.State, message: String) in
				TelecomManager.shared.onCallStateChanged(core: core, call: call, state: cstate, message: message)
			}, onAuthenticationRequested: { (_: Core, authInfo: AuthInfo, method: AuthMethod) in
				guard let username = authInfo.username, let server = authInfo.authorizationServer, !server.isEmpty else {
					Log.error("Authentication requested but either username [\(String(describing: authInfo.username))], domain [\(String(describing: authInfo.domain))] or server [\(String(describing: authInfo.authorizationServer))] is nil or empty!")
					return
				}
				if method == .Bearer {
					Log.info("Authentication requested method is Bearer, starting Single Sign On activity with server URL \(server) and username \(username)")
					self.bearerAuthInfoPendingPasswordUpdate = authInfo
					SingleSignOnManager.shared.setUp(ssoUrl: server, user: username)
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
				var accountModels: [AccountModel] = []
				for account in self.mCore.accountList {
					accountModels.append(AccountModel(account: account, core: self.mCore))
				}
				DispatchQueue.main.async {
					if status == ConfiguringState.Successful {
						ToastViewModel.shared.toastMessage = "Successful"
						ToastViewModel.shared.displayToast = true
						self.accounts = accountModels
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
					ContactsManager.shared.fetchContacts()
					if self.mCore.consolidatedPresence !=  ConsolidatedPresence.Online {
						self.updatePresence(core: self.mCore, presence: ConsolidatedPresence.Online)
					}
				case .Cleared:
					Log.info("[onAccountRegistrationStateChanged] Account \(account.displayName()) registration was cleared. Looking for auth info")
					if let authInfo = account.findAuthInfo() {
						Log.info("[onAccountRegistrationStateChanged] Found auth info for account, removing it")
						core.removeAuthInfo(info: authInfo)
					} else {
						Log.warn("[onAccountRegistrationStateChanged] Failed to find matching auth info for account")
					}
				case .Failed:  // If registration failed, remove account from core
					if self.networkStatusIsConnected {
						let params = account.params
						let clonedParams = params?.clone()
						clonedParams?.registerEnabled = false
						account.params = clonedParams
						
						Log.warn("Registration failed for account \(account.displayName()), deleting it from core")
						core.removeAccount(account: account)
					}
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
						ToastViewModel.shared.toastMessage = "Success_account_logged_out"
						ToastViewModel.shared.displayToast = true
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
			}, onAccountAdded: { (_: Core, _: Account) in
				var accountModels: [AccountModel] = []
				for account in self.mCore.accountList {
					accountModels.append(AccountModel(account: account, core: self.mCore))
				}
				DispatchQueue.main.async {
					self.accounts = accountModels
				}
			}, onAccountRemoved: { (_: Core, _: Account) in
				var accountModels: [AccountModel] = []
				for account in self.mCore.accountList {
					accountModels.append(AccountModel(account: account, core: self.mCore))
				}
				DispatchQueue.main.async {
					self.accounts = accountModels
				}
			})
			self.mCore.addDelegate(delegate: self.mCoreDelegate)
			
			self.mIterateSuscription = Timer.publish(every: 0.02, on: .main, in: .common)
				.autoconnect()
				.receive(on: coreQueue)
				.sink { _ in
					self.mCore.iterate()
				}
			try? self.mCore.start()
		}
	}
	
	func updatePresence(core: Core, presence: ConsolidatedPresence) {
		if core.config!.getBool(section: "app", key: "publish_presence", defaultValue: true) {
			core.consolidatedPresence = presence
		}
	}
	
	func onEnterForeground() {
		coreQueue.sync {
			// We can't rely on defaultAccount?.params?.isPublishEnabled
			// as it will be modified by the SDK when changing the presence status
		
			try? self.mCore.start()
			Log.info("App is in foreground, PUBLISHING presence as Online")
			self.updatePresence(core: self.mCore, presence: ConsolidatedPresence.Online)
		}
	}
	
	func onEnterBackground() {
		coreQueue.sync {
			// We can't rely on defaultAccount?.params?.isPublishEnabled
			// as it will be modified by the SDK when changing the presence status
			Log.info("App is in background, un-PUBLISHING presence info")
			
			// We don't use ConsolidatedPresence.Busy but Offline to do an unsubscribe,
			// Flexisip will handle the Busy status depending on other devices
			self.updatePresence(core: self.mCore, presence: ConsolidatedPresence.Offline)
			self.mCore.iterate()
			
			if self.mCore.currentCall == nil {
				self.mCore.stop()
			}
		}
	}
	
	func crashForCrashlytics() {
		fatalError("Crashing app to test crashlytics")
	}
	
	func performActionOnCoreQueueWhenCoreIsStarted(action: @escaping (_ core: Core) -> Void ) {
		if coreIsStarted {
			CoreContext.shared.doOnCoreQueue { core in
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
	
}

// swiftlint:enable line_length
// swiftlint:enable cyclomatic_complexity
// swiftlint:enable identifier_name
