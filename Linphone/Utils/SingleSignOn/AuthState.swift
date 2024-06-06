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
import AppAuth

class AuthState: Encodable, Decodable {
	var accessToken: String?
	var refreshToken: String?
	var tokenEndpointUri: String?
	var accessTokenExpirationTime: Date?
	var isAuthorized: Bool

	init(oidAuthState: OIDAuthState) {
		accessToken = oidAuthState.lastTokenResponse?.accessToken
		refreshToken = oidAuthState.refreshToken
		tokenEndpointUri = oidAuthState.lastTokenResponse?.request.configuration.tokenEndpoint.absoluteString
		accessTokenExpirationTime = oidAuthState.getAccessTokenExpirationTime()
		isAuthorized = oidAuthState.isAuthorized
	}
	
	func update(tokenResponse: OIDTokenResponse) {
		accessToken = tokenResponse.accessToken
		refreshToken = tokenResponse.refreshToken
		tokenEndpointUri = tokenResponse.request.configuration.tokenEndpoint.absoluteString
		accessTokenExpirationTime = tokenResponse.accessTokenExpirationDate
	}
}
