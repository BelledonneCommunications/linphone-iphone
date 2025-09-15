/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
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

import SwiftUI
import linphonesw
import AVFAudio
import Combine
import SwiftUI

// swiftlint:disable line_length
// swiftlint:disable type_body_length
// swiftlint:disable cyclomatic_complexity
class CallViewModel: ObservableObject {
	
	static let TAG = "[CallViewModel]"
	
	var coreContext = CoreContext.shared
	var telecomManager = TelecomManager.shared
	var sharedMainViewModel = SharedMainViewModel.shared
	
	@Published var displayName: String = ""
	@Published var direction: Call.Dir = .Outgoing
	@Published var remoteAddressString: String = ""
	@Published var remoteAddressCleanedString: String = ""
	@Published var remoteAddress: Address?
	@Published var avatarModel: ContactAvatarModel?
	@Published var micMutted: Bool = false
	@Published var isRecording: Bool = false
	@Published var isRemoteRecording: Bool = false
	@Published var isPaused: Bool = false
	@Published var timeElapsed: Int = 0
	@Published var zrtpPopupDisplayed: Bool = false
	@Published var upperCaseAuthTokenToRead = ""
	@Published var upperCaseAuthTokenToListen = ""
	@Published var isMediaEncrypted: Bool = false
	@Published var isNotEncrypted: Bool = false
	@Published var isZrtp: Bool = false
	@Published var isRemoteDeviceTrusted: Bool = false
	@Published var cacheMismatch: Bool = false
	@Published var isNotVerified: Bool = false
	@Published var selectedCall: Call?
	@Published var isTransferInsteadCall: Bool = false
	@Published var isOneOneCall: Bool = false
	@Published var isConference: Bool = false
	@Published var videoDisplayed: Bool = false
	@Published var chatEnabled: Bool = false
	@Published var participantList: [ParticipantModel] = []
	@Published var activeSpeakerParticipant: ParticipantModel?
	@Published var activeSpeakerName: String = ""
	@Published var myParticipantModel: ParticipantModel?
	@Published var callMediaEncryptionModel = CallMediaEncryptionModel()
	@Published var callStatsModel = CallStatsModel()
	
	@Published var qualityValue: Float = 0.0
	@Published var qualityIcon = "cell-signal-full"

	private var conferenceDelegate: ConferenceDelegate?
	private var waitingForConferenceDelegate: ConferenceDelegate?
	
	@Published var calls: [Call] = []
	@Published var callsCounter: Int = 0
	@Published var callsContactAvatarModel: [ContactAvatarModel?] = []
	
	let timer = Timer.publish(every: 1, on: .main, in: .common).autoconnect()
	
	var currentCall: Call?
	
	private var callDelegate: CallDelegate?
	
	@Published var letters1: String = "AA"
	@Published var letters2: String = "BB"
	@Published var letters3: String = "CC"
	@Published var letters4: String = "DD"
	
	@Published var operationInProgress: Bool = false
	
	private var mCoreDelegate: CoreDelegate?
	private var chatRoomDelegate: ChatRoomDelegate?
	
	init() {
		do {
			try AVAudioSession.sharedInstance().setCategory(.playAndRecord, mode: .voiceChat, options: .allowBluetooth)
		} catch _ {
			
		}
		NotificationCenter.default.addObserver(forName: Notification.Name("CallViewModelReset"), object: nil, queue: nil) { notification in
			self.resetCallView()
		}
	}
	
