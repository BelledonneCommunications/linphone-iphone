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
import AppAuth

class SingleSignOnManager {
	
	static let shared = SingleSignOnManager()

	private let TAG = "[SSO]"
	private let clientId = "linphone"
	private let userDefaultSSOKey = "sso-authstate"
	let ssoRedirectUri = URL(string: "org.linphone:/openidcallback")!
	private var singleSignOnUrl = ""
	private var username: String = ""
	private var authState: AuthState?
	private var authService: OIDAuthorizationService?
	var currentAuthorizationFlow: OIDExternalUserAgentSession?
	
	func persistedAuthState() -> AuthState? {
		if let persistentAuthState =  UserDefaults.standard.object(forKey: userDefaultSSOKey), let fromDictionary = persistentAuthState as? [String: Any] {
			return AuthState(dictionary: fromDictionary)
		} else {
			return nil
		}
	}
	
	func persistAuthState() {
		if let authState = authState {
			UserDefaults.standard.set(authState.asDictionary, forKey: userDefaultSSOKey)
		}
	}
	
	func setUp(ssoUrl: String, user: String = "") {
		singleSignOnUrl = ssoUrl
		username = user
		Log.info("\(TAG) Setting up SSO environment for username \(username) and URL \(singleSignOnUrl)")
		authState = persistedAuthState()
		updateTokenInfo()
	}
	
	private func updateTokenInfo() {
		Log.info("\(TAG) Updating token info")
		if authState?.isAuthorized == true {
			Log.info("\(TAG) User is already authenticated!")
			if let expiration = authState?.accessTokenExpirationTime {
				if expiration < Date() {
					Log.warn("\(TAG) Access token is expired")
					performRefreshToken()
				} else {
					Log.info("\(TAG) Access token valid, expires \(expiration)")
					storeTokensInAuthInfo()
				}
			} else {
				Log.warn("\(TAG) Access token expiration info not available")
				singleSignOn()
			}
		} else {
			Log.warn("\(TAG) User isn't authenticated yet")
			singleSignOn()
		}
	}
	
	private func performRefreshToken() {
		Log.info("\(TAG) Refreshing token")
		if let issuer = URL(string: singleSignOnUrl) {
			OIDAuthorizationService.discoverConfiguration(forIssuer: issuer) { configuration, error in
				guard let configuration = configuration, let refreshToken = self.authState?.refreshToken else {
					Log.error("\(self.TAG) Error retrieving discovery document: \(error?.localizedDescription ?? "Unknown error")")
					return
				}
				let request = OIDTokenRequest(
					configuration: configuration,
					grantType: OIDGrantTypeRefreshToken,
					authorizationCode: nil,
					redirectURL: nil,
					clientID: self.clientId,
					clientSecret: nil,
					scope: nil,
					refreshToken: refreshToken,
					codeVerifier: nil,
					additionalParameters: nil)
				
				OIDAuthorizationService.perform(request) { tokenResponse, error in
					if error != nil {
						Log.error("\(self.TAG) Error occured refreshing token \(String(describing: error))")
						self.authState = nil
						self.singleSignOn()
						return
					}
					if let tokenResponse = tokenResponse, tokenResponse.accessToken != nil {
						Log.info("\(self.TAG) Refreshed token \(String(describing: tokenResponse.accessToken))")
						self.authState?.update(tokenResponse: tokenResponse)
						self.storeTokensInAuthInfo()
					} else {
						Log.info("\(self.TAG) refresh token response or access token is empty")
						self.authState = nil
						self.singleSignOn()
					}
				}
			}
		}
	}
	
	private func singleSignOn() {
		if let issuer = URL(string: singleSignOnUrl) {
			OIDAuthorizationService.discoverConfiguration(forIssuer: issuer) { configuration, error in
				guard let configuration = configuration else {
					Log.error("\(self.TAG) Error retrieving discovery document: \(error?.localizedDescription ?? "Unknown error")")
					return
				}
				
				let request = OIDAuthorizationRequest(configuration: configuration,
													  clientId: self.clientId,
													  scopes: ["offline_access"],
													  redirectURL: self.ssoRedirectUri,
													  responseType: OIDResponseTypeCode,
													  additionalParameters: ["login_hint": self.username])

				Log.info("\(self.TAG) Initiating authorization request with scope: \(request.scope ?? "nil")")
				if let viewController = UIApplication.getTopMostViewController() {
					self.currentAuthorizationFlow =
					OIDAuthState.authState(byPresenting: request, presenting: viewController) { authState, error in
						if let authState = authState {
							self.authState = AuthState(oidAuthState: authState)
							self.persistAuthState()
							Log.info("\(self.TAG) Got authorization tokens. Access token: " +
									 "\(authState.lastTokenResponse?.accessToken ?? "nil")")
							self.storeTokensInAuthInfo()
						} else {
							Log.info("\(self.TAG) Authorization error: \(error?.localizedDescription ?? "Unknown error")")
							self.authState = nil
						}
					}
				}
			}
		}
	}
	
	private func storeTokensInAuthInfo() {
		CoreContext.shared.doOnCoreQueue { core in
			if let expire = self.authState?.accessTokenExpirationTime?.timeIntervalSince1970,
			   let accessToken = self.authState?.accessToken,
			   let lAccessToken = try?Factory.Instance.createBearerToken(token: accessToken, expirationTime: Int(expire)),
			   let refreshToken = self.authState?.refreshToken,
			   let lRefreshToken = try?Factory.Instance.createBearerToken(token: refreshToken, expirationTime: Int(expire)),
			   let authInfo = CoreContext.shared.bearerAuthInfoPendingPasswordUpdate {
				authInfo.accessToken = lAccessToken
				authInfo.refreshToken = lRefreshToken
				authInfo.tokenEndpointUri = self.authState?.tokenEndpointUri
				authInfo.clientId = self.clientId
				core.addAuthInfo(info: authInfo)
				Log.info("\(self.TAG) Auth info added username=\(self.username) access token=\(accessToken) refresh token=\(refreshToken) expire=\(expire)")
				core.refreshRegisters()
			} else {
				Log.warn("\(self.TAG) Unable to store SSO details in auth info")
			}
		}
	}
}
