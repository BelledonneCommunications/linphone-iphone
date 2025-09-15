/*
 * Copyright (c) 2010-2024 Belledonne Communications SARL.
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

class MeetingsListViewModel: ObservableObject {
	static let TAG = "[Meetings ListViewModel]"
	static let ScrollToTodayNotification = Notification.Name("ScrollToToday")
	
	private var coreContext = CoreContext.shared
	private var mMeetingsListCoreDelegate: CoreDelegate?
	var selectedMeetingToDelete: MeetingModel?
	
	@Published var meetingsList: [MeetingsListItemModel] = []
	@Published var currentFilter = ""
	@Published var todayIdx = 0
	
	init() {
		coreContext.doOnCoreQueue { core in
			self.mMeetingsListCoreDelegate = CoreDelegateStub(onConferenceInfoReceived: { (_: Core, conferenceInfo: ConferenceInfo) in
				Log.info("\(MeetingsListViewModel.TAG) Conference info received [\(conferenceInfo.uri?.asStringUriOnly() ?? "NIL")")
				self.computeMeetingsList()
			})
			core.addDelegate(delegate: self.mMeetingsListCoreDelegate!)
		}
		computeMeetingsList()
	}
	
	func computeMeetingsList() {
		let filter = self.currentFilter.uppercased()
		let isFiltering = !filter.isEmpty
		
		coreContext.doOnCoreQueue { core in
			var confInfoList: [ConferenceInfo] = []
			
			if let account = core.defaultAccount {
				confInfoList = account.conferenceInformationList
			}
			
			var meetingsListTmp: [MeetingsListItemModel] = []
			var meetingForTodayFound = false
			var currentIdx = 0
			var todayIdx = 0
			for confInfo in confInfoList {
				if confInfo.duration == 0 { continue }// This isn't a scheduled conference, don't display it
				var add = true
				if !filter.isEmpty {
					let organizerCheck = confInfo.organizer?.asStringUriOnly().range(of: filter, options: .caseInsensitive) != nil
					let subjectCheck = confInfo.subject?.range(of: filter, options: .caseInsensitive) != nil
					let descriptionCheck = confInfo.description?.range(of: filter, options: .caseInsensitive) != nil
					let participantsCheck = confInfo.participantInfos.first(
						where: {$0.address?.asStringUriOnly().range(of: filter, options: .caseInsensitive) != nil}
					) != nil
					
					add = organizerCheck || subjectCheck || descriptionCheck || participantsCheck
				}
				
				if add {
					let model = MeetingModel(conferenceInfo: confInfo)
					
					if !meetingForTodayFound && !isFiltering {
						if model.isToday {
							meetingForTodayFound = true
							todayIdx = currentIdx
						} else if model.isAfterToday {
							// If no meeting was found for today, insert "Today" fake model before the next meeting to come
							meetingsListTmp.append(MeetingsListItemModel(meetingModel: nil))
							meetingForTodayFound = true
							todayIdx = currentIdx
						}
					}
					
					var matchFilter = !isFiltering
					if isFiltering {
							matchFilter = matchFilter || confInfo.subject?.uppercased().contains(filter) ?? false
							matchFilter = matchFilter || confInfo.description?.uppercased().contains(filter) ?? false
							matchFilter = matchFilter || confInfo.organizer?.asStringUriOnly().uppercased().contains(filter) ?? false
							for pInfo in confInfo.participantInfos {
								matchFilter = matchFilter || pInfo.address?.asStringUriOnly().uppercased().contains(filter) ?? false
							}
					}
					
					if matchFilter {
						meetingsListTmp.append(MeetingsListItemModel(meetingModel: model))
						currentIdx += 1
					}
				}
			}
			
			if !meetingForTodayFound && !isFiltering && !meetingsListTmp.isEmpty {
				// All meetings in the list happened in the past, add "Today" fake model at the end
				meetingsListTmp.append(MeetingsListItemModel(meetingModel: nil))
				todayIdx = currentIdx
			}
			
			DispatchQueue.main.async {
				self.todayIdx = todayIdx
				self.meetingsList = meetingsListTmp
			}
			
			DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
				NotificationCenter.default.post(name: MeetingsListViewModel.ScrollToTodayNotification, object: nil)
			}
		}
	}

	func deleteSelectedMeeting() {
		guard let meetingToDelete = selectedMeetingToDelete else {
			Log.error("\(MeetingsListViewModel.TAG) Could not delete meeting because none was selected")
			return
		}
		
		coreContext.doOnCoreQueue { core in
			core.deleteConferenceInformation(conferenceInfo: meetingToDelete.confInfo)
		}
		
		if let index = self.meetingsList.firstIndex(where: { $0.model?.address == meetingToDelete.address }) {
			DispatchQueue.main.async {
				if self.todayIdx > index {
					// bump todayIdx one place up
					self.todayIdx -= 1
				}
				self.meetingsList.remove(at: index)
				if self.meetingsList.count == 1 && self.meetingsList[0].model == nil {
					// Only remaining meeting is the fake TodayMeeting, remove it too
					self.meetingsList.removeAll()
				}
				ToastViewModel.shared.toastMessage = "Success_toast_meeting_deleted"
				ToastViewModel.shared.displayToast = true
			}
		}
	}
	
	func cancelMeetingWithNotifications() {
		CoreContext.shared.doOnCoreQueue { core in
			if let meeting = self.selectedMeetingToDelete {
				let conferenceScheduler = try? core.createConferenceScheduler()
				
				//self.mSchedulerDelegate = ConferenceSchedulerDelegateStub(onStateChanged: { (_: ConferenceScheduler, state: ConferenceScheduler.State) in
				let mSchedulerDelegate = ConferenceSchedulerDelegateStub(onStateChanged: { (_: ConferenceScheduler, state: ConferenceScheduler.State) in
					Log.info("\(MeetingViewModel.TAG) Conference state changed \(state)")
					if state == ConferenceScheduler.State.Ready {
                        if !CorePreferences.disableChatFeature {
                            self.sendIcsInvitation(core: core, conferenceScheduler: conferenceScheduler)
                        }
                        
                        DispatchQueue.main.async {
                            self.deleteSelectedMeeting()
                        }
					}
				}, onInvitationsSent: { (_: ConferenceScheduler, failedInvitations: [Address]) in
					
					if failedInvitations.isEmpty {
						Log.info("\(MeetingViewModel.TAG) All invitations have been sent")
					} else {
						var failInvList = ""
						for failInv in failedInvitations {
							if !failInvList.isEmpty {
								failInvList += ", "
							}
							failInvList.append(failInv.asStringUriOnly())
						}
						Log.warn("\(MeetingViewModel.TAG) \(failedInvitations.count) invitations couldn't have been sent to: \(failInvList)")
						DispatchQueue.main.async {
							ToastViewModel.shared.toastMessage = "meeting_failed_to_send_part_of_invites_toast"
							ToastViewModel.shared.displayToast = true
						}
					}
				})
				
				conferenceScheduler?.addDelegate(delegate: mSchedulerDelegate)
				conferenceScheduler?.cancelConference(conferenceInfo: meeting.confInfo)
			}
		}
	}
	
	private func sendIcsInvitation(core: Core, conferenceScheduler: ConferenceScheduler?) {
		if let chatRoomParams = try? core.createDefaultChatRoomParams() {
			chatRoomParams.groupEnabled = false
			chatRoomParams.backend = ChatRoom.Backend.FlexisipChat
			chatRoomParams.encryptionEnabled = true
			chatRoomParams.subject = "Meeting ics"
			conferenceScheduler?.sendInvitations(chatRoomParams: chatRoomParams)
		} else {
			Log.error("\(MeetingViewModel.TAG) Failed to create default chatroom parameters. This should not happen")
		}
	}
}