	func resetCallView() {
		DispatchQueue.main.async {
			self.displayName = ""
			self.avatarModel = nil
		}
		
		coreContext.doOnCoreQueue { core in
            if (core.currentCall != nil && core.currentCall!.remoteAddress != nil) || (core.calls.first != nil && core.calls.first!.state == .Paused) {
                let currentCallTmp = core.currentCall ?? core.calls.first
                
				if self.callDelegate != nil {
					self.currentCall?.removeDelegate(delegate: self.callDelegate!)
					self.callDelegate = nil
				}
				if self.conferenceDelegate != nil {
					self.currentCall?.conference?.removeDelegate(delegate: self.conferenceDelegate!)
					self.conferenceDelegate = nil
				}
				if self.waitingForConferenceDelegate != nil {
					self.currentCall?.conference?.removeDelegate(delegate: self.waitingForConferenceDelegate!)
					self.waitingForConferenceDelegate = nil
				}
				self.currentCall = currentCallTmp
				let callsCounterTmp = core.calls.count
				
				var videoDisplayedTmp = false
				do {
					let params = try core.createCallParams(call: self.currentCall)
					videoDisplayedTmp = params.videoEnabled && params.videoDirection == .SendRecv || params.videoDirection == .SendOnly
				} catch {
					
				}
				
				var displayNameTmp = ""
				
				var isOneOneCallTmp = false
				if self.currentCall?.remoteAddress != nil {
					let conf = self.currentCall!.conference
					let confInfo = core.findConferenceInformationFromUri(uri: self.currentCall!.remoteAddress!)
					if conf == nil && confInfo == nil {
						isOneOneCallTmp = true
					} else {
						displayNameTmp = confInfo?.subject ?? "Conference-focus"
					}
				}
				
				var isMediaEncryptedTmp = false
				var isZrtpTmp = false
				if self.currentCall != nil && self.currentCall!.currentParams != nil {
					if self.currentCall!.currentParams!.mediaEncryption == .ZRTP ||
						self.currentCall!.currentParams!.mediaEncryption == .SRTP ||
						self.currentCall!.currentParams!.mediaEncryption == .DTLS {
						
						isMediaEncryptedTmp = true
						isZrtpTmp = self.currentCall!.currentParams!.mediaEncryption == .ZRTP
					}
				}
				
				let directionTmp = self.currentCall!.dir
				
				let remoteAddressTmp = self.currentCall!.remoteAddress!.clone()
				
				let remoteAddressStringTmp = remoteAddressTmp != nil ? String(remoteAddressTmp!.asStringUriOnly().dropFirst(4)) : ""
				
				remoteAddressTmp!.clean()
				
				let remoteAddressCleanedStringTmp = remoteAddressTmp != nil ? String(remoteAddressTmp!.asStringUriOnly().dropFirst(4)) : ""
				
				if self.currentCall?.conference != nil {
					displayNameTmp = self.currentCall?.conference?.subject ?? ""
				} else if self.currentCall?.remoteAddress != nil {
					let friend = ContactsManager.shared.getFriendWithAddress(address: self.currentCall!.remoteAddress)
					if friend != nil && friend!.address != nil && friend!.address!.displayName != nil {
						displayNameTmp = friend!.address!.displayName!
					} else {
						if self.currentCall!.remoteAddress!.displayName != nil {
							displayNameTmp = self.currentCall!.remoteAddress!.displayName!
						} else if self.currentCall!.remoteAddress!.username != nil && displayNameTmp.isEmpty {
							displayNameTmp = self.currentCall!.remoteAddress!.username!
						} else if displayNameTmp.isEmpty {
							displayNameTmp = String(self.currentCall!.remoteAddress!.asStringUriOnly().dropFirst(4))
						}
					}
					
					DispatchQueue.main.async {
						self.displayName = displayNameTmp
					}
					
					ContactAvatarModel.getAvatarModelFromAddress(address: self.currentCall!.remoteAddress!) { avatarResult in
						DispatchQueue.main.async {
							self.avatarModel = avatarResult
						}
					}
				}
				
				let micMuttedTmp = self.currentCall!.microphoneMuted || !core.micEnabled
				let isRecordingTmp = self.currentCall!.params!.isRecording
				let isPausedTmp = self.isCallPaused()
				let timeElapsedTmp = self.currentCall?.duration ?? 0
				
				let authToken = self.currentCall!.localAuthenticationToken
				let cacheMismatchFlag = self.currentCall!.zrtpCacheMismatchFlag
				let isDeviceTrusted = !cacheMismatchFlag && self.currentCall!.authenticationTokenVerified && authToken != nil
				let isRemoteDeviceTrustedTmp = self.telecomManager.callInProgress ? isDeviceTrusted : false
				
				if self.currentCall != nil {
					self.callMediaEncryptionModel.update(call: self.currentCall!)
					if self.currentCall!.audioStats != nil {
						self.callStatsModel.update(call: self.currentCall!, stats: self.currentCall!.audioStats!)
					}
				}
				
				var chatEnabledTmp = false
				if self.currentCall?.conference != nil {
					chatEnabledTmp = self.currentCall?.conference?.currentParams?.chatEnabled ?? false
				} else {
					chatEnabledTmp = true
				}
				
				DispatchQueue.main.async {
					self.direction = directionTmp
					self.remoteAddressString = remoteAddressStringTmp
					self.remoteAddressCleanedString = remoteAddressCleanedStringTmp
					self.remoteAddress = remoteAddressTmp
					self.displayName = displayNameTmp
					
					self.micMutted = micMuttedTmp
					self.isRecording = isRecordingTmp
					self.isPaused = isPausedTmp
					self.timeElapsed = timeElapsedTmp
					
					self.isRemoteDeviceTrusted = isRemoteDeviceTrustedTmp
					self.activeSpeakerParticipant = nil
					
					self.isRemoteRecording = false
					self.zrtpPopupDisplayed = false
					self.upperCaseAuthTokenToRead = ""
					self.upperCaseAuthTokenToListen = ""
					self.isNotVerified = false
					
					self.updateEncryption(withToast: false)
					self.isConference = false
					self.participantList = []
					self.activeSpeakerParticipant = nil
					self.activeSpeakerName = ""
					self.myParticipantModel = nil
					
					self.videoDisplayed = videoDisplayedTmp
					self.isOneOneCall = isOneOneCallTmp
					self.isMediaEncrypted = isMediaEncryptedTmp
					self.isNotEncrypted = false
					self.isZrtp = isZrtpTmp
					self.cacheMismatch = cacheMismatchFlag
					self.chatEnabled = chatEnabledTmp
					
					self.getCallsList()
					
					self.callsCounter = callsCounterTmp
					
					if self.currentCall?.conference?.state == .Created {
						self.getConference()
					} else {
						self.waitingForCreatedStateConference()
					}
				}
				
				self.callDelegate = CallDelegateStub(onEncryptionChanged: { (_: Call, _: Bool, _: String)in
                    DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
                        self.updateEncryption(withToast: false)
                    }
					if self.currentCall != nil {
						self.callMediaEncryptionModel.update(call: self.currentCall!)
					}
				}, onAuthenticationTokenVerified: { (_, verified: Bool) in
					Log.warn("[CallViewModel][ZRTPPopup] Notified that authentication token is \(verified ? "verified" : "not verified!")")
					if verified {
                        DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
                            self.updateEncryption(withToast: true)
                            if self.currentCall != nil {
                                self.callMediaEncryptionModel.update(call: self.currentCall!)
                            }
                        }
					} else {
                        DispatchQueue.main.async {
                            self.isNotVerified = true
                            self.zrtpPopupDisplayed = true
                        }
					}
				}, onStatsUpdated: { (_: Call, stats: CallStats) in
					DispatchQueue.main.async {
						if self.currentCall != nil {
							self.callStatsModel.update(call: self.currentCall!, stats: stats)
						}
					}
				})
				self.currentCall!.addDelegate(delegate: self.callDelegate!)
				self.updateCallQualityIcon()
			}
		}
	}
	
	func getCallsList() {
		self.callsContactAvatarModel.removeAll()
		self.calls.removeAll()
		coreContext.doOnCoreQueue { core in
			let callsTmp = core.calls
			callsTmp.forEach { call in
				ContactAvatarModel.getAvatarModelFromAddress(address: call.callLog!.remoteAddress!) { avatarResult in
					DispatchQueue.main.async {
						print("getCallsListgetCallsList 00 \(callsTmp.count)")
						self.callsContactAvatarModel.append(avatarResult)
						self.calls.append(call)
					}
				}
			}
		}
	}
	
	func getConference() {
		coreContext.doOnCoreQueue { _ in
			if self.currentCall?.conference != nil {
				let conf = self.currentCall!.conference!
				
				let displayNameTmp = conf.subject ?? ""
				
				var myParticipantModelTmp: ParticipantModel?
				if conf.me?.address != nil {
					myParticipantModelTmp = ParticipantModel(address: conf.me!.address!, isJoining: false, onPause: false, isMuted: false, isAdmin: conf.me!.isAdmin)
				} else if self.currentCall?.callLog?.localAddress != nil {
					myParticipantModelTmp = ParticipantModel(address: self.currentCall!.callLog!.localAddress!, isJoining: false, onPause: false, isMuted: false, isAdmin: conf.me!.isAdmin)
				}
				
				var activeSpeakerParticipantTmp: ParticipantModel?
				if conf.activeSpeakerParticipantDevice?.address != nil {
					activeSpeakerParticipantTmp = ParticipantModel(
						address: conf.activeSpeakerParticipantDevice!.address!,
						isJoining: conf.activeSpeakerParticipantDevice!.state == .Joining || conf.activeSpeakerParticipantDevice!.state == .Alerting,
						onPause: conf.activeSpeakerParticipantDevice!.state == .OnHold,
						isMuted: conf.activeSpeakerParticipantDevice!.isMuted
					)
				} else if conf.participantList.first?.address != nil && conf.participantList.first!.address!.clone()!.equal(address2: (conf.me?.address)!) {
					activeSpeakerParticipantTmp = ParticipantModel(
						address: conf.participantDeviceList.first!.address!,
						isJoining: conf.participantDeviceList.first!.state == .Joining || conf.participantDeviceList.first!.state == .Alerting,
						onPause: conf.participantDeviceList.first!.state == .OnHold,
						isMuted: conf.participantDeviceList.first!.isMuted
					)
				} else if conf.participantList.last?.address != nil {
					activeSpeakerParticipantTmp = ParticipantModel(
						address: conf.participantDeviceList.last!.address!,
						isJoining: conf.participantDeviceList.last!.state == .Joining || conf.participantDeviceList.last!.state == .Alerting,
						onPause: conf.participantDeviceList.last!.state == .OnHold,
						isMuted: conf.participantDeviceList.last!.isMuted
					)
				}
				
				var activeSpeakerNameTmp = ""
				if activeSpeakerParticipantTmp != nil {
					let friend = ContactsManager.shared.getFriendWithAddress(address: activeSpeakerParticipantTmp?.address)
					if friend != nil && friend!.address != nil && friend!.address!.displayName != nil {
						activeSpeakerNameTmp = friend!.address!.displayName!
					} else {
						if activeSpeakerParticipantTmp!.address.displayName != nil {
							activeSpeakerNameTmp = activeSpeakerParticipantTmp!.address.displayName!
						} else if activeSpeakerParticipantTmp!.address.username != nil {
							activeSpeakerNameTmp = activeSpeakerParticipantTmp!.address.username!
						} else {
							activeSpeakerNameTmp = String(activeSpeakerParticipantTmp!.address.asStringUriOnly().dropFirst(4))
						}
					}
				}
				
				var participantListTmp: [ParticipantModel] = []
				conf.participantDeviceList.forEach({ participantDevice in
					if participantDevice.address != nil && !conf.isMe(uri: participantDevice.address!.clone()!) {
						if !conf.isMe(uri: participantDevice.address!.clone()!) {
							let isAdmin = conf.participantList.first(where: {$0.address!.equal(address2: participantDevice.address!.clone()!)})?.isAdmin
							participantListTmp.append(
								ParticipantModel(
									address: participantDevice.address!,
									isJoining: participantDevice.state == .Joining || participantDevice.state == .Alerting,
									onPause: participantDevice.state == .OnHold,
									isMuted: participantDevice.isMuted,
									isAdmin: isAdmin ?? false
								)
							)
						}
					}
				})
				
				DispatchQueue.main.async {
					self.displayName = displayNameTmp
					
					self.isConference = true
					
					self.myParticipantModel = myParticipantModelTmp
					
					self.activeSpeakerParticipant = activeSpeakerParticipantTmp
					
					self.activeSpeakerName = activeSpeakerNameTmp
					
					self.participantList = participantListTmp
					
					self.addConferenceCallBacks()
				}
			} else if self.currentCall?.remoteContactAddress != nil {
				self.addConferenceCallBacks()
			}
		}
	}
	
	func waitingForCreatedStateConference() {
		if let conference = self.currentCall?.conference {
			self.waitingForConferenceDelegate = ConferenceDelegateStub(onStateChanged: { (_: Conference, newState: Conference.State) in
				if newState == .Created {
					DispatchQueue.main.async {
						self.getConference()
					}
				}
			})
			conference.addDelegate(delegate: self.waitingForConferenceDelegate!)
		}
	}
	
	func addConferenceCallBacks() {
		coreContext.doOnCoreQueue { _ in
			guard let conference = self.currentCall?.conference else { return }
			
			self.conferenceDelegate = ConferenceDelegateStub(onParticipantDeviceAdded: { (conference: Conference, participantDevice: ParticipantDevice) in
				if participantDevice.address != nil {
					var participantListTmp: [ParticipantModel] = []
					conference.participantDeviceList.forEach({ pDevice in
						if pDevice.address != nil && !conference.isMe(uri: pDevice.address!.clone()!) {
							if !conference.isMe(uri: pDevice.address!.clone()!) {
								let isAdmin = conference.participantList.first(where: {$0.address!.equal(address2: pDevice.address!.clone()!)})?.isAdmin
								participantListTmp.append(
									ParticipantModel(
										address: pDevice.address!,
										isJoining: pDevice.state == .Joining || pDevice.state == .Alerting,
										onPause: pDevice.state == .OnHold,
										isMuted: pDevice.isMuted,
										isAdmin: isAdmin ?? false
									)
								)
							}
						}
					})
					
					var activeSpeakerParticipantTmp: ParticipantModel?
					var activeSpeakerNameTmp = ""
					
					if self.activeSpeakerParticipant == nil {
						if conference.activeSpeakerParticipantDevice?.address != nil {
							activeSpeakerParticipantTmp = ParticipantModel(
								address: conference.activeSpeakerParticipantDevice!.address!,
								isJoining: conference.activeSpeakerParticipantDevice!.state == .Joining || conference.activeSpeakerParticipantDevice!.state == .Alerting,
								onPause: conference.activeSpeakerParticipantDevice!.state == .OnHold,
								isMuted: conference.activeSpeakerParticipantDevice!.isMuted
							)
						} else if conference.participantList.first?.address != nil && conference.participantList.first!.address!.clone()!.equal(address2: (conference.me?.address)!) {
							activeSpeakerParticipantTmp = ParticipantModel(
								address: conference.participantDeviceList.first!.address!,
								isJoining: conference.participantDeviceList.first!.state == .Joining || conference.participantDeviceList.first!.state == .Alerting,
								onPause: conference.participantDeviceList.first!.state == .OnHold,
								isMuted: conference.participantDeviceList.first!.isMuted
							)
						} else if conference.participantList.last?.address != nil {
							activeSpeakerParticipantTmp = ParticipantModel(
								address: conference.participantDeviceList.last!.address!,
								isJoining: conference.participantDeviceList.last!.state == .Joining || conference.participantDeviceList.last!.state == .Alerting,
								onPause: conference.participantDeviceList.last!.state == .OnHold,
								isMuted: conference.participantDeviceList.last!.isMuted
							)
						}
						
						if activeSpeakerParticipantTmp != nil {
							let friend = ContactsManager.shared.getFriendWithAddress(address: activeSpeakerParticipantTmp?.address)
							if friend != nil && friend!.address != nil && friend!.address!.displayName != nil {
								activeSpeakerNameTmp = friend!.address!.displayName!
							} else {
								if activeSpeakerParticipantTmp!.address.displayName != nil {
									activeSpeakerNameTmp = activeSpeakerParticipantTmp!.address.displayName!
								} else if activeSpeakerParticipantTmp!.address.username != nil {
									activeSpeakerNameTmp = activeSpeakerParticipantTmp!.address.username!
								} else {
									activeSpeakerNameTmp = String(activeSpeakerParticipantTmp!.address.asStringUriOnly().dropFirst(4))
						  		}
							}
							DispatchQueue.main.async {
								if self.activeSpeakerParticipant == nil {
									self.activeSpeakerName = activeSpeakerNameTmp
								}
							}
						}
					}
					
					DispatchQueue.main.async {
						if self.activeSpeakerParticipant == nil {
							self.activeSpeakerParticipant = activeSpeakerParticipantTmp
							self.activeSpeakerName = activeSpeakerNameTmp
						}
						self.participantList = participantListTmp
					}
				}
			}, onParticipantDeviceRemoved: { (conference: Conference, participantDevice: ParticipantDevice) in
				if participantDevice.address != nil {
					var participantListTmp: [ParticipantModel] = []
					conference.participantDeviceList.forEach({ pDevice in
						if pDevice.address != nil && !conference.isMe(uri: pDevice.address!.clone()!) {
							if !conference.isMe(uri: pDevice.address!.clone()!) {
								let isAdmin = conference.participantList.first(where: {$0.address!.equal(address2: pDevice.address!.clone()!)})?.isAdmin
								participantListTmp.append(
									ParticipantModel(
										address: pDevice.address!,
										isJoining: pDevice.state == .Joining || pDevice.state == .Alerting,
										onPause: pDevice.state == .OnHold,
										isMuted: pDevice.isMuted,
										isAdmin: isAdmin ?? false
									)
								)
							}
						}
					})
					
					let participantDeviceListCount = conference.participantDeviceList.count
					
					DispatchQueue.main.async {
						self.participantList = participantListTmp
						
						if participantDeviceListCount == 1 {
							self.activeSpeakerParticipant = nil
						}
					}
				}
			}, onParticipantAdminStatusChanged: { (_: Conference, participant: Participant) in
				let isAdmin = participant.isAdmin
				if self.myParticipantModel != nil && self.myParticipantModel!.address.clone()!.equal(address2: participant.address!) {
					DispatchQueue.main.async {
						self.myParticipantModel!.isAdmin = isAdmin
					}
				}
				self.participantList.forEach({ participantDevice in
					if participantDevice.address.clone()!.equal(address2: participant.address!) {
						DispatchQueue.main.async {
							participantDevice.isAdmin = isAdmin
						}
					}
				})
			}, onParticipantDeviceStateChanged: { (_: Conference, device: ParticipantDevice, state: ParticipantDevice.State) in
				Log.info(
					"[CallViewModel] Participant device \(device.address!.asStringUriOnly()) state changed \(state)"
				)
				if self.activeSpeakerParticipant != nil && self.activeSpeakerParticipant!.address.equal(address2: device.address!) {
					DispatchQueue.main.async {
						self.activeSpeakerParticipant!.onPause = state == .OnHold
						self.activeSpeakerParticipant!.isJoining = state == .Joining || state == .Alerting
					}
				}
				self.participantList.forEach({ participantDevice in
					if participantDevice.address.equal(address2: device.address!) {
						DispatchQueue.main.async {
							participantDevice.onPause = state == .OnHold
							participantDevice.isJoining = state == .Joining || state == .Alerting
						}
					}
				})
			}, onParticipantDeviceIsSpeakingChanged: { (_: Conference, device: ParticipantDevice, isSpeaking: Bool) in
				let isSpeaking = device.isSpeaking
				if self.myParticipantModel != nil && self.myParticipantModel!.address.clone()!.equal(address2: device.address!) {
					DispatchQueue.main.async {
						self.myParticipantModel!.isSpeaking = isSpeaking
					}
				}
				self.participantList.forEach({ participantDeviceList in
					if participantDeviceList.address.clone()!.equal(address2: device.address!) {
						DispatchQueue.main.async {
							participantDeviceList.isSpeaking = isSpeaking
						}
					}
				})
			}, onParticipantDeviceIsMuted: { (_: Conference, device: ParticipantDevice, isMuted: Bool) in
				if self.activeSpeakerParticipant != nil && self.activeSpeakerParticipant!.address.equal(address2: device.address!) {
					DispatchQueue.main.async {
						self.activeSpeakerParticipant!.isMuted = isMuted
					}
				}
				self.participantList.forEach({ participantDevice in
					if participantDevice.address.equal(address2: device.address!) {
						DispatchQueue.main.async {
							participantDevice.isMuted = isMuted
						}
					}
				})
			}, onActiveSpeakerParticipantDevice: { (conference: Conference, participantDevice: ParticipantDevice?) in
				if participantDevice != nil && participantDevice!.address != nil {
					let activeSpeakerParticipantBis = self.activeSpeakerParticipant
					
					let activeSpeakerParticipantTmp = ParticipantModel(
						address: participantDevice!.address!,
						isJoining: participantDevice!.state == .Joining || participantDevice!.state == .Alerting,
						onPause: participantDevice!.state == .OnHold,
						isMuted: participantDevice!.isMuted
					)
					
					var activeSpeakerNameTmp = ""
					let friend = ContactsManager.shared.getFriendWithAddress(address: activeSpeakerParticipantTmp.address)
					if friend != nil && friend!.address != nil && friend!.address!.displayName != nil {
						activeSpeakerNameTmp = friend!.address!.displayName!
					} else {
						if activeSpeakerParticipantTmp.address.displayName != nil {
							activeSpeakerNameTmp = activeSpeakerParticipantTmp.address.displayName!
						} else if activeSpeakerParticipantTmp.address.username != nil {
							activeSpeakerNameTmp = activeSpeakerParticipantTmp.address.username!
						} else {
							activeSpeakerNameTmp = String(activeSpeakerParticipantTmp.address.asStringUriOnly().dropFirst(4))
						}
					}
					
					var participantListTmp: [ParticipantModel] = []
					if (activeSpeakerParticipantBis != nil && !activeSpeakerParticipantBis!.address.equal(address2: activeSpeakerParticipantTmp.address))
						|| ( activeSpeakerParticipantBis == nil) {
						
						conference.participantDeviceList.forEach({ pDevice in
							if pDevice.address != nil && !conference.isMe(uri: pDevice.address!.clone()!) {
								if !conference.isMe(uri: pDevice.address!.clone()!) {
									let isAdmin = conference.participantList.first(where: {$0.address!.equal(address2: pDevice.address!.clone()!)})?.isAdmin
									participantListTmp.append(
										ParticipantModel(
											address: pDevice.address!,
											isJoining: pDevice.state == .Joining || pDevice.state == .Alerting,
											onPause: pDevice.state == .OnHold,
											isMuted: pDevice.isMuted,
											isAdmin: isAdmin ?? false
										)
									)
								}
							}
						})
					}
					
					DispatchQueue.main.async {
						self.activeSpeakerParticipant = activeSpeakerParticipantTmp
						self.activeSpeakerName = activeSpeakerNameTmp
						if (activeSpeakerParticipantBis != nil && !activeSpeakerParticipantBis!.address.equal(address2: activeSpeakerParticipantTmp.address))
							|| ( activeSpeakerParticipantBis == nil) {
							self.participantList = participantListTmp
						}
					}
				}
			})
			conference.addDelegate(delegate: self.conferenceDelegate!)
		}
	}
	
	func terminateCall() {
		coreContext.doOnCoreQueue { core in
			if self.currentCall != nil {
				self.telecomManager.terminateCall(call: self.currentCall!)
			}
			
			if core.callsNb == 0 {
				DispatchQueue.main.async {
					self.timer.upstream.connect().cancel()
					self.currentCall = nil
				}
			}
		}
	}
	
	func acceptCall() {
		withAnimation {
			telecomManager.outgoingCallStarted = false
			telecomManager.callInProgress = true
			telecomManager.callDisplayed = true
			telecomManager.callStarted = true
		}
		
		coreContext.doOnCoreQueue { core in
			if self.currentCall != nil {
				self.telecomManager.acceptCall(core: core, call: self.currentCall!, hasVideo: false)
			}
		}
		
		timer.upstream.connect().cancel()
	}
	
	func toggleMuteMicrophone() {
		coreContext.doOnCoreQueue { core in
			if self.currentCall != nil {
				if !core.micEnabled && !self.currentCall!.microphoneMuted {
					core.micEnabled = true
				} else {
					self.currentCall!.microphoneMuted = !self.currentCall!.microphoneMuted
				}
				
				let micMuttedTmp = self.currentCall!.microphoneMuted || !core.micEnabled
				DispatchQueue.main.async {
					self.micMutted = micMuttedTmp
				}
				
				Log.info(
					"[CallViewModel] Microphone mute switch \(self.micMutted)"
				)
			}
		}
	}
	
	func displayMyVideo() {
		coreContext.doOnCoreQueue { core in
			guard let call = self.currentCall else { return }
			
			let invalidStates: [Call.State] = [.Released, .End, .Error, .Idle]

			guard !invalidStates.contains(call.state) else {
				Log.warn("\(CallViewModel.TAG) displayMyVideo called in invalid state: \(call.state), skipping update")
				return
			}
			
			do {
				let params = try core.createCallParams(call: call)
				
				if !params.videoEnabled {
					Log.info("\(CallViewModel.TAG) Video disabled in params, enabling it")
					params.videoEnabled = true
					params.videoDirection = .SendRecv
				} else {
					if params.videoDirection == .SendRecv || params.videoDirection == .SendOnly {
						Log.info("\(CallViewModel.TAG) Video already enabled, switching to recv only")
						params.videoDirection = .RecvOnly
					} else {
						Log.info("\(CallViewModel.TAG) Video already enabled, switching to send & recv")
						params.videoDirection = .SendRecv
					}
				}
				
				try call.update(params: params)
				
				let video = params.videoDirection == .SendRecv || params.videoDirection == .SendOnly
				DispatchQueue.main.asyncAfter(deadline: .now() + (video ? 1 : 0)) {
					if video {
						self.videoDisplayed = false
					}
					self.videoDisplayed = video
				}
			} catch {
				Log.error("\(CallViewModel.TAG) Failed to update video params: \(error)")
			}
		}
	}

	
	func toggleVideoMode(isAudioOnlyMode: Bool) {
		coreContext.doOnCoreQueue { core in
			if self.currentCall != nil {
				do {
					let params = try core.createCallParams(call: self.currentCall)
					
					params.videoEnabled = !isAudioOnlyMode
					
					try self.currentCall!.update(params: params)
				} catch {
					
				}
			}
		}
	}
	
	func switchCamera() {
		coreContext.doOnCoreQueue { core in
			let currentDevice = core.videoDevice
			Log.info("[CallViewModel] Current camera device is \(currentDevice ?? "nil")")
			
			core.videoDevicesList.forEach { camera in
				if camera != currentDevice && camera != "StaticImage: Static picture" {
					Log.info("[CallViewModel] New camera device will be \(camera)")
					do {
						try core.setVideodevice(newValue: camera)
					} catch _ {
						
					}
				}
			}
		}
	}
	
	func toggleRecording() {
		coreContext.doOnCoreQueue { _ in
			if self.currentCall != nil && self.currentCall!.params != nil {
				if self.currentCall!.params!.isRecording {
					Log.info("[CallViewModel] Stopping call recording")
					self.currentCall!.stopRecording()
				} else {
					Log.info("[CallViewModel] Starting call recording \(self.currentCall!.params!.isRecording)")
					self.currentCall!.startRecording()
				}
				
				let isRecordingTmp = self.currentCall!.params!.isRecording
				DispatchQueue.main.async {
					self.isRecording = isRecordingTmp
				}
			}
		}
	}
	
	func togglePause() {
		coreContext.doOnCoreQueue { _ in
			if self.currentCall != nil && self.currentCall!.remoteAddress != nil {
				do {
					if self.isCallPaused() {
						Log.info("[CallViewModel] Resuming call \(self.currentCall!.remoteAddress!.asStringUriOnly())")
						try self.currentCall!.resume()
						
						DispatchQueue.main.async {
							self.isPaused = false
						}
					} else {
						Log.info("[CallViewModel] Pausing call \(self.currentCall!.remoteAddress!.asStringUriOnly())")
						try self.currentCall!.pause()
						
						DispatchQueue.main.async {
							self.isPaused = true
						}
					}
				} catch _ {
					
				}
			}
		}
	}
	
	func isCallPaused() -> Bool {
		var result = false
		if self.currentCall != nil {
			switch self.currentCall!.state {
			case Call.State.Paused, Call.State.Pausing:
				result = true
			default:
				result = false
			}
		}
		return result
	}
	
	func counterToMinutes() -> String {
		let currentTime = timeElapsed
		let seconds = currentTime % 60
		let minutes = String(format: "%02d", Int(currentTime / 60))
		let hours = String(format: "%02d", Int(currentTime / 3600))
		
		if Int(currentTime / 3600) > 0 {
			return "\(hours):\(minutes):\(seconds < 10 ? "0" : "")\(seconds)"
		} else {
			return "\(minutes):\(seconds < 10 ? "0" : "")\(seconds)"
		}
	}
	
	func isHeadPhoneAvailable() -> Bool {
		guard let availableInputs = AVAudioSession.sharedInstance().availableInputs else {return false}
		for inputDevice in availableInputs {
			if inputDevice.portType == .headsetMic  || inputDevice.portType == .headphones {
				return true
			}
		}
		return false
	}
	
	func orientationUpdate(orientation: UIDeviceOrientation) {
		coreContext.doOnCoreQueue { core in
			let oldLinphoneOrientation = core.deviceRotation
			var newRotation = 0
			switch orientation {
			case .portrait:
				newRotation = 0
			case .portraitUpsideDown:
				newRotation = 180
			case .landscapeRight:
				newRotation = 90
			case .landscapeLeft:
				newRotation = 270
			default:
				newRotation = oldLinphoneOrientation
			}
			
			if oldLinphoneOrientation != newRotation {
				core.deviceRotation = newRotation
			}
		}
	}
	
	func skipZrtpAuthentication() {
		Log.info(
			"[ZRTPPopup] User skipped SAS validation in ZRTP call"
		)
		
		coreContext.doOnCoreQueue { core in
			if core.currentCall != nil {
				core.currentCall!.skipZrtpAuthentication()
			}
		}
	}
	
	func updateZrtpSas(authTokenClicked: String) {
		coreContext.doOnCoreQueue { core in
			if core.currentCall != nil {
				if authTokenClicked.isEmpty {
					Log.error(
						"[ZRTPPopup] Doing a fake ZRTP SAS check with empty token because user clicked on 'Not Found' button!"
					)
				} else {
					Log.info(
						"[ZRTPPopup] Checking if ZRTP SAS auth token \(authTokenClicked) is the right one"
					)
				}
				core.currentCall!.checkAuthenticationTokenSelected(selectedValue: authTokenClicked)
			}
		}
	}
	
	func remoteAuthenticationTokens() {
		coreContext.doOnCoreQueue { core in
			if core.currentCall != nil {
				let tokens = core.currentCall!.remoteAuthenticationTokens
				if !tokens.isEmpty {
					DispatchQueue.main.async {
						self.letters1 = tokens[0]
						self.letters2 = tokens[1]
						self.letters3 = tokens[2]
						self.letters4 = tokens[3]
					}
				}
			}
		}
	}
	
	private func updateEncryption(withToast: Bool) {
		coreContext.doOnCoreQueue { _ in
			if self.currentCall != nil && self.currentCall!.currentParams != nil {
				switch self.currentCall!.currentParams!.mediaEncryption {
				case MediaEncryption.ZRTP:
					let authToken = self.currentCall!.localAuthenticationToken
					let isDeviceTrusted = self.currentCall!.authenticationTokenVerified && authToken != nil
					
					Log.info(
						"[CallViewModel] Current call media encryption is ZRTP, auth token is \(isDeviceTrusted ? "trusted" : "not trusted yet")"
					)
					
					let cacheMismatchFlag = self.currentCall!.zrtpCacheMismatchFlag
					let isRemoteDeviceTrustedTmp = !cacheMismatchFlag && isDeviceTrusted
					
					/*
					 let securityLevel = isDeviceTrusted ? SecurityLevel.Safe : SecurityLevel.Encrypted
					 let avatarModel = contact
					 if (avatarModel != nil) {
					 avatarModel.trust.postValue(securityLevel)
					 contact.postValue(avatarModel!!)
					 } else {
					 Log.error("$TAG No avatar model found!")
					 }
					 */
					
					DispatchQueue.main.async {
						self.isRemoteDeviceTrusted = isRemoteDeviceTrustedTmp
						self.isMediaEncrypted = true
						self.isZrtp = true
						self.cacheMismatch = cacheMismatchFlag
						self.isNotEncrypted = false
						
						if isDeviceTrusted && withToast {
							ToastViewModel.shared.toastMessage = "Info_call_securised"
							ToastViewModel.shared.displayToast = true
						}
					}
					
					if !isDeviceTrusted && authToken != nil && !authToken!.isEmpty {
						Log.info("[CallViewModel] Showing ZRTP SAS confirmation dialog")
						self.showZrtpSasDialog(authToken: authToken!)
					}
				case MediaEncryption.SRTP, MediaEncryption.DTLS:
					DispatchQueue.main.async {
						self.isMediaEncrypted = true
						self.isZrtp = false
						self.isNotEncrypted = false
					}
				case MediaEncryption.None:
					let isNotEncryptedTmp = self.currentCall?.state == .StreamsRunning
					DispatchQueue.main.async {
						self.isMediaEncrypted = false
						self.isZrtp = false
						self.isNotEncrypted = isNotEncryptedTmp
					}
				}
			}
		}
	}
	
	func showZrtpSasDialogIfPossible() {
		if currentCall != nil && currentCall!.currentParams != nil && currentCall!.currentParams!.mediaEncryption == MediaEncryption.ZRTP {
			let authToken = currentCall!.localAuthenticationToken
			let isDeviceTrusted = currentCall!.authenticationTokenVerified && authToken != nil
			Log.info(
				"[CallViewModel] Current call media encryption is ZRTP, auth token is \(isDeviceTrusted ? "trusted" : "not trusted yet")"
			)
			if authToken != nil && !authToken!.isEmpty {
				showZrtpSasDialog(authToken: authToken!)
			}
		}
	}
	
	private func showZrtpSasDialog(authToken: String) {
		if self.currentCall != nil {
			let upperCaseAuthToken = authToken.localizedUppercase
			
			let mySubstringPrefix = upperCaseAuthToken.prefix(2)
			
			let mySubstringSuffix = upperCaseAuthToken.suffix(2)
			
			DispatchQueue.main.async {
				switch self.currentCall!.dir {
				case Call.Dir.Incoming:
					self.upperCaseAuthTokenToRead = String(mySubstringPrefix)
					self.upperCaseAuthTokenToListen = String(mySubstringSuffix)
				default:
					self.upperCaseAuthTokenToRead = String(mySubstringSuffix)
					self.upperCaseAuthTokenToListen = String(mySubstringPrefix)
				}
				
				self.zrtpPopupDisplayed = true
			}
		}
	}
	
	func transferClicked() {
		coreContext.doOnCoreQueue { core in
			let callToTransferTo = core.calls.last { call in
				call.state == Call.State.Paused && call.callLog?.callId != self.currentCall?.callLog?.callId
			}
			
			if callToTransferTo == nil {
				Log.error(
					"[CallViewModel] Couldn't find a call in Paused state to transfer current call to"
				)
			} else {
				if self.currentCall != nil && self.currentCall!.remoteAddress != nil && callToTransferTo!.remoteAddress != nil {
					Log.info(
						"[CallViewModel] Doing an attended transfer between currently displayed call \(self.currentCall!.remoteAddress!.asStringUriOnly()) "
						+ "and paused call \(callToTransferTo!.remoteAddress!.asStringUriOnly())"
					)
					
					do {
						try callToTransferTo!.transferToAnother(dest: self.currentCall!)
						Log.info("[CallViewModel] Attended transfer is successful")
					} catch _ {
						ToastViewModel.shared.toastMessage = "Failed_toast_call_transfer_failed"
						ToastViewModel.shared.displayToast = true
						
						Log.error("[CallViewModel] Failed to make attended transfer!")
					}
				}
			}
		}
	}
	
	func blindTransferCallTo(toAddress: Address) {
		if self.currentCall != nil && self.currentCall!.remoteAddress != nil {
			Log.info(
				"[CallViewModel] Call \(self.currentCall!.remoteAddress!.asStringUriOnly()) is being blindly transferred to \(toAddress.asStringUriOnly())"
			)
			
			do {
				try self.currentCall!.transferTo(referTo: toAddress)
				Log.info("[CallViewModel] Blind call transfer is successful")
			} catch _ {
				ToastViewModel.shared.toastMessage = "Failed_toast_call_transfer_failed"
				ToastViewModel.shared.displayToast = true
				
				Log.error("[CallViewModel] Failed to make blind call transfer!")
			}
		}
	}
	
	func toggleAdminParticipant(index: Int) {
		coreContext.doOnCoreQueue { _ in
			self.currentCall?.conference?.participantList.forEach({ participant in
				if participant.address != nil && self.participantList[index].address.clone() != nil && participant.address!.equal(address2: self.participantList[index].address.clone()!) {
					self.currentCall?.conference?.setParticipantAdminStatus(participant: participant, isAdmin: !participant.isAdmin)
				}
			})
		}
	}
	
	func removeParticipant(index: Int) {
		coreContext.doOnCoreQueue { _ in
			self.currentCall?.conference?.participantList.forEach({ participant in
				if participant.address != nil && self.participantList[index].address.clone() != nil && participant.address!.equal(address2: self.participantList[index].address.clone()!) {
					do {
						try self.currentCall?.conference?.removeParticipant(participant: participant)
					} catch {
						
					}
				}
			})
		}
	}
	
	func updateCallQualityIcon() {
		DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
			self.coreContext.doOnCoreQueue { core in
				if self.currentCall != nil {
					let quality = self.currentCall!.currentQuality
					let icon = switch floor(quality) {
					case 4, 5: "cell-signal-full"
					case 3: "cell-signal-high"
					case 2: "cell-signal-medium"
					case 1: "cell-signal-low"
					default: "cell-signal-none"
					}
					
					DispatchQueue.main.async {
						self.qualityValue = quality
						self.qualityIcon = icon
					}
					
					if core.callsNb > 0 {
						self.updateCallQualityIcon()
					}
				}
			}
		}
	}
	
	func mergeCallsIntoConference() {
		self.coreContext.doOnCoreQueue { core in
			let callsCount = core.callsNb
			let defaultAccount = core.defaultAccount
			var subject = ""
			
			if defaultAccount != nil && defaultAccount!.params != nil && defaultAccount!.params!.audioVideoConferenceFactoryAddress != nil {
				Log.info("[CallViewModel] Merging \(callsCount) calls into a remotely hosted conference")
				subject = "Remote group call"
			} else {
				Log.info("[CallViewModel] Merging \(callsCount) calls into a locally hosted conference")
				subject = "Local group call"
			}
			do {
				let params = try core.createConferenceParams(conference: nil)
				params.subject = subject
				// Prevent group call to start in audio only layout
				params.videoEnabled = true
				
				let conference = try core.createConferenceWithParams(params: params)
				try conference.addParticipants(calls: core.calls)
			} catch {
				
			}
		}
	}
	
	func addParticipants(participantsToAdd: [SelectedAddressModel]) {
		var list: [SelectedAddressModel] = []
		for selectedAddr in participantsToAdd {
			if let found = list.first(where: { $0.address.weakEqual(address2: selectedAddr.address) }) {
				Log.info("\(CallViewModel.TAG) Participant \(found.address.asStringUriOnly()) already in list, skipping")
				continue
			}
			
			list.append(selectedAddr)
			Log.info("\(CallViewModel.TAG) Added participant \(selectedAddr.address.asStringUriOnly())")
		}
		
		do {
			try self.currentCall!.conference?.addParticipants(addresses: list.map { $0.address })
		} catch {
			
		}
		
		Log.info("\(CallViewModel.TAG) \(list.count) participants added to conference")
	}
	
	func createConversation() {
		if currentCall != nil {
			self.operationInProgress = true
			CoreContext.shared.doOnCoreQueue { _ in
				let existingConversation = self.lookupCurrentCallConversation(call: self.currentCall!)
				if existingConversation != nil {
					Log.info("\(CallViewModel.TAG) Found existing conversation \(LinphoneUtils.getConversationId(chatRoom: existingConversation!)), going to it")
					
					let model = ConversationModel(chatRoom: existingConversation!)
					DispatchQueue.main.async {
						SharedMainViewModel.shared.displayedConversation = model
						self.operationInProgress = false
					}
				} else {
					Log.info("\(CallViewModel.TAG) No existing conversation was found, let's create it")
					self.createCurrentCallConversation(call: self.currentCall!)
				}
			}
		}
	}
	
	func lookupCurrentCallConversation(call: Call) -> ChatRoom? {
		let localAddress = call.callLog?.localAddress
		let remoteAddress = call.remoteAddress
		
		let existingConversation: ChatRoom?
		if call.conference != nil {
			existingConversation = call.conference?.chatRoom
		} else {
			let params = getChatRoomParams(call: call)
			let participants = [remoteAddress!]
			existingConversation = call.core?.searchChatRoom(
				params: params,
				localAddr: localAddress,
				remoteAddr: nil,
				participants: participants
			)
		}
		
		if existingConversation != nil {
			Log.info("\(CallViewModel.TAG) Found existing conversation \(existingConversation!.peerAddress?.asStringUriOnly() ?? "") found for current call with local address \(localAddress?.asStringUriOnly() ?? "") and remote address \(remoteAddress?.asStringUriOnly() ?? "")")
		} else {
			Log.warn("\(CallViewModel.TAG) No existing conversation found for current call with local address \(localAddress?.asStringUriOnly() ?? "") and remote address \(remoteAddress?.asStringUriOnly() ?? "")")
		}
		
		return existingConversation
	}
	
	func createCurrentCallConversation(call: Call) {
		if let remoteAddress = call.remoteAddress {
			let participants = [remoteAddress]
			let core = call.core
			DispatchQueue.main.async {
				self.operationInProgress = true
			}
			
			if let params = getChatRoomParams(call: call) {
				do {
					if core != nil {
						let chatRoom = try core!.createChatRoom(params: params, participants: participants)
						if params.chatParams?.backend == ChatRoom.Backend.FlexisipChat {
							if chatRoom.state == ChatRoom.State.Created {
								let chatRoomId = LinphoneUtils.getConversationId(chatRoom: chatRoom)
								Log.info("\(CallViewModel.TAG) 1-1 conversation \(chatRoomId) has been created")
								let model = ConversationModel(chatRoom: chatRoom)
								DispatchQueue.main.async {
									SharedMainViewModel.shared.displayedConversation = model
									self.operationInProgress = false
								}
							} else {
								Log.info("\(CallViewModel.TAG) Conversation isn't in Created state yet, wait for it")
								self.chatRoomAddDelegate(core: core!, chatRoom: chatRoom)
							}
						} else {
							let id = LinphoneUtils.getConversationId(chatRoom: chatRoom)
							Log.info("\(CallViewModel.TAG) Conversation successfully created \(id)")
							
							let model = ConversationModel(chatRoom: chatRoom)
							DispatchQueue.main.async {
								SharedMainViewModel.shared.displayedConversation = model
								self.operationInProgress = false
							}
							
						}
					}
				} catch {
					Log.error(
						"\(CallViewModel.TAG) Failed to create 1-1 conversation with \(remoteAddress.asStringUriOnly())"
					)
					DispatchQueue.main.async {
						self.operationInProgress = false
						ToastViewModel.shared.toastMessage = "Failed_to_create_conversation_error"
						ToastViewModel.shared.displayToast = true
					}
				}
			}
		}
	}
	
	func getChatRoomParams(call: Call) -> ConferenceParams? {
		let localAddress = call.callLog?.localAddress
		let remoteAddress = call.remoteAddress
		let core = call.core
		if let account = LinphoneUtils.getAccountForAddress(address: localAddress!) ?? LinphoneUtils.getDefaultAccount() {
			do {
				let params = try coreContext.mCore.createConferenceParams(conference: call.conference)
				params.chatEnabled = true
				params.groupEnabled = false
				params.subject = NSLocalizedString("conversation_one_to_one_hidden_subject", comment: "")
				params.account = account
				
				if let chatParams = params.chatParams {
					chatParams.ephemeralLifetime = 0 // Make sure ephemeral is disabled by default
					
					let sameDomain = remoteAddress?.domain == CorePreferences.defaultDomain && remoteAddress?.domain == account.params?.domain
					if account.params != nil && (account.params!.instantMessagingEncryptionMandatory && sameDomain) {
						Log.info(
							"\(CallViewModel.TAG) Account is in secure mode & domain matches, requesting E2E encryption"
						)
						chatParams.backend = ChatRoom.Backend.FlexisipChat
						params.securityLevel = Conference.SecurityLevel.EndToEnd
					} else if account.params != nil && !account.params!.instantMessagingEncryptionMandatory {
						if core != nil && LinphoneUtils.isEndToEndEncryptedChatAvailable(core: core!) {
							Log.info(
								"\(CallViewModel.TAG) Account is in interop mode but LIME is available, requesting E2E encryption"
							)
							chatParams.backend = ChatRoom.Backend.FlexisipChat
							params.securityLevel = Conference.SecurityLevel.EndToEnd
						} else {
							Log.info(
								"\(CallViewModel.TAG) Account is in interop mode but LIME isn't available, disabling E2E encryption"
							)
							chatParams.backend = ChatRoom.Backend.Basic
							params.securityLevel = Conference.SecurityLevel.None
						}
					} else {
						Log.error(
							"\(CallViewModel.TAG) Account is in secure mode, can't chat with SIP address of different domain \(remoteAddress?.asStringUriOnly() ?? "")"
						)
						return nil
					}
					return params
				} else {
					return nil
				}
			} catch {
				Log.error("\(CallViewModel.TAG) Can't create ConferenceParams \(remoteAddress?.asStringUriOnly() ?? "")")
				return nil
			}
		} else {
			return nil
		}
	}
	
	func goToConversation(model: ConversationModel) {
		if 	self.operationInProgress == false {
			DispatchQueue.main.async {
				self.operationInProgress = true
			}
			
			DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
				SharedMainViewModel.shared.displayedConversation = model
				self.operationInProgress = false
			}
		} else {
			DispatchQueue.main.async {
				SharedMainViewModel.shared.displayedConversation = model
				self.operationInProgress = false
			}
		}
	}
	
	func chatRoomAddDelegate(core: Core, chatRoom: ChatRoom) {
		self.chatRoomDelegate = ChatRoomDelegateStub(onStateChanged: { (chatRoom: ChatRoom, state: ChatRoom.State) in
			let state = chatRoom.state
			let id = LinphoneUtils.getChatRoomId(room: chatRoom)
			if state == ChatRoom.State.CreationFailed {
				Log.error("\(StartConversationViewModel.TAG) Conversation \(id) creation has failed!")
				chatRoom.removeDelegate(delegate: self.chatRoomDelegate!)
				self.chatRoomDelegate = nil
				DispatchQueue.main.async {
					self.operationInProgress = false
					ToastViewModel.shared.toastMessage = "Failed_to_create_conversation_error"
					ToastViewModel.shared.displayToast = true
				}
			}
		}, onConferenceJoined: { (chatRoom: ChatRoom, _: EventLog) in
			let state = chatRoom.state
			let id = LinphoneUtils.getChatRoomId(room: chatRoom)
			Log.info("\(StartConversationViewModel.TAG) Conversation \(id) \(chatRoom.subject ?? "") state changed: \(state)")
			if state == ChatRoom.State.Created {
				Log.info("\(StartConversationViewModel.TAG) Conversation \(id) successfully created")
				chatRoom.removeDelegate(delegate: self.chatRoomDelegate!)
				self.chatRoomDelegate = nil
				
				let model = ConversationModel(chatRoom: chatRoom)
				if 	self.operationInProgress == false {
					DispatchQueue.main.async {
						self.operationInProgress = true
					}
					
					DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
						SharedMainViewModel.shared.displayedConversation = model
						self.operationInProgress = false
					}
				} else {
					DispatchQueue.main.async {
						SharedMainViewModel.shared.displayedConversation = model
						self.operationInProgress = false
					}
				}
			} else if state == ChatRoom.State.CreationFailed {
				Log.error("\(StartConversationViewModel.TAG) Conversation \(id) creation has failed!")
				chatRoom.removeDelegate(delegate: self.chatRoomDelegate!)
				self.chatRoomDelegate = nil
				DispatchQueue.main.async {
					self.operationInProgress = false
					ToastViewModel.shared.toastMessage = "Failed_to_create_conversation_error"
					ToastViewModel.shared.displayToast = true
				}
			}
		})
		chatRoom.addDelegate(delegate: self.chatRoomDelegate!)
	}
	
	func changeDisplayedChatRoom(conversationModel: ConversationModel) {
		CoreContext.shared.doOnCoreQueue { core in
			let nilParams: ConferenceParams? = nil
			if let newChatRoom = core.searchChatRoom(params: nilParams, localAddr: nil, remoteAddr: conversationModel.chatRoom.peerAddress, participants: nil) {
				if LinphoneUtils.getChatRoomId(room: newChatRoom) == conversationModel.id {
					if self.sharedMainViewModel.displayedConversation == nil {
						DispatchQueue.main.async {
							withAnimation {
								self.sharedMainViewModel.displayedConversation = conversationModel
							}
						}
					} else {
						DispatchQueue.main.async {
							self.sharedMainViewModel.displayedConversation = nil
							DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
								withAnimation {
									self.sharedMainViewModel.displayedConversation = conversationModel
								}
							}
						}
					}
				}
			}
		}
	}
}
// swiftlint:enable type_body_length
// swiftlint:enable line_length
// swiftlint:enable cyclomatic_complexity
