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


import UIKit
import Foundation
import SnapKit
import linphonesw

class VoipConferenceActiveSpeakerView: UIView, UICollectionViewDataSource, UICollectionViewDelegate, UICollectionViewDelegateFlowLayout {
	
	// Layout constants :
	let inter_cell = 10.0
	let record_pause_button_margin = 10.0
	let duration_margin_top = 4.0
	let record_pause_button_size = 40
	let record_pause_button_inset = UIEdgeInsets(top: 7, left: 7, bottom: 7, right: 7)
	let grid_height = 100.0
	let cell_width = 100.0
	let switch_camera_button_size = 35
	let switch_camera_button_margins = 7.0
		
	let switchCamera = UIImageView(image: UIImage(named:"voip_change_camera")?.tinted(with:.white))
	let subjectLabel = StyledLabel(VoipTheme.call_display_name_duration)
	let duration = CallTimer(nil, VoipTheme.call_display_name_duration)
	let muted = MicMuted(VoipActiveSpeakerParticipantCell.mute_size)
	let pause = UIImageView(image: UIImage(named: "voip_pause")?.tinted(with: .white))

	let remotelyRecording =  RemotelyRecordingView(height: ActiveCallView.remote_recording_height,text: VoipTexts.call_remote_recording)
	var recordCallButtons : [CallControlButton] = []
	var pauseCallButtons :  [CallControlButton] = []
	
	let activeSpeakerView = UIView()
	let activeSpeakerVideoView = UIView()
	let activeSpeakerVideoViewAlone = UIView()
	let activeSpeakerAvatar = Avatar(color:VoipTheme.voipBackgroundColor, textStyle: VoipTheme.call_generated_avatar_large)
	let activeSpeakerDisplayName = StyledLabel(VoipTheme.call_remote_name)

	var grid : UICollectionView
	var meGrid : UICollectionView

	let layout: UICollectionViewFlowLayout = UICollectionViewFlowLayout()
	var fullScreenOpaqueMasqForNotchedDevices =  UIView()
	let conferenceJoinSpinner = RotatingSpinner(color:VoipTheme.dark_grey_color)

	
	var conferenceViewModel: ConferenceViewModel? = nil {
		didSet {
			if let model = conferenceViewModel {
				self.activeSpeakerVideoView.isHidden = true
				self.activeSpeakerVideoViewAlone.isHidden = true
				self.setJoininngSpeakerState(enabled: false)
				self.activeSpeakerAvatar.showAsAvatarIcon()
				model.subject.readCurrentAndObserve { (subject) in
					self.subjectLabel.text = subject
				}
				duration.conference = model.conference.value
				self.remotelyRecording.isRemotelyRecorded = model.isRemotelyRecorded
				model.conferenceParticipantDevices.readCurrentAndObserve { value in
					model.activeSpeakerConferenceParticipantDevices.value = Array((value!.dropFirst().filter { !$0.isMe } ))
				}
				model.activeSpeakerConferenceParticipantDevices.readCurrentAndObserve { (_) in
					self.reloadData()
					let otherSpeakersCount = model.activeSpeakerConferenceParticipantDevices.value!.count
					self.switchCamera.isHidden = true
					if (otherSpeakersCount == 0) {
						self.layoutRotatableElements()
						self.meGrid.isHidden = true
						self.grid.isHidden = true
						model.meParticipant.value?.videoEnabled.readCurrentAndObserve { video in
							self.switchCamera.isHidden = video != true
							self.fillActiveSpeakerSpace(data: model.meParticipant.value,video: video == true, alone:true)
						}
						model.meParticipant.value?.micMuted.readCurrentAndObserve { muted in
							self.muted.isHidden = muted != true
						}
						model.meParticipant.value?.isInConference.readCurrentAndObserve { isIn in
							self.pause.isHidden = isIn == true
							if (isIn != true) {
								self.activeSpeakerVideoView.isHidden = true
								self.activeSpeakerVideoViewAlone.isHidden = true
							}
						}
					} else if (otherSpeakersCount == 1) {
						if let data =  model.activeSpeakerConferenceParticipantDevices.value!.first {
							data.videoEnabled.readCurrentAndObserve { video in
								self.fillActiveSpeakerSpace(data: data,video: video == true)
							}
							data.micMuted.readCurrentAndObserve { muted in
								self.muted.isHidden = muted != true
							}
							data.isInConference.readCurrentAndObserve { isIn in
								self.pause.isHidden = isIn == true || data.isJoining.value == true
								if (isIn != true) {
									self.activeSpeakerVideoView.isHidden = true
									self.activeSpeakerVideoViewAlone.isHidden = true
								}
							}
						}
						self.layoutRotatableElements()
						self.meGrid.isHidden = false
						self.grid.isHidden = true
					} else if (otherSpeakersCount == 2) {
						self.meGrid.isHidden = false
						self.grid.isHidden = false
						self.layoutRotatableElements()
					} else {
						self.activeSpeakerVideoView.isHidden = false
						self.activeSpeakerVideoViewAlone.isHidden = true
						self.meGrid.isHidden = false
						self.grid.isHidden = false
						self.layoutRotatableElements()
					}
				}
				model.isConferenceLocallyPaused.readCurrentAndObserve { (paused) in
					self.pauseCallButtons.forEach {
						$0.isSelected = paused == true
					}
				}
				model.isRecording.readCurrentAndObserve { (selected) in
					self.recordCallButtons.forEach {
						$0.isSelected = selected == true
					}
				}
				model.speakingParticipant.readCurrentAndObserve { speakingParticipant in
					if (model.activeSpeakerConferenceParticipantDevices.value!.count > 1) {
						speakingParticipant?.videoEnabled.readCurrentAndObserve { video in
							self.fillActiveSpeakerSpace(data: speakingParticipant,video: video == true)
							self.muted.isHidden = true
						}
						speakingParticipant?.isInConference.readCurrentAndObserve { isIn in
							self.pause.isHidden = isIn == true
							if (isIn != true) {
								self.activeSpeakerVideoView.isHidden = true
								self.activeSpeakerVideoViewAlone.isHidden = true
							}
						}
					}
				}
			}
			self.reloadData()
			
		}
	}
	
