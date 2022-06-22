//
//  ContactListMagicSearch.swift
//  linphone
//
//  Created by QuentinArguillere on 25/03/2022.
//

import Foundation
import linphonesw

@objc class MagicSearchSingleton : NSObject {
	static var theMagicSearchSingleton: MagicSearchSingleton?
	
	var lc = CallManager.instance().lc
	var ongoingSearch = false
	var needUpdateLastSearchContacts = false
	var lastSearchContacts : [Contact] = []
	
	@objc var currentFilter : String = ""
	var previousFilter : String?
	var magicSearch : MagicSearch
	var magicSearchDelegate : MagicSearchDelegate?
	
	
	override init() {
		magicSearch = try! lc!.createMagicSearch()
		magicSearch.limitedSearch = false
		super.init()
		
		magicSearchDelegate = MagicSearchDelegateStub(onSearchResultsReceived: { (magicSearch: MagicSearch) in
			self.needUpdateLastSearchContacts = true
			self.ongoingSearch = false
			Log.directLog(BCTBX_LOG_MESSAGE, text: "Contact magic search -- filter = \(String(describing: self.previousFilter)) -- \(magicSearch.lastSearch.count) contact founds")
			NotificationCenter.default.post(name: Notification.Name(kLinphoneMagicSearchFinished), object: self)
		}, onLdapHaveMoreResults: { (magicSearch: MagicSearch, ldap: Ldap) in
			Log.directLog(BCTBX_LOG_MESSAGE, text: "Ldap have more result")
		})
		
		magicSearch.addDelegate(delegate: magicSearchDelegate!)
	}
	
	
	@objc static func instance() -> MagicSearchSingleton {
		if (theMagicSearchSingleton == nil) {
			theMagicSearchSingleton = MagicSearchSingleton()
		}
		return theMagicSearchSingleton!
	}
	
	
	func getContactFromAddr(addr: Address) -> Contact? {
		return LinphoneManager.instance().fastAddressBook.addressBookMap.object(forKey: addr.asStringUriOnly() as Any) as? Contact
	}
	func getContactFromPhoneNb(phoneNb: String) -> Contact? {
		let contactKey =  FastAddressBook.localizedLabel(FastAddressBook.normalizeSipURI( lc?.defaultAccount?.normalizePhoneNumber(username: phoneNb) ?? phoneNb))
		return LinphoneManager.instance().fastAddressBook.addressBookMap.object(forKey: contactKey as Any) as? Contact
	}
	
	func searchAndAddMatchingContact(searchResult: SearchResult) -> Contact? {
		if let friend = searchResult.friend {
			if let addr = friend.address, let foundContact = getContactFromAddr(addr: addr) {
				return foundContact
			}
			for phoneNb in friend.phoneNumbers {
				if let foundContact = getContactFromPhoneNb(phoneNb: phoneNb) {
					return foundContact
				}
			}
			// No contacts found (searchResult likely comes from LDAP), creating a new one
			if let newContact = Contact(friend: friend.getCobject) {
				newContact.createdFromLdap = true
				return newContact
			}
		}
		
		if let addr = searchResult.address, let foundContact = getContactFromAddr(addr: addr)  {
			return foundContact
		}
		
		if let foundContact = getContactFromPhoneNb(phoneNb: searchResult.phoneNumber)  {
			return foundContact
		}
		
		return nil
	}
	
	@objc func isSearchOngoing() -> Bool {
		return ongoingSearch
	}
	
	@objc func getLastSearchResults() -> UnsafeMutablePointer<bctbx_list_t>? {
		
		var cList: UnsafeMutablePointer<bctbx_list_t>? = nil
		for data in magicSearch.lastSearch {
			cList = bctbx_list_append(cList, UnsafeMutableRawPointer(data.getCobject))
		}
		return cList
	}
	
	@objc func getLastSearchContacts() -> [Contact] {
		if (needUpdateLastSearchContacts) {
			lastSearchContacts = []
			for res in magicSearch.lastSearch {
				if let contact = searchAndAddMatchingContact(searchResult: res) {
					lastSearchContacts.append(contact)
				}
			}
			needUpdateLastSearchContacts = false
		}
		
		return lastSearchContacts
	}
	
	@objc func searchForContacts(domain: String, sourceFlags: Int, clearCache: Bool) {
		if (clearCache) {
			magicSearch.resetSearchCache()
		}
		if let oldFilter = previousFilter {
			if (oldFilter.count > currentFilter.count || oldFilter != currentFilter) {
				magicSearch.resetSearchCache()
			}
		}
		previousFilter = currentFilter
		
		ongoingSearch = true
		DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
			if (self.ongoingSearch) {
				NotificationCenter.default.post(name: Notification.Name(kLinphoneMagicSearchStarted), object: self)
			}
		}
		magicSearch.getContactsListAsync(filter: currentFilter, domain: domain, sourceFlags: sourceFlags, aggregation: MagicSearchAggregation.Friend)
	}
	
	
	func setupLDAPTestSettings() {
	}
}
