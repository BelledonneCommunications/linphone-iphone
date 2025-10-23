/*
 * Copyright (c) 2010-2023 Belledonne Communications SARL.
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

import linphonesw
import SwiftUI

class SharedMainViewModel: ObservableObject {
	
	static let shared = SharedMainViewModel()
	
	@Published var welcomeViewDisplayed = false
	@Published var generalTermsAccepted = false
	@Published var displayProfileMode = false
	@Published var defaultAvatar: URL?
	@Published var indexView: Int = 0
	
	@Published var displayedFriend: ContactAvatarModel?
	@Published var displayedCall: HistoryModel?
	@Published var displayedConversation: ConversationModel?
	@Published var displayedMeeting: MeetingModel?
	
	@Published var dialPlansList: [DialPlan?] = []
	@Published var dialPlansLabelList: [String] = []
	@Published var dialPlansShortLabelList: [String] = []
	
	@Published var fileUrlsToShare: [String] = []
	
	@Published var operationInProgress = false
    
    @Published var unreadMessages: Int = 0
    @Published var missedCallsCount: Int = 0
	@Published var cardDavFriendsListsCount: Int = 0
	
	@Published var disableChatFeature: Bool = false
	@Published var disableMeetingFeature: Bool = false
	
	let welcomeViewKey = "welcome_view"
	let generalTermsKey = "general_terms"
	let displayProfileModeKey = "display_profile_mode"
	let defaultAvatarKey = "default_avatar"
	let indexViewKey = "index_view"
	
	var maxWidth = 600.0
	
	private init() {
		let preferences = UserDefaults.standard
		
		if preferences.object(forKey: indexViewKey) == nil {
			preferences.set(indexView, forKey: indexViewKey)
		} else {
			indexView = preferences.integer(forKey: indexViewKey)
		}
		
		if preferences.object(forKey: welcomeViewKey) == nil {
			preferences.set(welcomeViewDisplayed, forKey: welcomeViewKey)
		} else {
			welcomeViewDisplayed = preferences.bool(forKey: welcomeViewKey)
		}
		
		if preferences.object(forKey: generalTermsKey) == nil {
			preferences.set(generalTermsAccepted, forKey: generalTermsKey)
		} else {
			generalTermsAccepted = preferences.bool(forKey: generalTermsKey)
		}
		
		if preferences.object(forKey: displayProfileModeKey) == nil {
			preferences.set(displayProfileMode, forKey: displayProfileModeKey)
		} else {
			displayProfileMode = preferences.bool(forKey: displayProfileModeKey)
		}
		
		if preferences.object(forKey: defaultAvatarKey) == nil {
			preferences.set(defaultAvatar, forKey: defaultAvatarKey)
		} else {
			if let defaultAvatarTmp = preferences.url(forKey: defaultAvatarKey) {
				defaultAvatar = defaultAvatarTmp
			}
		}
        
        updateMissedCallsCount()
        updateUnreadMessagesCount()
		updateDisableChatFeature()
		updateDisableMeetingFeature()
		
		getCardDavFriendsListsCount()
	}
	
	func changeWelcomeView() {
		let preferences = UserDefaults.standard
		
		welcomeViewDisplayed = true
		preferences.set(welcomeViewDisplayed, forKey: welcomeViewKey)
	}
	
	func changeGeneralTerms() {
		let preferences = UserDefaults.standard
		
		generalTermsAccepted = true
		preferences.set(generalTermsAccepted, forKey: generalTermsKey)
	}
	
	func changeDisplayProfileMode() {
		let preferences = UserDefaults.standard
		
		displayProfileMode = true
		preferences.set(displayProfileMode, forKey: displayProfileModeKey)
	}
	
	func changeHideProfileMode() {
		let preferences = UserDefaults.standard
		
		displayProfileMode = false
		preferences.set(displayProfileMode, forKey: displayProfileModeKey)
	}
	
	func changeDefaultAvatar(defaultAvatarURL: URL) {
		let preferences = UserDefaults.standard
		
		defaultAvatar = defaultAvatarURL
		preferences.set(defaultAvatar, forKey: defaultAvatarKey)
	}
	
	func changeIndexView(indexViewInt: Int) {
		let preferences = UserDefaults.standard
		
		indexView = indexViewInt
		preferences.set(indexView, forKey: indexViewKey)
	}
	
	func getDialPlansList() {
		CoreContext.shared.doOnCoreQueue { _ in
			let dialPlans = Factory.Instance.dialPlans
			var dialPlansListTmp: [DialPlan?] = []
			var dialPlansLabelListTmp: [String] = []
			var dialPlansShortLabelListTmp: [String] = []
			
			dialPlansListTmp.append(nil)
			dialPlansLabelListTmp.append("No country code")
			dialPlansShortLabelListTmp.append("---")
			
			dialPlans.forEach { dialPlan in
				dialPlansListTmp.append(dialPlan)
				dialPlansLabelListTmp.append(
					"\(dialPlan.flag) \(dialPlan.country) | +\(dialPlan.countryCallingCode)"
				)
				dialPlansShortLabelListTmp.append(
					"\(dialPlan.flag) +\(dialPlan.countryCallingCode)"
				)
			}
			
			DispatchQueue.main.async {
				self.dialPlansList = dialPlansListTmp
				self.dialPlansLabelList = dialPlansLabelListTmp
				self.dialPlansShortLabelList = dialPlansShortLabelListTmp
			}
		}
	}
    
    func resetMissedCallsCount() {
        CoreContext.shared.doOnCoreQueue { core in
            let account = core.defaultAccount
            if account != nil {
                account?.resetMissedCallsCount()
                DispatchQueue.main.async {
                    self.missedCallsCount = 0
                }
            } else {
                DispatchQueue.main.async {
                    self.missedCallsCount = 0
                }
            }
        }
    }
    
    func updateMissedCallsCount() {
        CoreContext.shared.doOnCoreQueue { core in
            let account = core.defaultAccount
            if account != nil {
                let count = account?.missedCallsCount != nil ? account!.missedCallsCount : core.missedCallsCount
                
                DispatchQueue.main.async {
                    self.missedCallsCount = count
                }
            } else {
                DispatchQueue.main.async {
                    self.missedCallsCount = 0
                }
            }
        }
    }
    
    func updateUnreadMessagesCount() {
        CoreContext.shared.doOnCoreQueue { core in
            let account = core.defaultAccount
            if account != nil {
                let count = account?.unreadChatMessageCount != nil ? account!.unreadChatMessageCount : core.unreadChatMessageCount
                
                DispatchQueue.main.async {
                    self.unreadMessages = count
                    UIApplication.shared.applicationIconBadgeNumber = count
                }
            } else {
                DispatchQueue.main.async {
                    self.unreadMessages = 0
                    UIApplication.shared.applicationIconBadgeNumber = 0
                }
            }
        }
    }
	
	func updateDisableChatFeature() {
		CoreContext.shared.doOnCoreQueue { core in
			let disableChatFeatureTmp = CorePreferences.disableChatFeature
			
			DispatchQueue.main.async {
				self.disableChatFeature = disableChatFeatureTmp
			}
		}
	}
	
	func updateDisableMeetingFeature() {
		CoreContext.shared.doOnCoreQueue { core in
			let disableMeetingFeatureTmp = CorePreferences.disableMeetings ||
			!LinphoneUtils.isRemoteConferencingAvailable(core: core)
			DispatchQueue.main.async {
				self.disableMeetingFeature = disableMeetingFeatureTmp
			}
		}
	}
	
	func getCardDavFriendsListsCount() {
		CoreContext.shared.doOnCoreQueue { core in
			var list: [String] = []
			
			core.friendsLists.forEach({ friendList in
				if friendList.type == .CardDAV {
					let label = friendList.displayName ?? friendList.uri ?? ""
					if !label.isEmpty {
						list.append(label)
					}
				}
			})

			DispatchQueue.main.async {
				self.updateCardDavFriendsListsCount(cardDavFriendsListsCount: list.count)
			}
		}
	}
	
	func updateCardDavFriendsListsCount(cardDavFriendsListsCount: Int) {
		self.cardDavFriendsListsCount = cardDavFriendsListsCount
	}
}