	func setJoininngSpeakerState(enabled: Bool) {
		if (!enabled) {
			self.conferenceJoinSpinner.isHidden = true
			self.conferenceJoinSpinner.stopRotation()
		} else {
			self.conferenceJoinSpinner.isHidden = false
			self.conferenceJoinSpinner.startRotation()
		}
	}
	
	func fillActiveSpeakerSpace(data: ConferenceParticipantDeviceData?, video: Bool, alone: Bool = false) {
		data?.isJoining.readCurrentAndObserve { joining in
			self.setJoininngSpeakerState(enabled: joining == true || data?.participantDevice.address == nil)
		}
		if let address = data?.participantDevice.address {
			self.activeSpeakerAvatar.fillFromAddress(address: address)
			self.activeSpeakerDisplayName.text = address.addressBookEnhancedDisplayName()
		} else {
			self.activeSpeakerAvatar.showAsAvatarIcon()
			self.activeSpeakerDisplayName.text = nil
		}
		if (video) {
			if (alone) {
				Core.get().nativePreviewWindow = self.activeSpeakerVideoViewAlone
			} else {
				Core.get().nativeVideoWindow = self.activeSpeakerVideoView
			}
		}
		self.activeSpeakerVideoView.isHidden = !video || alone
		self.activeSpeakerVideoViewAlone.isHidden = !video || !alone
	}
	
	func reloadData() {
		self.grid.reloadData()
		self.meGrid.reloadData()
	}
			
