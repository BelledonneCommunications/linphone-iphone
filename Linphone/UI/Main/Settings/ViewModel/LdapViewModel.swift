/*
 * Copyright (c) 2010-2025 Belledonne Communications SARL.
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
import Combine
import SwiftUI
import linphonesw

class LdapViewModel: ObservableObject {
	static let TAG = "[LDAP ViewModel]"
	
	private var coreContext = CoreContext.shared
	
	@Published var isEdit: Bool = false
	@Published var isEnabled: Bool = true
	@Published var serverUrl: String = ""
	@Published var bindDn: String = ""
	@Published var password: String = ""
	@Published var useTls: Bool = true
	@Published var searchBase: String = ""
	@Published var searchFilter: String = ""
	@Published var maxResults: String = ""
	@Published var requestTimeout: String = "5"
	@Published var requestDelay: String = "2000"
	@Published var minCharacters: String = "3"
	@Published var nameAttributes: String = ""
	@Published var sipAttributes: String = ""
	@Published var sipDomain: String = ""
	
	var isFormComplete: Bool {
		!serverUrl.isEmpty &&
		!bindDn.isEmpty &&
		!searchBase.isEmpty &&
		!searchFilter.isEmpty &&
		!maxResults.isEmpty &&
		!requestTimeout.isEmpty &&
		!requestDelay.isEmpty &&
		!minCharacters.isEmpty &&
		!nameAttributes.isEmpty &&
		!sipAttributes.isEmpty &&
		!sipDomain.isEmpty
	}
	
	@Published var ldapServerOperationSuccessful = false
	
	private var ldapToEdit: Ldap?
	
	init(url: String? = "") {
		isEdit = false
		isEnabled = true
		
		useTls = true
		minCharacters = "3"
		requestTimeout = "5"
		requestDelay = "2000"
		
		if let url = url, !url.isEmpty {
			loadLdap(url: url)
		}
	}
	
	func loadLdap(url: String) {
		self.coreContext.doOnCoreQueue { core in
			if let found = core.ldapList.first(where: { $0.params?.server == url }) {
				let isEditTmp = true
				self.ldapToEdit = found
				if let ldapParams = self.ldapToEdit?.params {
					let isEnabledTmp = ldapParams.enabled
					
					let serverUrlTmp = ldapParams.server
					let bindDnTmp = ldapParams.bindDn ?? ""
					let useTlsTmp = ldapParams.tlsEnabled
					let searchBaseTmp = ldapParams.baseObject
					let searchFilterTmp = ldapParams.filter ?? ""
					let maxResultsTmp = String(ldapParams.maxResults)
					let requestTimeoutTmp = String(ldapParams.timeout)
					let requestDelayTmp = String(ldapParams.delay)
					let minCharactersTmp = String(ldapParams.minChars)
					let nameAttributesTmp = ldapParams.nameAttribute ?? ""
					let sipAttributesTmp = ldapParams.sipAttribute ?? ""
					let sipDomainTmp = ldapParams.sipDomain ?? ""
					Log.info("\(LdapViewModel.TAG) Existing LDAP server values loaded")
					
					DispatchQueue.main.async {
						self.isEdit = isEditTmp
						self.isEnabled = isEnabledTmp
						
						self.serverUrl = serverUrlTmp
						self.bindDn = bindDnTmp
						self.useTls = useTlsTmp
						self.searchBase = searchBaseTmp
						self.searchFilter = searchFilterTmp
						self.maxResults = maxResultsTmp
						self.requestTimeout = requestTimeoutTmp
						self.requestDelay = requestDelayTmp
						self.minCharacters = minCharactersTmp
						self.nameAttributes = nameAttributesTmp
						self.sipAttributes = sipAttributesTmp
						self.sipDomain = sipDomainTmp
					}
				}
			} else {
				print("\(LdapViewModel.TAG) Failed to find LDAP server with URL \(url)!")
				return
			}
		}
	}
	
	func delete() {
		self.coreContext.doOnCoreQueue { core in
			if let ldapToEdit = self.ldapToEdit {
				if self.isEdit {
					let serverUrl = ldapToEdit.params?.server
					core.removeLdap(ldap: ldapToEdit)
					Log.info("\(LdapViewModel.TAG) Removed LDAP config for server URL \(serverUrl)")
					
					DispatchQueue.main.async {
						self.ldapServerOperationSuccessful = true
					}
				}
			}
		}
	}
	
	func addServer() {
		self.coreContext.doOnCoreQueue { core in
			do {
				let server = self.serverUrl
				if server.isEmpty {
					Log.error("\(LdapViewModel.TAG) Server field can't be empty!")
					return
				}
				
				let ldapParams = try core.createLdapParams()
				
				ldapParams.enabled = self.isEnabled == true
				ldapParams.server = server
				ldapParams.bindDn = self.bindDn
				ldapParams.password = self.password
				ldapParams.authMethod = Ldap.AuthMethod.Simple
				ldapParams.tlsEnabled = self.useTls == true
				ldapParams.serverCertificatesVerificationMode = Ldap.CertVerificationMode.Default
				ldapParams.baseObject = self.searchBase
				ldapParams.filter = self.searchFilter
				ldapParams.maxResults = Int(self.maxResults) ?? 0
				ldapParams.timeout = Int(self.requestTimeout) ?? 0
				ldapParams.delay = Int(self.requestDelay) ?? 0
				ldapParams.minChars = Int(self.minCharacters) ?? 0
				ldapParams.nameAttribute = self.nameAttributes
				ldapParams.sipAttribute = self.sipAttributes
				ldapParams.sipDomain = self.sipDomain
				ldapParams.debugLevel = Ldap.DebugLevel.Verbose
				
				if self.isEdit && self.ldapToEdit != nil {
					self.ldapToEdit?.params = ldapParams
					Log.info("\(LdapViewModel.TAG) LDAP changes have been applied")
				} else {
					let ldap = try core.createLdapWithParams(params: ldapParams)
					Log.info("\(LdapViewModel.TAG) New LDAP config created")
				}
				
				DispatchQueue.main.async {
					self.ldapServerOperationSuccessful = true
				}
			} catch let error {
				Log.error("\(LdapViewModel.TAG) Exception while creating LDAP: \(error)")
			}
		}
	}
}

