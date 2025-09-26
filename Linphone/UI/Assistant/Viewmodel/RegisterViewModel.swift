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

import Foundation
import linphonesw
import Combine
import SwiftUI

// swiftlint:disable line_length
// swiftlint:disable type_body_length
class RegisterViewModel: ObservableObject {
	
	static let TAG = "[RegisterViewModel]"
	let accountTokenNotification = Notification.Name("AccountCreationTokenReceived")
	
	private var coreContext = CoreContext.shared
	
	@Published var username: String = ""
	@Published var usernameError: String = ""
	@Published var phoneNumber: String = ""
	@Published var phoneNumberError: String = ""
	@Published var passwd: String = ""
	@Published var passwordError: String = ""
	@Published var domain: String = "sip.linphone.org"
	@Published var displayName: String = ""
	@Published var transportType: String = "TLS"
	
	@Published var dialPlanValueSelected: String = "---"
	
	private let HASHALGORITHM = "SHA-256"
	
	private var accountManagerServices: AccountManagerServices?
	private var accountManagerServicesRequest: AccountManagerServicesRequest?
	private var accountCreationToken: String?
	private var accountCreatedAuthInfo: AuthInfo?
	private var accountCreated: Account?
	private var normalizedPhoneNumber: String?
	
	private var requestDelegate: AccountManagerServicesRequestDelegate?
	
	@Published var isLinkActive: Bool = false
	@Published var createInProgress: Bool = false
	
	@Published var otpField = "" {
		didSet {
			guard otpField.count <= 5,
				  otpField.last?.isNumber ?? true else {
				otpField = oldValue
				return
			}
		}
	}
	var otp1: String {
		guard otpField.count >= 1 else {
			return ""
		}
		return String(Array(otpField)[0])
	}
	var otp2: String {
		guard otpField.count >= 2 else {
			return ""
		}
		return String(Array(otpField)[1])
	}
	var otp3: String {
		guard otpField.count >= 3 else {
			return ""
		}
		return String(Array(otpField)[2])
	}
	var otp4: String {
		guard otpField.count >= 4 else {
			return ""
		}
		return String(Array(otpField)[3])
	}
	