	init() {
		
		layout.minimumInteritemSpacing = 0
		layout.minimumLineSpacing = 0
		layout.scrollDirection = .horizontal
		layout.itemSize = CGSize(width:cell_width, height:grid_height)
		grid = UICollectionView(frame:.zero, collectionViewLayout: layout)
		
		let meLayout: UICollectionViewFlowLayout = UICollectionViewFlowLayout()
		meLayout.scrollDirection = .horizontal
		meLayout.minimumInteritemSpacing = 0
		meLayout.minimumLineSpacing = 0
		meLayout.itemSize = CGSize(width:cell_width, height:grid_height)
		meGrid = UICollectionView(frame:.zero, collectionViewLayout: meLayout)
		
		super.init(frame: .zero)
		
		let headerView = UIStackView()
		addSubview(headerView)
		headerView.matchParentSideBorders().alignParentTop().done()
		
		headerView.distribution = .equalSpacing
		headerView.alignment = .bottom
		headerView.spacing = record_pause_button_margin
		headerView.axis = .vertical
				
		let subjectDuration = UIView()
		
		subjectDuration.addSubview(subjectLabel)
		subjectLabel.alignParentLeft().done()
	
		subjectDuration.addSubview(duration)
		duration.alignParentLeft().alignUnder(view: subjectLabel,withMargin:duration_margin_top).done()
	
		let upperSection = UIStackView()
		upperSection.distribution = .equalSpacing
		upperSection.alignment = .center
		upperSection.spacing = record_pause_button_margin
		upperSection.axis = .horizontal
		
		upperSection.addArrangedSubview(subjectDuration)
		subjectDuration.wrapContentY().done()
		
		// Record (with video)
		let recordCall = CallControlButton(width: record_pause_button_size, height: record_pause_button_size, imageInset:record_pause_button_inset, buttonTheme: VoipTheme.call_record, onClickAction: {
			self.conferenceViewModel?.toggleRecording()
		})
		
		recordCall.isHidden = true;
		
		let recordPauseView = UIStackView()
		recordPauseView.spacing = record_pause_button_margin
		recordCallButtons.append(recordCall)
		recordPauseView.addArrangedSubview(recordCall)
		
		// Pause (with video)
		let pauseCall = CallControlButton(width: record_pause_button_size, height: record_pause_button_size, imageInset:record_pause_button_inset, buttonTheme: VoipTheme.call_pause, onClickAction: {
			self.conferenceViewModel?.togglePlayPause()

		})
		pauseCallButtons.append(pauseCall)
		recordPauseView.addArrangedSubview(pauseCall)
								   
		upperSection.addArrangedSubview(recordPauseView)
		
		headerView.addArrangedSubview(upperSection)
		upperSection.matchParentSideBorders().alignParentTop(withMargin:ActiveCallView.top_displayname_margin_top).done()
		
		headerView.addArrangedSubview(remotelyRecording)
		remotelyRecording.matchParentSideBorders().alignUnder(view:upperSection, withMargin:ActiveCallView.remote_recording_margin_top).height(CGFloat(ActiveCallView.remote_recording_height)).done()
		
		
		// Container view that can toggle full screen by single tap
		let fullScreenMutableView = UIView()
		addSubview(fullScreenMutableView)
		fullScreenMutableView.backgroundColor = ControlsViewModel.shared.fullScreenMode.value == true ? .black : VoipTheme.voipBackgroundColor.get()
		fullScreenMutableView.matchParentSideBorders().alignUnder(view:headerView,withMargin: ActiveCallView.center_view_margin_top).alignParentBottom().done()
		fullScreenOpaqueMasqForNotchedDevices.backgroundColor = fullScreenMutableView.backgroundColor
		
		// Active speaker
		fullScreenMutableView.addSubview(activeSpeakerView)
		activeSpeakerView.layer.cornerRadius = ActiveCallView.center_view_corner_radius
		activeSpeakerView.clipsToBounds = true
		activeSpeakerView.backgroundColor = VoipTheme.voipParticipantBackgroundColor.get()
				
		activeSpeakerView.addSubview(activeSpeakerAvatar)
		
		activeSpeakerView.addSubview(activeSpeakerVideoView)
		activeSpeakerVideoView.matchParentDimmensions().done()
		activeSpeakerVideoView.contentMode = .scaleAspectFill
		activeSpeakerView.addSubview(activeSpeakerVideoViewAlone)
		activeSpeakerVideoViewAlone.matchParentDimmensions().done()
		activeSpeakerVideoViewAlone.contentMode = .scaleAspectFill

		activeSpeakerView.addSubview(switchCamera)
		switchCamera.contentMode = .scaleAspectFit
		switchCamera.onClick {
			Core.get().toggleCamera()
		}
		
		activeSpeakerView.addSubview(muted)
		muted.isHidden = true
		muted.alignParentLeft(withMargin: switch_camera_button_margins).alignParentTop(withMargin:switch_camera_button_margins).done()
		
		activeSpeakerView.addSubview(conferenceJoinSpinner)
		conferenceJoinSpinner.square(AbstractIncomingOutgoingCallView.spinner_size).center().done()

		switchCamera.alignParentTop(withMargin: switch_camera_button_margins).alignParentRight(withMargin: switch_camera_button_margins).square(switch_camera_button_size).done()
		
		activeSpeakerView.addSubview(activeSpeakerDisplayName)
		activeSpeakerDisplayName.alignParentLeft(withMargin:ActiveCallView.bottom_displayname_margin_left).alignParentRight().alignParentBottom(withMargin:ActiveCallView.bottom_displayname_margin_bottom).done()
		
		activeSpeakerAvatar.addSubview(pause)
		pause.isHidden = true
		pause.backgroundColor = activeSpeakerAvatar.backgroundColor
		pause.matchParentDimmensions().done()
		pause.contentMode = .scaleAspectFit
		
		// CollectionViews
		grid.dataSource = self
		grid.delegate = self
		grid.register(VoipActiveSpeakerParticipantCell.self, forCellWithReuseIdentifier: "VoipActiveSpeakerParticipantCell")
		grid.backgroundColor = .clear
		grid.isScrollEnabled = true
		fullScreenMutableView.addSubview(grid)
		
		meGrid.dataSource = self
		meGrid.delegate = self
		meGrid.register(VoipActiveSpeakerParticipantCell.self, forCellWithReuseIdentifier: "VoipActiveSpeakerParticipantCell")
		meGrid.backgroundColor = .clear
		meGrid.isScrollEnabled = false
		fullScreenMutableView.addSubview(meGrid)
		
						
		// Full screen video togggle
		activeSpeakerView.onClick {
			ControlsViewModel.shared.toggleFullScreen()
		}
		
		ControlsViewModel.shared.fullScreenMode.observe { (fullScreen) in
			if (self.superview?.superview?.superview == nil || self.conferenceViewModel?.conference.value?.call?.params?.conferenceVideoLayout != .ActiveSpeaker) {
				return
			}
			fullScreenMutableView.removeConstraints().done()
			fullScreenMutableView.removeFromSuperview()
			self.fullScreenOpaqueMasqForNotchedDevices.removeFromSuperview()
			if (fullScreen == true) {
				fullScreenMutableView.backgroundColor = .black
				self.fullScreenOpaqueMasqForNotchedDevices.backgroundColor = .black
				self.fullScreenOpaqueMasqForNotchedDevices.addSubview(fullScreenMutableView)
				PhoneMainView.instance().mainViewController.view?.addSubview(self.fullScreenOpaqueMasqForNotchedDevices)
				self.fullScreenOpaqueMasqForNotchedDevices.matchParentDimmensions().done()
				if (UIDevice.hasNotch()) {
					fullScreenMutableView.matchParentDimmensions(insetedBy:UIApplication.shared.keyWindow!.safeAreaInsets).done()
				} else {
					fullScreenMutableView.matchParentDimmensions().done()
				}
			} else {
				fullScreenMutableView.backgroundColor = VoipTheme.voipBackgroundColor.get()
				self.addSubview(fullScreenMutableView)
				fullScreenMutableView.matchParentSideBorders().alignUnder(view:headerView,withMargin: ActiveCallView.center_view_margin_top).alignParentBottom().done()
			}
			UIView.animate(withDuration: 0.3, animations: {
					self.layoutIfNeeded()
			})
			DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
				self.reloadData()
			}
		}
		
