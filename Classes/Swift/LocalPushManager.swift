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
import NetworkExtension
import linphonesw
import Combine


let localPushProviderBundleIdentifier = "org.linphone.phone.localpushprovider"

@available(iOS 15.0, *)
@objc class LocalPushManager : NSObject, NEAppPushDelegate {
	
	@objc static let shared = LocalPushManager()
	private var appPushManager: NEAppPushManager?
	private let isInitialized = MutableLiveData(false)
	let isActive = MutableLiveData(false)
	private let dispatchQueue = DispatchQueue(label: "DirectoryViewModel.dispatchQueue")
	private let pushManagerIsActiveSubject = CurrentValueSubject<Bool, Never>(false)
	private var pushManagerIsActiveCancellable: AnyCancellable?
	private var cancellables = Set<AnyCancellable>()
	private(set) lazy var pushManagerIsActivePublisher = {
		pushManagerIsActiveSubject
			.debounce(for: .milliseconds(500), scheduler: dispatchQueue)
		.eraseToAnyPublisher()
	}()

	override init() {
		super.init()
		NEAppPushManager.loadAllFromPreferences { managers, error in
			if let error = error {
				Log.w("[LocalPushManager] Failed to load all NEAppPushManager's from preferences: \(error)")
			}
			self.appPushManager = managers?.first ?? NEAppPushManager()
			let appPushManager = self.appPushManager!
			self.pushManagerIsActiveCancellable = NSObject.KeyValueObservingPublisher(object: appPushManager, keyPath: \.isActive, options: [.initial, .new])
				.subscribe(self.pushManagerIsActiveSubject)
			self.pushManagerIsActivePublisher
				.receive(on: DispatchQueue.main)
				.sink { [weak self] isAppPushManagerActive in
					self?.isActive.value = isAppPushManagerActive
				}
				.store(in: &self.cancellables)
			
			appPushManager.delegate = self
			
			self.isInitialized.value = true
			Log.i("[LocalPushManager] NEAppPushManager initialisation : enabled=\(String(describing: appPushManager.isEnabled)) ssids=\(String(describing: appPushManager.matchSSIDs))")
		}
	}
	
	@objc func extensionIsActive() -> Bool {
		return appPushManager?.isActive == true
	}
	
	private func applyConfig(coreConfig:Config) {
		let appPushManager = self.appPushManager!
		let ssids = coreConfig.getStringList(section: "local_push", key: "ssids", defaultList: []) // csv
		let enabled = !ssids.isEmpty
		appPushManager.isEnabled = enabled
		appPushManager.matchSSIDs = ssids
		appPushManager.providerConfiguration = [
			"coreconfig": coreConfig.dump()
		]
		appPushManager.localizedDescription = NSLocalizedString("Local Push Manager", comment: "")
		appPushManager.providerBundleIdentifier = localPushProviderBundleIdentifier
		
		if (appPushManager.isEnabled) {
			self.pushManagerIsActiveCancellable = NSObject.KeyValueObservingPublisher(object: appPushManager, keyPath: \.isActive, options: [.initial, .new])
				.subscribe(self.pushManagerIsActiveSubject)
			appPushManager.saveToPreferences { error in
				if (error != nil) {
					Log.e("[LocalPushManager] error saving Local Push preferences \(String(describing: error)) enabled=\(String(describing: appPushManager.isEnabled)) ssids=\(String(describing: appPushManager.matchSSIDs))")
				} else {
					Log.i("[LocalPushManager] NEAppPushManager saved : enabled=\(String(describing: appPushManager.isEnabled)) ssids=\(String(describing: appPushManager.matchSSIDs))")
				}
				appPushManager.loadFromPreferences { error in
					if (error != nil) {
						Log.e("[LocalPushManager] error post save reloading Local Push preferences \(String(describing: error)) enabled=\(String(describing: appPushManager.isEnabled)) ssids=\(String(describing: appPushManager.matchSSIDs))")
					} else {
						Log.i("[LocalPushManager] NEAppPushManager post save reloaded : enabled=\(String(describing: appPushManager.isEnabled)) ssids=\(String(describing: appPushManager.matchSSIDs))")
					}

				}
			}
		} else {
			pushManagerIsActiveSubject.send(false)
			Log.i("[LocalPushManager] NEAppPushManager disabled.")
		}
	}
	
	@objc func configureLocalPush(cCoreConfig:OpaquePointer) {
		if (self.isInitialized.value != true ) {
			self.isInitialized.observeOnce { _ in
				self.applyConfig(coreConfig: Config.getSwiftObject(cObject: cCoreConfig))
			}
		} else {
			applyConfig(coreConfig: Config.getSwiftObject(cObject: cCoreConfig))
		}
		
	}
	func appPushManager(_ manager: NEAppPushManager, didReceiveIncomingCallWithUserInfo userInfo: [AnyHashable : Any] = [:]) {
		// Call handling
	}
	
	@objc func addActiveCallBackObserver (action:@escaping(Bool) -> Void) {
		isActive.readCurrentAndObserve { active in
			action(active!)
		}
	}
	
}