	init() {
		SharedMainViewModel.shared.getDialPlansList()
		getAccountCreationToken()
		
		self.usernameError = ""
		self.phoneNumberError = ""
		self.passwordError = ""
		
		NotificationCenter.default.addObserver(forName: accountTokenNotification, object: nil, queue: nil) { notification in
			if !(self.username.isEmpty || self.passwd.isEmpty) {
				if let token = notification.userInfo?["token"] as? String {
					if !token.isEmpty {
						self.accountCreationToken = token
						Log.info(
							"\(RegisterViewModel.TAG) Extracted token \(self.accountCreationToken ?? "Error token") from push payload, creating account"
						)
						self.createAccount()
					} else {
						Log.error("\(RegisterViewModel.TAG) Push payload JSON object has an empty 'token'!")
						self.onFlexiApiTokenRequestError()
					}
				}
			}
		}
	}
	func addDelegate(request: AccountManagerServicesRequest) {
		coreContext.doOnCoreQueue { core in
			self.requestDelegate = AccountManagerServicesRequestDelegateStub(onRequestSuccessful: { (request: AccountManagerServicesRequest, data: String) in
				Log.info("\(RegisterViewModel.TAG) Request \(request) was successful, data is \(data)")
				switch request.type {
				case .CreateAccountUsingToken:
					if !data.isEmpty {
						self.storeAccountInCore(core: core, identity: data)
						self.sendCodeBySms()
					} else {
						Log.error(
							"\(RegisterViewModel.TAG) No data found for createAccountUsingToken request, can't continue!"
						)
					}
					
				case .SendPhoneNumberLinkingCodeBySms:
					DispatchQueue.main.async {
						self.createInProgress = false
						self.isLinkActive = true
					}
					
				case .LinkPhoneNumberUsingCode:
					let account = self.accountCreated
					if account != nil {
						Log.info( "\(RegisterViewModel.TAG) Account \(account?.params?.identityAddress?.asStringUriOnly() ?? "NIL") has been created & activated, setting it as default")
						
						if let assistantLinphone = Bundle.main.path(forResource: "assistant_linphone_default_values", ofType: nil) {
							core.loadConfigFromXml(xmlUri: assistantLinphone)
						}
						
						DispatchQueue.main.async {
							self.createInProgress = false
						}
						
						do {
							try core.addAccount(account: account!)
							core.defaultAccount = account
							request.removeDelegate(delegate: self.requestDelegate!)
							self.requestDelegate = nil
						} catch {
						}
					}
					
				default: break
				}
			}, onRequestError: { (request: AccountManagerServicesRequest, statusCode: Int, errorMessage: String, parameterErrors: Dictionary?) in
				Log.error(
					"\(RegisterViewModel.TAG) Request \(request) returned an error with status code \(statusCode) and message \(errorMessage)"
				)
				
				if !errorMessage.isEmpty {
					DispatchQueue.main.async {
						ToastViewModel.shared.toastMessage = "Error: \(errorMessage)"
						ToastViewModel.shared.displayToast = true
					}
				}
				
				parameterErrors?.keys.forEach({ parameter in
					let parameterErrorMessage = parameterErrors?.getString(key: parameter) ?? ""
					
					DispatchQueue.main.async {
						switch parameter {
						case "username":
							self.usernameError = parameterErrorMessage
						case "password":
							self.passwordError = parameterErrorMessage
						case "phone":
							self.phoneNumberError = parameterErrorMessage
						default:
							break
						}
					}
				})
				
				switch request.type {
				case .SendAccountCreationTokenByPush:
					Log.warn("\(RegisterViewModel.TAG) Cancelling job waiting for push notification")
				default: break
				}
				
				DispatchQueue.main.async {
					self.createInProgress = false
				}
			})
			request.addDelegate(delegate: self.requestDelegate!)
		}
	}
	
	func getAccountCreationToken() {
		coreContext.doOnCoreQueue { core in
			do {
				self.accountManagerServices = try core.createAccountManagerServices()
				if self.accountManagerServices != nil {
					self.accountManagerServices!.language = Locale.current.identifier
				}
			} catch {
				
			}
		}
	}
	
	func startAccountCreation() {
		coreContext.doOnCoreQueue { core in
			if self.accountCreationToken == nil {
				Log.info("\(RegisterViewModel.TAG) We don't have a creation token, let's request one")
				self.requestFlexiApiToken(core: core)
			} else {
				let authInfo = self.accountCreatedAuthInfo
				if authInfo != nil {
					Log.info("\(RegisterViewModel.TAG) Account has already been created, requesting SMS to be sent")
					self.sendCodeBySms()
				} else {
					Log.info("\(RegisterViewModel.TAG) We've already have a token \(self.accountCreationToken ?? ""), continuing")
					self.createAccount()
				}
			}
		}
	}
	