		//Rotation
		layoutRotatableElements()
		
		//Appearance
		UIDeviceBridge.displayModeSwitched.observe { _ in
			fullScreenMutableView.backgroundColor = ControlsViewModel.shared.fullScreenMode.value == true ? .black : VoipTheme.voipBackgroundColor.get()
			self.fullScreenOpaqueMasqForNotchedDevices.backgroundColor = fullScreenMutableView.backgroundColor
			self.activeSpeakerView.backgroundColor = VoipTheme.voipParticipantBackgroundColor.get()
			self.pause.backgroundColor = self.activeSpeakerAvatar.backgroundColor
			self.reloadData()
		}
	
	}
	
	// Rotations
	
	func bounceGrids() {
		let superView = grid.superview
		grid.removeFromSuperview()
		meGrid.removeFromSuperview()
		superView?.addSubview(grid)
		superView?.addSubview(meGrid)
	}
	
	func layoutRotatableElements() {
		grid.removeConstraints().done()
		meGrid.removeConstraints().done()
		activeSpeakerView.removeConstraints().done()
		activeSpeakerAvatar.removeConstraints().done()
		let otherParticipantsCount = conferenceViewModel?.activeSpeakerConferenceParticipantDevices.value!.count
		if ([.landscapeLeft, .landscapeRight].contains( UIDevice.current.orientation)) {
			if (otherParticipantsCount == 0) {
				activeSpeakerView.matchParentDimmensions().done()
				activeSpeakerAvatar.square(Avatar.diameter_for_call_views_land).center().done()
				if (UIDevice.current.orientation == .landscapeLeft) { // work around some constraints issues with Notch on the left.
					bounceGrids()
				}
			} else if (otherParticipantsCount == 1) {
				activeSpeakerView.matchParentDimmensions().done()
				if (UIDevice.current.orientation == .landscapeLeft) { // work around some constraints issues with Notch on the left.
					bounceGrids()
				}
				activeSpeakerAvatar.square(Avatar.diameter_for_call_views_land).center().done()
				meGrid.alignParentRight(withMargin: ActiveCallView.center_view_margin_top).height(grid_height).width(grid_height).alignParentBottom(withMargin: ActiveCallView.center_view_margin_top).done()
			} else {
				activeSpeakerView.alignParentTop().alignParentBottom().alignParentLeft().toLeftOf(grid,withRightMargin: SharedLayoutConstants.content_inset).done()
				if (UIDevice.current.orientation == .landscapeLeft) { // work around some constraints issues with Notch on the left.
					bounceGrids()
				}
				meGrid.width(grid_height).height(grid_height).toRightOf(activeSpeakerView,withLeftMargin: SharedLayoutConstants.content_inset).alignParentBottom().alignParentRight().done()
				grid.width(grid_height).toRightOf(activeSpeakerView,withLeftMargin: SharedLayoutConstants.content_inset).alignParentTop().alignAbove(view: meGrid, withMargin: SharedLayoutConstants.content_inset).alignParentRight().done()
				layout.scrollDirection = .vertical
				activeSpeakerAvatar.square(Avatar.diameter_for_call_views_land).center().done()
			}
		} else {
			if (otherParticipantsCount == 0) {
				activeSpeakerView.matchParentDimmensions().done()
				activeSpeakerAvatar.square(Avatar.diameter_for_call_views).center().done()
			} else if (otherParticipantsCount == 1) {
				activeSpeakerView.matchParentDimmensions().done()
				activeSpeakerAvatar.square(Avatar.diameter_for_call_views).center().done()
				meGrid.alignParentRight(withMargin: ActiveCallView.center_view_margin_top).height(grid_height).width(grid_height).alignParentBottom(withMargin: ActiveCallView.center_view_margin_top).done()
			} else {
				activeSpeakerAvatar.square(Avatar.diameter_for_call_views).center().done()
				activeSpeakerView.matchParentSideBorders().alignParentTop().done()
				meGrid.alignParentLeft().height(grid_height).width(grid_height).alignParentBottom().alignUnder(view: activeSpeakerView, withMargin:ActiveCallView.center_view_margin_top).done()
				grid.toRightOf(meGrid,withLeftMargin: SharedLayoutConstants.content_inset).height(grid_height).alignParentRight().alignParentBottom().alignUnder(view: activeSpeakerView, withMargin:ActiveCallView.center_view_margin_top).done()
				layout.scrollDirection = .horizontal
			}
		}
		UIView.animate(withDuration: 0.3, animations: {
				self.layoutIfNeeded()
		})
	}
	
	// UICollectionView related delegates
	
	func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, minimumInteritemSpacingForSectionAt section: Int) -> CGFloat {
	   return inter_cell
	}

	func collectionView(_ collectionView: UICollectionView, layout
				collectionViewLayout: UICollectionViewLayout,
								minimumLineSpacingForSectionAt section: Int) -> CGFloat {
	 return inter_cell
	}
	
	func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
		if (self.isHidden || conferenceViewModel?.conference.value?.call?.params?.conferenceVideoLayout != .ActiveSpeaker) {
			return 0
		}
		guard let participantsCount = collectionView == meGrid ? (conferenceViewModel?.meParticipant.value != nil ? 1 : 0) : conferenceViewModel?.activeSpeakerConferenceParticipantDevices.value?.count else {
			return .zero
		}
		return participantsCount
	}
	
	func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
		let cell:VoipActiveSpeakerParticipantCell = collectionView.dequeueReusableCell(withReuseIdentifier: "VoipActiveSpeakerParticipantCell", for: indexPath) as! VoipActiveSpeakerParticipantCell
		guard let participantData =  collectionView == meGrid ? conferenceViewModel?.meParticipant.value : conferenceViewModel?.activeSpeakerConferenceParticipantDevices.value?[indexPath.row] else {
			return cell
		}
		cell.participantData = participantData
		return cell
	}
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
	
	
}
