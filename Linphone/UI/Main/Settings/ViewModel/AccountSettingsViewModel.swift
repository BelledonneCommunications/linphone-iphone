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

class AccountSettingsViewModel: ObservableObject {
	
	static let TAG = "[AccountSettingsViewModel]"
	
	@Published var accountModel: AccountModel
	
	@Published var pushNotification: Bool
	@Published var imEncryptionMandatory: Bool
	@Published var voicemailUri: String
	@Published var mwiUri: String
	@Published var applyInternationalPrefix: Bool
	@Published var replacePlusBy00: Bool
	@Published var stunServerUrl: String
	@Published var enableIce: Bool
	@Published var enableTurn: Bool
	@Published var turnUsername: String
	@Published var turnPassword: String
	@Published var transport: String
	@Published var sipProxyUrl: String
	@Published var outboundProxy: Bool
	@Published var avpf: Bool
	@Published var bundleMode: Bool
	@Published var cpimInBasicConversations: Bool
	@Published var expire: String
	@Published var conferenceFactoryUri: String
	@Published var audioVideoConferenceFactoryUri: String
	@Published var ccmpServerUrl: String
	@Published var limeServerUrl: String
	
	private var natPolicy: NatPolicy?
 	private var natPolicyAuthInfo: AuthInfo?
	
	init(accountModel: AccountModel) {
		self.accountModel = accountModel
		
		self.pushNotification = accountModel.account.params?.pushNotificationAllowed ?? false
		self.imEncryptionMandatory = accountModel.account.params?.instantMessagingEncryptionMandatory ?? false
		self.voicemailUri = accountModel.account.params?.voicemailAddress?.asStringUriOnly() ?? ""
		self.mwiUri = accountModel.account.params?.mwiServerAddress?.asStringUriOnly() ?? ""
		self.applyInternationalPrefix = accountModel.account.params?.useInternationalPrefixForCallsAndChats ?? false
		self.replacePlusBy00 = accountModel.account.params?.dialEscapePlusEnabled ?? false
		
		self.natPolicy = accountModel.account.params?.natPolicy
		self.stunServerUrl = accountModel.account.params?.natPolicy?.stunServer ?? ""
		self.enableIce = accountModel.account.params?.natPolicy?.iceEnabled ?? false
		self.enableTurn = accountModel.account.params?.natPolicy?.turnEnabled ?? false
		let turnUsernameTmp = accountModel.account.params?.natPolicy?.stunServerUsername ?? ""
		self.turnUsername = turnUsernameTmp
		self.turnPassword = ""
		
		let transportTmp = accountModel.account.params?.transport
		if transportTmp == .Tls {
			self.transport = "TLS"
		} else if transportTmp == .Tcp {
			self.transport = "TCP"
		} else if transportTmp == .Udp {
			self.transport = "UDP"
		} else {
			self.transport = "DTLS"
		}
		
		self.sipProxyUrl = accountModel.account.params?.serverAddress?.asStringUriOnly() ?? ""
		self.outboundProxy = accountModel.account.params?.outboundProxyEnabled ?? false
		self.avpf = accountModel.account.avpfEnabled
		self.bundleMode = accountModel.account.params?.rtpBundleEnabled ?? false
		self.cpimInBasicConversations = accountModel.account.params?.cpimInBasicChatRoomEnabled ?? false
		self.expire = accountModel.account.params?.expires.description ?? ""
		self.conferenceFactoryUri = accountModel.account.params?.conferenceFactoryAddress?.asStringUriOnly() ?? ""
		self.audioVideoConferenceFactoryUri = accountModel.account.params?.audioVideoConferenceFactoryAddress?.asStringUriOnly() ?? ""
		self.ccmpServerUrl = accountModel.account.params?.ccmpServerUrl ?? ""
		self.limeServerUrl = accountModel.account.params?.limeServerUrl ?? ""
		
		if !turnUsernameTmp.isEmpty {
			CoreContext.shared.doOnCoreQueue { core in
				let authInfo = core.findAuthInfo(realm: nil, username: turnUsernameTmp, sipDomain: nil)
				if authInfo == nil {
					Log.warn("\(AccountSettingsViewModel.TAG) TURN username not empty but unable to find matching auth info!")
				} else {
					self.natPolicyAuthInfo = authInfo!
					DispatchQueue.main.async {
						self.turnPassword = authInfo!.password ?? ""
					}
				}
				
				if accountModel.account.params?.natPolicy == nil {
					self.natPolicy = try? core.createNatPolicy()
				}
			}
		}
	}
	