	func storeAccountInCore(core: Core, identity: String) {
		do {
			let passwordValue = passwd
			let sipIdentity = try Factory.Instance.createAddress(addr: identity)
			
			// We need to have an AuthInfo for newly created account to authorize phone number linking request
			let authInfo = try Factory.Instance.createAuthInfo(
				username: sipIdentity.username ?? "Error username",
				userid: nil,
				passwd: passwordValue,
				ha1: nil,
				realm: nil,
				domain: sipIdentity.domain
			)
			
			core.addAuthInfo(info: authInfo)
			Log.info("\(RegisterViewModel.TAG) Auth info for SIP identity \(sipIdentity.asStringUriOnly()) created & added")
			
			var dialPlan: DialPlan?
			
			SharedMainViewModel.shared.dialPlansList.forEach { dial in
				let countryCode = dialPlanValueSelected.components(separatedBy: "+")
				if dial?.countryCallingCode == countryCode[1] {
					dialPlan = dial
				}
			}
			
			let accountParams = try core.createAccountParams()
			try accountParams.setIdentityaddress(newValue: sipIdentity)
			if dialPlan != nil {
				let dialPlanTmp = dialPlan?.internationalCallPrefix ?? "Error international call prefix"
				let isoCountryCodeTmp = dialPlan?.isoCountryCode ?? "Error iso country code"
				Log.info(
					"\(RegisterViewModel.TAG) Setting international prefix \(dialPlanTmp) and country \(isoCountryCodeTmp) to account params"
				)
				accountParams.internationalPrefix = dialPlan!.internationalCallPrefix
				accountParams.internationalPrefixIsoCountryCode = dialPlan!.isoCountryCode
			}
			let account = try core.createAccount(params: accountParams)
			
			Log.info("\(RegisterViewModel.TAG) Account for SIP identity \(sipIdentity.asStringUriOnly()) created & added")
			
			accountCreatedAuthInfo = authInfo
			accountCreated = account
		} catch let error {
			Log.error("\(RegisterViewModel.TAG) Failed to create address from SIP Identity \(identity)!")
			Log.error("\(RegisterViewModel.TAG) Error is \(error)")
		}
	}
	
	func requestFlexiApiToken(core: Core) {
		if !core.isPushNotificationAvailable {
			Log.error(
				"\(RegisterViewModel.TAG) Core says push notification aren't available, can't request a token from FlexiAPI"
			)
			self.onFlexiApiTokenRequestError()
			return
		}
		
		let pushConfig = core.pushNotificationConfig
		if pushConfig != nil && self.accountManagerServices != nil {
#if DEBUG
					let pushEnvironment = ".dev"
#else
					let pushEnvironment = ""
#endif
			pushConfig!.provider = "apns\(pushEnvironment)"
			var formatedPnParam = pushConfig!.param
			formatedPnParam = formatedPnParam?.replacingOccurrences(of: "voip&remote", with: "remote")
			pushConfig!.param = formatedPnParam
			
			let coreRemoteToken = pushConfig!.remoteToken
			var formatedRemoteToken = ""
			if coreRemoteToken != nil {
				formatedRemoteToken = String(coreRemoteToken!.prefix(64))
				pushConfig!.prid = formatedRemoteToken.uppercased()
				do {
					let request = try self.accountManagerServices!.createSendAccountCreationTokenByPushRequest(
						pnProvider: pushConfig?.provider ?? "",
						pnParam: pushConfig?.param ?? "",
						pnPrid: pushConfig?.prid ?? ""
					)
					self.addDelegate(request: request)
					request.submit()
				} catch {
					Log.error("\(RegisterViewModel.TAG) Can't create account creation token by push request")
					self.onFlexiApiTokenRequestError()
				}
			} else {
				Log.error("\(RegisterViewModel.TAG) No remote push token available in core for account creator configuration")
				self.onFlexiApiTokenRequestError()
			}
			
			Log.info("\(RegisterViewModel.TAG) Found push notification info: provider \("apns.dev"), param \(formatedPnParam ?? "error") and prid \(formatedRemoteToken)")
		} else {
			Log.error("\(RegisterViewModel.TAG) No push configuration object in Core, shouldn't happen!")
			self.onFlexiApiTokenRequestError()
		}
	}
	
	func onFlexiApiTokenRequestError() {
		Log.error("\(RegisterViewModel.TAG) Flexi API token request by push error!")
		
		DispatchQueue.main.async {
			self.createInProgress = false
			
			ToastViewModel.shared.toastMessage = "Failed_push_notification_not_received_error"
			ToastViewModel.shared.displayToast = true
		}
	}
	
