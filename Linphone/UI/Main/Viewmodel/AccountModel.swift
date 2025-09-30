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
	static let TAG = "[AccountModel]"
	
	let account: Account
	@Published var registrationState: RegistrationState = .None
	@Published var humanReadableRegistrationState: String = ""
	@Published var summary: String = ""
	@Published var registrationStateAssociatedUIColor: Color = .clear
	@Published var isRegistrered: Bool = false
	@Published var notificationsCount: Int = 0
	@Published var isDefaultAccount: Bool = false
	@Published var displayName: String = ""
	@Published var address: String = ""
	@Published var avatarModel: ContactAvatarModel?
	@Published var photoAvatarModel: String?
	@Published var displayNameAvatar: String = ""
	@Published var usernaneAvatar: String = ""
	@Published var imagePathAvatar: URL?
	
	@Published var devices: [AccountDeviceModel] = []
	
	private var accountDelegate: AccountDelegate?
	private var coreDelegate: CoreDelegate?
	
	private var accountManagerServices: AccountManagerServices?
	private var requestDelegate: AccountManagerServicesRequestDelegate?
	
	init(account: Account, core: Core) {
		self.account = account
        
        self.computeNotificationsCount()
		
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
		
		let displayNameTmp = account.params?.identityAddress?.displayName ?? displayName
		let usernaneAvatarTmp = account.contactAddress?.username ?? displayName
		var photoAvatarModelTmp = ""
		
		let preferences = UserDefaults.standard
		
		let photoAvatarModelKey = usernaneAvatarTmp
		
		if !photoAvatarModelKey.isEmpty {
			if preferences.object(forKey: photoAvatarModelKey) == nil {
				self.saveImage(
					image: ContactsManager.shared.textToImage(
						firstName: usernaneAvatarTmp, lastName: ""),
					name: usernaneAvatarTmp,
					prefix: "-default")
			} else {
				photoAvatarModelTmp = preferences.string(forKey: photoAvatarModelKey)!
			}
			
			DispatchQueue.main.async { [self] in
				switch state {
				case .Cleared, .None:
					humanReadableRegistrationState = "drawer_menu_account_connection_status_cleared".localized()
					summary = "manage_account_status_cleared_summary".localized()
					registrationStateAssociatedUIColor = .orangeWarning600
				case .Progress:
					humanReadableRegistrationState = "drawer_menu_account_connection_status_progress".localized()
					summary = "manage_account_status_progress_summary".localized()
					registrationStateAssociatedUIColor = .greenSuccess500
				case .Failed:
					humanReadableRegistrationState = "drawer_menu_account_connection_status_failed".localized()
					summary = "manage_account_status_failed_summary".localized()
					registrationStateAssociatedUIColor = .redDanger500
				case .Ok:
					humanReadableRegistrationState = "drawer_menu_account_connection_status_connected".localized()
					summary = "manage_account_status_connected_summary".localized()
					registrationStateAssociatedUIColor = .greenSuccess500
				case .Refreshing:
					humanReadableRegistrationState = "drawer_menu_account_connection_status_refreshing".localized()
					summary = "manage_account_status_progress_summary".localized()
					registrationStateAssociatedUIColor = .grayMain2c500
				}
				
				registrationState = state
				
				isRegistrered = state == .Ok
				isDefaultAccount = isDefault
				self.displayName = displayName
				address.map {self.address = $0}
				
				photoAvatarModel = photoAvatarModelTmp
				displayNameAvatar = displayNameTmp
				usernaneAvatar = usernaneAvatarTmp
				imagePathAvatar = getImagePath()
			}
		}
	}
	
	private func computeNotificationsCount() {
		CoreContext.shared.doOnCoreQueue { core in
			let count = self.account.unreadChatMessageCount + self.account.missedCallsCount
			SharedMainViewModel.shared.updateMissedCallsCount()
			SharedMainViewModel.shared.updateUnreadMessagesCount()
			
			DispatchQueue.main.async {
				self.notificationsCount = count
			}
		}
	}
	
	func refreshRegiter() {
		CoreContext.shared.doOnCoreQueue { _ in
			self.account.refreshRegister()
		}
	}
	
	func getImagePath() -> URL {
		let imagePath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first!.appendingPathComponent(
			photoAvatarModel ?? "Error"
		)
		
		return imagePath
	}
	
	func requestDevicesList() {
		if account.params != nil && account.params!.identityAddress != nil, let identityAddress = account.params!.identityAddress {
			Log.info(
				"\(AccountModel.TAG) Request devices list for identity address \(identityAddress.asStringUriOnly())"
			)
			CoreContext.shared.doOnCoreQueue { core in
				do {
					self.accountManagerServices = try core.createAccountManagerServices()
					if self.accountManagerServices != nil {
						self.accountManagerServices!.language = Locale.current.identifier
						
						do {
							let request = try self.accountManagerServices!.createGetDevicesListRequest(sipIdentity: identityAddress)
							self.addDelegate(request: request)
						} catch {
							print("\(AccountModel.TAG) Failed to create request: \(error.localizedDescription)")
						}
					}
				} catch {
					
				}
			}
		}
	}
	
	func addDelegate(request: AccountManagerServicesRequest) {
		self.requestDelegate = AccountManagerServicesRequestDelegateStub(
			onRequestSuccessful: { (request: AccountManagerServicesRequest, data: String) in
				Log.info("\(AccountModel.TAG) Request \(request) was successful, data is \(data)")
			}, onRequestError: { (request: AccountManagerServicesRequest, statusCode: Int, errorMessage: String, parameterErrors: Dictionary?) in
				Log.error(
					"\(AccountModel.TAG) Request \(request) returned an error with status code \(statusCode) and message \(errorMessage)"
				)
				// TODO Display Error Toast
			}, onDevicesListFetched: { (request: AccountManagerServicesRequest, accountDevices: [AccountDevice]) in
				Log.info("\(AccountModel.TAG) Fetched \(accountDevices.count) devices for our account")
				var devicesList: [AccountDeviceModel] = []
				accountDevices.forEach { accountDevice in
					devicesList.append(AccountDeviceModel(accountDevice: accountDevice))
				}
				
				request.removeDelegate(delegate: self.requestDelegate!)
				DispatchQueue.main.async {
					self.devices = devicesList
				}
			}
		)
		
		request.addDelegate(delegate: self.requestDelegate!)
		request.submit()
	}
	
	func removeDevice(deviceIndex: Int) {
		let removedDevice = self.devices[deviceIndex].accountDevice
		self.devices.remove(at: deviceIndex)
		if account.params != nil && account.params!.identityAddress != nil, let identityAddress = account.params!.identityAddress {
			Log.info(
				"\(AccountModel.TAG) Delete device for identity address \(identityAddress.asStringUriOnly())"
			)
			CoreContext.shared.doOnCoreQueue { core in
				do {
					self.accountManagerServices = try core.createAccountManagerServices()
					if self.accountManagerServices != nil {
						self.accountManagerServices!.language = Locale.current.identifier
						
						do {
							let request = try self.accountManagerServices!.createDeleteDeviceRequest(sipIdentity: identityAddress, device: removedDevice)
							self.addDelegate(request: request)
						} catch {
							print("\(AccountModel.TAG) Failed to create request: \(error.localizedDescription)")
						}
					}
				} catch {
					
				}
			}
		}
	}
	
	func logout() {
		CoreContext.shared.doOnCoreQueue { core in
			Log.info("Account \(self.account.displayName()) has been removed")
			core.removeAccount(account: self.account)
			
			if let authInfo = self.account.findAuthInfo() {
				core.removeAuthInfo(info: authInfo)
			}
		}
	}
	
	func saveImage(image: UIImage, name: String, prefix: String) {
		guard let data = image.jpegData(compressionQuality: 1) ?? image.pngData() else {
			return
		}
		
		let photoAvatarModelKey = name
		
		ContactsManager.shared.awaitDataWrite(data: data, name: name, prefix: prefix) { result in
			UserDefaults.standard.set(result, forKey: photoAvatarModelKey)
			
			self.photoAvatarModel = ""
			self.imagePathAvatar = nil
			NotificationCenter.default.post(name: NSNotification.Name("ImageChanged"), object: nil)
			
			DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
				self.photoAvatarModel = result
				self.imagePathAvatar = self.getImagePath()
				NotificationCenter.default.post(name: NSNotification.Name("ImageChanged"), object: nil)
			}
		}
	}
	
	func setAsDefault() {
		CoreContext.shared.doOnCoreQueue { core in
			if core.defaultAccount?.displayName() != self.account.displayName() {
				core.defaultAccount = self.account
				
				for friendList in core.friendsLists {
					if (friendList.subscriptionsEnabled) {
						Log.info(
							"\(AccountModel.TAG) Default account has changed, refreshing friend list \(friendList.displayName ?? "") subscriptions"
						)
						// friendList.updateSubscriptions() won't trigger a refresh unless a friend has changed
						friendList.subscriptionsEnabled = false
						friendList.subscriptionsEnabled = true
					}
				}
			}
		}
		
		self.isDefaultAccount = true
	}
}

class AccountDeviceModel: ObservableObject {
	let accountDevice: AccountDevice
	@Published var deviceName: String = ""
	@Published var lastDate: String = ""
	@Published var lastTime: String = ""
	@Published var isMobileDevice: Bool = true
	
	init(accountDevice: AccountDevice) {
		self.accountDevice = accountDevice
		self.deviceName = accountDevice.name ?? ""
		
		let timeInterval = TimeInterval(accountDevice.lastUpdateTimestamp ?? 0)
		let dateTmp = Date(timeIntervalSince1970: timeInterval)
		
		let dateFormat = DateFormatter()
		dateFormat.dateFormat = Locale.current.identifier == "fr_FR" ? "dd/MM/YYYY" : "MM/dd/YYYY"
		let date = dateFormat.string(from: dateTmp)
		
		let dateFormatBis = DateFormatter()
		dateFormatBis.dateFormat = "HH:mm"
		let time = dateFormatBis.string(from: dateTmp)
		
		self.lastDate = date
		self.lastTime = time
		
		self.isMobileDevice = accountDevice.userAgent.contains("LinphoneAndroid") || accountDevice.userAgent.contains(
			"LinphoneiOS"
		)
	}
}