	func saveChanges() {
		CoreContext.shared.doOnCoreQueue { core in
			print("\(AccountSettingsViewModel.TAG) Saving changes...")
			
			if let newParams = self.accountModel.account.params?.clone() {
				newParams.pushNotificationAllowed = self.pushNotification
				newParams.remotePushNotificationAllowed = self.pushNotification
				
				newParams.instantMessagingEncryptionMandatory = self.imEncryptionMandatory
				
				if !self.sipProxyUrl.isEmpty {
					if let serverAddress = core.interpretUrl(url: self.sipProxyUrl, applyInternationalPrefix: false) {
						
						var transportTmp: TransportType = .Tls
						
						if self.transport == "TLS" {
							transportTmp = .Tls
						} else if self.transport == "TCP" {
							transportTmp = .Tcp
						} else if self.transport == "UDP" {
							transportTmp = .Udp
						} else {
							transportTmp = .Dtls
						}
						
						try? serverAddress.setTransport(newValue: transportTmp)
						try? newParams.setServeraddress(newValue: serverAddress)
					}
				}
				newParams.outboundProxyEnabled = self.outboundProxy
				
				if let natPolicy = self.natPolicy {
					print("\(AccountSettingsViewModel.TAG) Also applying changes to NAT policy")
					natPolicy.stunServer = self.stunServerUrl
					natPolicy.stunEnabled = !self.stunServerUrl.isEmpty
					natPolicy.iceEnabled = self.enableIce
					natPolicy.turnEnabled = self.enableTurn
					let stunTurnUsername = self.turnUsername
					natPolicy.stunServerUsername = stunTurnUsername
					newParams.natPolicy = natPolicy
					
					if let natPolicyAuthInfo = self.natPolicyAuthInfo {
						if stunTurnUsername.isEmpty {
							print("\(AccountSettingsViewModel.TAG) NAT policy TURN username is now empty, removing existing auth info")
							core.removeAuthInfo(info: natPolicyAuthInfo)
						} else {
							print("\(AccountSettingsViewModel.TAG) Found NAT policy auth info, updating it")
							natPolicyAuthInfo.username = stunTurnUsername
							natPolicyAuthInfo.password = self.turnPassword
						}
					} else if !stunTurnUsername.isEmpty {
						print("\(AccountSettingsViewModel.TAG) No NAT policy auth info found, creating it with")
						if let authInfo = try? Factory.Instance.createAuthInfo(
							username: stunTurnUsername,
							userid: nil,
							passwd: self.turnPassword,
							ha1: nil,
							realm: nil,
							domain: nil
						) {
							core.addAuthInfo(info: authInfo)
						}
					}
				}
				
				newParams.avpfMode = self.avpf ? .Enabled : .Disabled
				
				newParams.rtpBundleEnabled = self.bundleMode
				
				newParams.cpimInBasicChatRoomEnabled = self.cpimInBasicConversations
				
				if !self.mwiUri.isEmpty {
					newParams.mwiServerAddress = core.interpretUrl(url: self.mwiUri, applyInternationalPrefix: false)
				} else {
					newParams.mwiServerAddress = nil
				}
				
				if !self.voicemailUri.isEmpty {
					newParams.voicemailAddress = core.interpretUrl(url: self.voicemailUri, applyInternationalPrefix: false)
				} else {
					newParams.voicemailAddress = nil
				}
				
				newParams.useInternationalPrefixForCallsAndChats = self.applyInternationalPrefix
				newParams.dialEscapePlusEnabled = self.replacePlusBy00
				
				let expireInt: Int = {
					if !self.expire.isEmpty {
						return Int(self.expire) ?? 31536000
					}
					return 31536000
				}()
				
				newParams.expires = expireInt
				
				if !self.conferenceFactoryUri.isEmpty {
					newParams.conferenceFactoryAddress = core.interpretUrl(url: self.conferenceFactoryUri, applyInternationalPrefix: false)
				} else {
					newParams.conferenceFactoryAddress = nil
				}
				
				if !self.audioVideoConferenceFactoryUri.isEmpty {
					newParams.audioVideoConferenceFactoryAddress = core.interpretUrl(url: self.audioVideoConferenceFactoryUri, applyInternationalPrefix: false)
				} else {
					newParams.audioVideoConferenceFactoryAddress = nil
				}
				
				newParams.ccmpServerUrl = self.ccmpServerUrl
				newParams.limeServerUrl = self.limeServerUrl
				
				self.accountModel.account.params = newParams
				
				SharedMainViewModel.shared.updateDisableMeetingFeature()
				
				print("\(AccountSettingsViewModel.TAG) Changes have been saved")
			}
		}
	}
}