	func sendCodeBySms() {
		let account = accountCreated
		if accountManagerServices != nil && account != nil {
			let phoneNumberValue = normalizedPhoneNumber
			if phoneNumberValue == nil || phoneNumberValue!.isEmpty {
				Log.error("\(RegisterViewModel.TAG) Phone number is null or empty, this shouldn't happen at this step!")
				return
			}
			
			let identity = account!.params!.identityAddress
			if identity != nil {
				Log.info("\(RegisterViewModel.TAG) Account \(identity!.asStringUriOnly()) should now be created, asking account manager to send a confirmation code by SMS to \(phoneNumberValue ?? "")")
				do {
					let request = try accountManagerServices?.createSendPhoneNumberLinkingCodeBySmsRequest(
						sipIdentity: identity!,
						phoneNumber: phoneNumberValue!
					)
					
					if request != nil {
						self.addDelegate(request: request!)
						request!.submit()
					}
				} catch {
					Log.error("\(RegisterViewModel.TAG) Can't create send phone number linking code by SMS request")
				}
			}
		}
	}
	
	func createAccount() {
		if accountManagerServices != nil {
			let token = accountCreationToken
			if token == nil || (token != nil && token!.isEmpty) {
				Log.error("\(RegisterViewModel.TAG) No account creation token, can't create account!")
				return
			}
			
			if username.isEmpty || passwd.isEmpty {
				Log.error("\(RegisterViewModel.TAG) Either username \(username) or password is null or empty!")
				return
			}
			
			Log.info( "\(RegisterViewModel.TAG) Account creation token is \(token ?? "Error token"), creating account with username \(username) and algorithm \(HASHALGORITHM)")
			
			do {
				let request = try accountManagerServices!.createNewAccountUsingTokenRequest(
					username: username,
					password: passwd,
					algorithm: HASHALGORITHM,
					token: token!
				)
				self.addDelegate(request: request)
				request.submit()
			} catch {
				Log.error("\(RegisterViewModel.TAG) Can't create account using token")
			}
		}
	}
	
	func phoneNumberConfirmedByUser() {
		coreContext.doOnCoreQueue { _ in
			if self.accountManagerServices != nil {
				var dialPlan: DialPlan?
				
				for dial in SharedMainViewModel.shared.dialPlansList {
					let countryCode = self.dialPlanValueSelected.components(separatedBy: "+")
					if dial?.countryCallingCode == countryCode[1] {
						dialPlan = dial
						break
					}
				}
				if dialPlan == nil {
					Log.error("\(RegisterViewModel.TAG) No dial plan (country) selected!")
				}
				
				let number = self.phoneNumber
				let formattedPhoneNumber = dialPlan?.formatPhoneNumber(phoneNumber: number, escapePlus: false)
				Log.info( "\(RegisterViewModel.TAG) Formatted phone number \(number) using dial plan \(dialPlan?.country ?? "Error country") is \(formattedPhoneNumber ?? "Error phone number")")
				
				self.normalizedPhoneNumber = formattedPhoneNumber
			} else {
				Log.error("\(RegisterViewModel.TAG) Account manager services hasn't been initialized!")
				
				DispatchQueue.main.async {
					ToastViewModel.shared.toastMessage = "Failed_account_register_unexpected_error"
					ToastViewModel.shared.displayToast = true
				}
			}
		}
	}
	
	func validateCode() {
		createInProgress = true
		let account = accountCreated
		if accountManagerServices != nil && account != nil {
			let code = otpField
			let identity = account!.params?.identityAddress
			if identity != nil {
				Log.info(
					"\(RegisterViewModel.TAG) Activating account using code \(code) for account \(identity!.asStringUriOnly())"
				)
				
				do {
					let request = try accountManagerServices?.createLinkPhoneNumberToAccountUsingCodeRequest(sipIdentity: identity!, code: code)
					if request != nil {
						self.addDelegate(request: request!)
						request!.submit()
					}
				} catch {
					Log.error("\(RegisterViewModel.TAG) Can't create link phone number to account using code request")
				}
			}
		}
	}
}

// swiftlint:enable line_length
// swiftlint:enable type_body_length
