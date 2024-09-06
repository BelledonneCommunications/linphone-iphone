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

// swiftlint:disable large_tuple
// swiftlint:disable line_length
// swiftlint:disable cyclomatic_complexity
// swiftlint:disable identifier_name
// swiftlint:disable type_body_length

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
	@Published var hasDefaultAccount: Bool = false
	@Published var coreIsStarted: Bool = false
	@Published var accounts: [AccountModel] = []
	
	private var mCore: Core!
	private var mIterateSuscription: AnyCancellable?
	private var mCoreSuscriptions = Set<AnyCancellable?>()
	
	var bearerAuthInfoPendingPasswordUpdate: AuthInfo?
	
	let monitor = NWPathMonitor()
	var networkStatusIsConnected: Bool = true // updated on core queue
	
	private var mCorePushIncomingDelegate: CoreDelegate!
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
				if isConnected {
					Log.info("Network is now satisfied")
				} else {
					Log.error("Network is now \(path.status)")
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
			
			self.mCore.setUserAgent(name: "\(appName ?? "Linphone")iOS/\(version ?? "6.0.0") Beta (\(UIDevice.current.localizedModel)) LinphoneSDK", version: self.coreVersion)
			
			self.mCore.videoCaptureEnabled = true
			self.mCore.videoDisplayEnabled = true
			self.mCore.videoPreviewEnabled = false
			self.mCore.fecEnabled = true
			self.mCore.friendListSubscriptionEnabled = true
			self.mCore.maxSizeForAutoDownloadIncomingFiles = 0
			self.mCore.config!.setBool(section: "sip", key: "auto_answer_replacing_calls", value: false)
			self.mCore.config!.setBool(section: "sip", key: "deliver_imdn", value: false)
			self.mCoreSuscriptions.insert(self.mCore.publisher?.onGlobalStateChanged?.postOnCoreQueue { (cbVal: (core: Core, state: GlobalState, message: String)) in
				if cbVal.state == GlobalState.On {
#if DEBUG
					let pushEnvironment = ".dev"
#else
					let pushEnvironment = ""
#endif
					for account in cbVal.core.accountList where account.params?.pushNotificationConfig?.provider != ("apns" + pushEnvironment) {
						let newParams = account.params?.clone()
						Log.info("Account \(String(describing: newParams?.identityAddress?.asStringUriOnly())) - updating apple push provider from \(String(describing: newParams?.pushNotificationConfig?.provider)) to apns\(pushEnvironment)")
						newParams?.pushNotificationConfig?.provider = "apns" + pushEnvironment
						account.params = newParams
					}
					
					self.actionsToPerformOnCoreQueueWhenCoreIsStarted.forEach {	$0(cbVal.core) }
					self.actionsToPerformOnCoreQueueWhenCoreIsStarted.removeAll()
					
					let hasDefaultAccount = self.mCore.defaultAccount != nil ? true : false
					Log.info("debugtrace onGlobalStateChanged -- core accounts: \(self.mCore.accountList.count), hasDefaultAccount: \(self.mCore.defaultAccount != nil ? "yes" : "no")")
					var accountModels: [AccountModel] = []
					for account in self.mCore.accountList {
						accountModels.append(AccountModel(account: account, corePublisher: self.mCore.publisher))
					}
					DispatchQueue.main.async {
						self.hasDefaultAccount = hasDefaultAccount
						self.coreIsStarted = true
						self.accounts = accountModels
					}
				}
			})
			
			// Create a Core listener to listen for the callback we need
			// In this case, we want to know about the account registration status
			self.mCoreSuscriptions.insert(self.mCore.publisher?.onConfiguringStatus?.postOnCoreQueue { (cbVal: (core: Core, status: ConfiguringState, message: String)) in
				Log.info("New configuration state is \(cbVal.status) = \(cbVal.message)\n")
				Log.info("debugtrace onConfiguringStatus -- core accounts: \(self.mCore.accountList.count), hasDefaultAccount: \(self.mCore.defaultAccount != nil ? "yes" : "no")")
				var accountModels: [AccountModel] = []
				for account in self.mCore.accountList {
					accountModels.append(AccountModel(account: account, corePublisher: self.mCore.publisher))
				}
				DispatchQueue.main.async {
					if cbVal.status == ConfiguringState.Successful {
						ToastViewModel.shared.toastMessage = "Successful"
						ToastViewModel.shared.displayToast = true
						self.accounts = accountModels
					}
				}
				/*
				 else {
				 ToastViewModel.shared.toastMessage = "Failed"
				 ToastViewModel.shared.displayToast = true
				 }
				 */
			})
			
			self.mCoreSuscriptions.insert(self.mCore.publisher?.onAccountRegistrationStateChanged?.postOnCoreQueue { (cbVal: (core: Core, account: Account, state: RegistrationState, message: String)) in
				
				// If account has been configured correctly, we will go through Progress and Ok states
				// Otherwise, we will be Failed.
				Log.info("New registration state is \(cbVal.state) for user id " +
						 "\( String(describing: cbVal.account.params?.identityAddress?.asString())) = \(cbVal.message)\n")
				Log.info("debugtrace onAccountRegistrationStateChanged -- core accounts: \(self.mCore.accountList.count), hasDefaultAccount: \(self.mCore.defaultAccount != nil ? "yes" : "no")")
				
				switch(cbVal.state) {
				case .Ok:
					let newParams = cbVal.account.params?.clone()
					newParams?.internationalPrefix = "33"
					newParams?.internationalPrefixIsoCountryCode = "FRA"
					newParams?.useInternationalPrefixForCallsAndChats = true
					cbVal.account.params = newParams
					
					ContactsManager.shared.fetchContacts()
					if self.mCore.consolidatedPresence !=  ConsolidatedPresence.Online {
						self.updatePresence(core: self.mCore, presence: ConsolidatedPresence.Online)
					}
				case .Cleared:
					Log.info("[onAccountRegistrationStateChanged] Account \(cbVal.account.displayName()) registration was cleared. Looking for auth info")
					if let authInfo = cbVal.account.findAuthInfo() {
						Log.info("[onAccountRegistrationStateChanged] Found auth info for account, removing it")
						cbVal.core.removeAuthInfo(info: authInfo)
					} else {
						Log.warn("[onAccountRegistrationStateChanged] Failed to find matching auth info for account")
					}
				case .Failed:  // If registration failed, remove account from core
					if self.networkStatusIsConnected {
						let params = cbVal.account.params
						let clonedParams = params?.clone()
						clonedParams?.registerEnabled = false
						cbVal.account.params = clonedParams
						
						Log.warn("Registration failed for account \(cbVal.account.displayName()), deleting it from core")
						cbVal.core.removeAccount(account: cbVal.account)
					}
				default:
					break
				}
				
				TelecomManager.shared.onAccountRegistrationStateChanged(core: cbVal.core, account: cbVal.account, state: cbVal.state, message: cbVal.message)
				
				DispatchQueue.main.async {
					if cbVal.state == .Ok {
						self.loggingInProgress = false
						self.loggedIn = true
					} else if cbVal.state == .Progress || cbVal.state == .Refreshing {
						self.loggingInProgress = true
					} else if cbVal.state == .Cleared {
						self.loggingInProgress = false
						self.loggedIn = false
						self.hasDefaultAccount = false
						ToastViewModel.shared.toastMessage = "Success_account_logged_out"
						ToastViewModel.shared.displayToast = true
					} else {
						self.loggingInProgress = false
						self.loggedIn = false
						ToastViewModel.shared.toastMessage = "Registration_failed"
						ToastViewModel.shared.displayToast = true
					}
				}
			})
			
			self.mCoreSuscriptions.insert(self.mCore.publisher?.onCallStateChanged?.postOnCoreQueue { (cbVal: (core: Core, call: Call, state: Call.State, message: String)) in
				TelecomManager.shared.onCallStateChanged(core: cbVal.core, call: cbVal.call, state: cbVal.state, message: cbVal.message)
			})
			
			self.mCorePushIncomingDelegate = CoreDelegateStub(onCallStateChanged: { (_, call: Call, cstate: Call.State, _) in
				if cstate == .PushIncomingReceived {
					let callLog = call.callLog
					let callId = callLog?.callId ?? ""
					Log.info("PushIncomingReceived in core delegate, display callkit call")
					TelecomManager.shared.displayIncomingCall(call: call, handle: "Calling", hasVideo: false, callId: callId, displayName: "Calling")
				}
			})
			self.mCore.addDelegate(delegate: self.mCorePushIncomingDelegate)
			
			self.mCoreSuscriptions.insert(self.mCore.publisher?.onLogCollectionUploadStateChanged?.postOnCoreQueue { (cbValue: (_: Core, _: Core.LogCollectionUploadState, info: String)) in
				
				if cbValue.info.starts(with: "https") {
					DispatchQueue.main.async {
						UIPasteboard.general.setValue(
							cbValue.info,
							forPasteboardType: UTType.plainText.identifier
						)
						ToastViewModel.shared.toastMessage = "Success_send_logs"
						ToastViewModel.shared.displayToast = true
					}
				}
			})
			
			self.mCoreSuscriptions.insert(self.mCore.publisher?.onTransferStateChanged?.postOnCoreQueue { (cbValue: (_: Core, transferred: Call, callState: Call.State)) in
				Log.info(
					"[CoreContext] Transferred call \(cbValue.transferred.remoteAddress!.asStringUriOnly()) state changed \(cbValue.callState)"
				)
				
				DispatchQueue.main.async {
					if cbValue.callState == Call.State.Connected {
						ToastViewModel.shared.toastMessage = "Success_toast_call_transfer_successful"
						ToastViewModel.shared.displayToast = true
					} else if cbValue.callState == Call.State.OutgoingProgress {
						ToastViewModel.shared.toastMessage = "Success_toast_call_transfer_in_progress"
						ToastViewModel.shared.displayToast = true
					} else if cbValue.callState == Call.State.End || cbValue.callState == Call.State.Error {
						ToastViewModel.shared.toastMessage = "Failed_toast_call_transfer_failed"
						ToastViewModel.shared.displayToast = true
					}
				}
			})
			
			self.mCoreSuscriptions.insert(self.mCore.publisher?.onAuthenticationRequested?.postOnCoreQueue { (cbValue: (_: Core, authInfo: AuthInfo, method: AuthMethod)) in
				let authInfo = cbValue.authInfo
				guard let username = authInfo.username, let server = authInfo.authorizationServer, !server.isEmpty else {
					Log.error("Authentication requested but either username [\(String(describing: authInfo.username))], domain [\(String(describing: authInfo.domain))] or server [\(String(describing: authInfo.authorizationServer))] is nil or empty!")
					return
				}
				if cbValue.method == .Bearer {
					Log.info("Authentication requested method is Bearer, starting Single Sign On activity with server URL \(server) and username \(username)")
					self.bearerAuthInfoPendingPasswordUpdate = cbValue.authInfo
					SingleSignOnManager.shared.setUp(ssoUrl: server, user: username)
				}
			})
			
			self.mCoreSuscriptions.insert(self.mCore.publisher?.onAccountAdded?
				.postOnCoreQueue { _ in
					var accountModels: [AccountModel] = []
					for account in self.mCore.accountList {
						accountModels.append(AccountModel(account: account, corePublisher: self.mCore.publisher))
					}
					DispatchQueue.main.async {
						self.accounts = accountModels
					}
				})
			
			self.mCoreSuscriptions.insert(self.mCore.publisher?.onAccountRemoved?
				.postOnCoreQueue { _ in
					var accountModels: [AccountModel] = []
					for account in self.mCore.accountList {
						accountModels.append(AccountModel(account: account, corePublisher: self.mCore.publisher))
					}
					DispatchQueue.main.async {
						self.accounts = accountModels
					}
				})
			
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
	
	func getCorePublisher() -> CoreDelegatePublisher? {
		return mCore.publisher
	}
	
}

// swiftlint:enable large_tuple
// swiftlint:enable line_length
// swiftlint:enable cyclomatic_complexity
// swiftlint:enable identifier_name
// swiftlint:enable type_body_length
