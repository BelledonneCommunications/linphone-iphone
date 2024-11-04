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
import Combine

final class MagicSearchSingleton: ObservableObject {
	
	static let shared = MagicSearchSingleton()
	private var coreContext = CoreContext.shared
	private var contactsManager = ContactsManager.shared
	
	private var magicSearch: MagicSearch!
	
	var currentFilter: String = ""
	var previousFilter: String?
	
	var currentFilterSuggestions: String = ""
	var previousFilterSuggestions: String?
	
	var needUpdateLastSearchContacts = false
	
	private var limitSearchToLinphoneAccounts = true
	
	@Published var allContact = false
	private var domainDefaultAccount = ""
	
	var searchDelegate: MagicSearchDelegate?
	
	func destroyMagicSearch() {
		magicSearch = nil
	}
	
	private init() {
		coreContext.doOnCoreQueue { core in
			self.domainDefaultAccount = core.defaultAccount?.params?.domain ?? ""
			
			self.magicSearch = try? core.createMagicSearch()
			self.magicSearch.limitedSearch = false
			
			self.searchDelegate = MagicSearchDelegateStub(onSearchResultsReceived: { (magicSearch: MagicSearch) in
				self.needUpdateLastSearchContacts = true
				
				var lastSearchFriend: [SearchResult] = []
				var lastSearchSuggestions: [SearchResult] = []
				
				magicSearch.lastSearch.forEach { searchResult in
					if searchResult.friend != nil {
						lastSearchFriend.append(searchResult)
					} else {
						lastSearchSuggestions.append(searchResult)
					}
				}
				lastSearchSuggestions.sort(by: {
					$0.address!.asStringUriOnly() < $1.address!.asStringUriOnly()
				})
				let sortedLastSearch = lastSearchFriend.sorted(by: {
					$0.friend!.name!.lowercased().folding(options: .diacriticInsensitive, locale: .current)
					<
						$1.friend!.name!.lowercased().folding(options: .diacriticInsensitive, locale: .current)
				})
				
				var addedAvatarListModel: [ContactAvatarModel] = []
				sortedLastSearch.forEach { searchResult in
					if searchResult.friend != nil {
						addedAvatarListModel.append(
							ContactAvatarModel(
								friend: searchResult.friend!,
								name: searchResult.friend?.name ?? "",
								address: searchResult.friend?.address?.clone()?.asStringUriOnly() ?? "",
								withPresence: true
							)
						)
					}
				}
				
				DispatchQueue.main.async {
					self.contactsManager.lastSearch = sortedLastSearch
					self.contactsManager.lastSearchSuggestions = lastSearchSuggestions
					
					self.contactsManager.avatarListModel.forEach { contactAvatarModel in
						contactAvatarModel.removeFriendDelegate()
					}
					self.contactsManager.avatarListModel.removeAll()
					self.contactsManager.avatarListModel += addedAvatarListModel
					
					NotificationCenter.default.post(name: NSNotification.Name("ContactLoaded"), object: nil)
				}
			})
			self.magicSearch.addDelegate(delegate: self.searchDelegate!)
		}
	}
	
	func searchForContacts(sourceFlags: Int) {
		coreContext.doOnCoreQueue { _ in
			var needResetCache = false
			
			DispatchQueue.main.sync {
				if let oldFilter = self.previousFilter {
					if oldFilter.count > self.currentFilter.count || oldFilter != self.currentFilter {
						needResetCache = true
					}
				}
				self.previousFilter = self.currentFilter
			}
			if needResetCache {
				self.magicSearch.resetSearchCache()
			}
			
			self.magicSearch.getContactsListAsync(
				filter: self.currentFilter,
				domain: self.allContact ? "" : self.domainDefaultAccount,
				sourceFlags: sourceFlags,
				aggregation: MagicSearch.Aggregation.Friend)
		}
	}
	
	func searchForSuggestions() {
		coreContext.doOnCoreQueue { _ in
			var needResetCache = false
			
			DispatchQueue.main.sync {
				if let oldFilter = self.previousFilterSuggestions {
					if oldFilter.count > self.currentFilterSuggestions.count || oldFilter != self.currentFilterSuggestions {
						needResetCache = true
					}
				}
				self.previousFilterSuggestions = self.currentFilterSuggestions
			}
			if needResetCache {
				self.magicSearch.resetSearchCache()
			}
			
			self.magicSearch.getContactsListAsync(
				filter: self.currentFilterSuggestions,
				domain: self.domainDefaultAccount,
				sourceFlags: MagicSearch.Source.All.rawValue,
				aggregation: MagicSearch.Aggregation.Friend)
		}
	}
}
