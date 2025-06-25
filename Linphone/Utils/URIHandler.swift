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
import Combine

class URIHandler {
	
	// Need to cover all Info.plist URL schemes.
	private static let callSchemes = ["sip", "sip-linphone", "linphone-sip", "tel", "callto"]
	private static let secureCallSchemes = ["sips", "sips-linphone", "linphone-sips"]
	private static let configurationSchemes = ["linphone-config"]
	private static let sharedExtensionSchemes = ["linphone-message"]

	private static var uriHandlerCoreDelegate: CoreDelegateStub?
	
	static func addCoreDelegate() {
		uriHandlerCoreDelegate = CoreDelegateStub(
			onCallStateChanged: { (_: Core, _: Call, state: Call.State, _: String) in
				if state == .Error {
					toast("Failed_uri_handler_call_failed")
					CoreContext.shared.removeCoreDelegateStub(delegate: uriHandlerCoreDelegate!)
				}
				if state == .End {
					CoreContext.shared.removeCoreDelegateStub(delegate: uriHandlerCoreDelegate!)
				}
			},
			onConfiguringStatus: { (_: Core, state: ConfiguringState, _: String) in
				if state == .Failed {
					toast("Failed_uri_handler_config_failed")
					CoreContext.shared.removeCoreDelegateStub(delegate: uriHandlerCoreDelegate!)
				}
				if state == .Successful {
					toast("uri_handler_config_success")
					CoreContext.shared.removeCoreDelegateStub(delegate: uriHandlerCoreDelegate!)
				}
			})
		CoreContext.shared.addCoreDelegateStub(delegate: uriHandlerCoreDelegate!)
	}
	
	static func handleURL(url: URL) {
		Log.info("[URIHandler] handleURL: \(url)")
		if let scheme = url.scheme {
			if secureCallSchemes.contains(scheme) {
				initiateCall(url: url, withScheme: "sips")
			} else if callSchemes.contains(scheme) {
				initiateCall(url: url, withScheme: "sip")
			} else if configurationSchemes.contains(scheme) {
				initiateConfiguration(url: url)
			} else if sharedExtensionSchemes.contains(scheme) {
				processReceivedFiles(url: url)
			} else if scheme == SingleSignOnManager.shared.ssoRedirectUri.scheme {
				continueSSO(url: url)
			} else {
				Log.error("[URIHandler] unhandled URL \(url) (check Info.plist)")
			}
		} else {
			Log.error("[URIHandler] invalid scheme for URL \(url)")
		}
	}
	
	private static func initiateCall(url: URL, withScheme newScheme: String) {
		CoreContext.shared.performActionOnCoreQueueWhenCoreIsStarted { core in
			if let newSchemeUrl = url.withNewScheme(newScheme),
			   let address = core.interpretUrl(url: newSchemeUrl.absoluteString,
											   applyInternationalPrefix: LinphoneUtils.applyInternationalPrefix(core: core)) {
				Log.info("[URIHandler] initiating call to address : \(address.asString())")
				addCoreDelegate()
				TelecomManager.shared.doCallWithCore(addr: address, isVideo: false, isConference: false)
			} else {
				Log.error("[URIHandler] unable to call with \(url.resourceSpecifier)")
				toast("Failed_uri_handler_bad_call_address")
			}
		}
	}
	
	private static func initiateConfiguration(url: URL) {
		if autoRemoteProvisioningOnConfigUriHandler() {
			CoreContext.shared.doOnCoreQueue { core in
				Log.info("[URIHandler] provisioning app with URI: \(url.resourceSpecifier)")
				do {
					addCoreDelegate()
					var urlString = url.resourceSpecifier
					if urlString.starts(with: "//") {
						urlString = String(urlString.dropFirst(2))
					}
					
					core.config?.setString(section: "misc", key: "config-uri", value: urlString)
					try core.setProvisioninguri(newValue: urlString)
					core.stop()
					try core.start()
				} catch {
					Log.error("[URIHandler] unable to configure the app with \(url.resourceSpecifier) \(error)")
					toast("Failed_uri_handler_bad_config_address")
				}
			}
		} else {
			Log.warn("[URIHandler] received configuration request, but automatic provisioning is disabled.")
		}
	}
	
	private static func processReceivedFiles(url: URL) {
		Log.info("[URIHandler] processing received files from URL: \(url.path)")
		
		var urlString = url.path
		if urlString.starts(with: "//") {
			urlString = String(urlString.dropFirst(2))
		}
		
		for urlFile in urlString.components(separatedBy: ",") {
			SharedMainViewModel.shared.fileUrlsToShare.append(urlFile)
		}
		
		SharedMainViewModel.shared.changeIndexView(indexViewInt: 2)
	}
	
	private static func continueSSO(url: URL) {
		if let authorizationFlow = SingleSignOnManager.shared.currentAuthorizationFlow,
		   authorizationFlow.resumeExternalUserAgentFlow(with: url) {
			SingleSignOnManager.shared.currentAuthorizationFlow = nil
		}
	}
	
	private static func autoRemoteProvisioningOnConfigUriHandler() -> Bool {
		return Config.get().getBool(section: "app", key: "auto_apply_provisioning_config_uri_handler", defaultValue: true)
	}
	
	private static func toast(_ message: String) {
		DispatchQueue.main.async {
			ToastViewModel.shared.toastMessage = message
			ToastViewModel.shared.displayToast = true
		}
	}
}
