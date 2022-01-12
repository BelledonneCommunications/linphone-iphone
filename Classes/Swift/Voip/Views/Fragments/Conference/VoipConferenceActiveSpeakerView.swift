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
	let grid_height = 150.0
	let cell_width = 100.0

	
	let subjectLabel = StyledLabel(VoipTheme.call_display_name_duration)
	let duration = CallTimer(nil, VoipTheme.call_display_name_duration)
	
	let remotelyRecording =  RemotelyRecordingView(height: ActiveCallView.remote_recording_height,text: VoipTexts.call_remote_recording)
	var recordCallButtons : [CallControlButton] = []
	var pauseCallButtons :  [CallControlButton] = []
	
	let activeSpeakerView = UIView()
	let activeSpeakerVideoView = UIView()
	let activeSpeakerAvatar = Avatar(diameter: CGFloat(Avatar.diameter_for_call_views), color:VoipTheme.voipBackgroundColor, textStyle: VoipTheme.call_generated_avatar_large)
	let activeSpeakerDisplayName = StyledLabel(VoipTheme.call_remote_name)
	var activeSpeakerMonitorTimer : Timer? = nil

	var grid : UICollectionView
	
	
	var conferenceViewModel: ConferenceViewModel? = nil {
		didSet {
			if let model = conferenceViewModel {
				model.subject.readCurrentAndObserve { (subject) in
					self.subjectLabel.text = subject
				}
				duration.conference = model.conference.value
				self.remotelyRecording.isRemotelyRecorded = model.isRemotelyRecorded
				model.conferenceParticipantDevices.readCurrentAndObserve { (_) in
					self.grid.reloadData()
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
				Core.get().nativeVideoWindow = self.activeSpeakerVideoView
				activeSpeakerMonitorTimer?.invalidate()
				activeSpeakerMonitorTimer = Timer.scheduledTimer(withTimeInterval: 0.5, repeats: true) { timer in
					var thereIsAnActiveSpeaker = false
					model.conferenceParticipantDevices.value?.forEach { (data) in
						if (data.activeSpeaker.value == true) {
							thereIsAnActiveSpeaker = true
							data.participantDevice.address.map {
								self.activeSpeakerAvatar.isHidden = false
								self.activeSpeakerAvatar.fillFromAddress(address: $0)
								self.activeSpeakerDisplayName.text = $0.addressBookEnhancedDisplayName()
							}
							self.activeSpeakerVideoView.isHidden = data.videoEnabled.value != true
							return
						}
					}
					if (!thereIsAnActiveSpeaker) {
						self.activeSpeakerAvatar.isHidden = true
						self.activeSpeakerVideoView.isHidden = true
						self.activeSpeakerDisplayName.text = VoipTexts.conference_display_no_active_speaker
					}
				}
			} else {
				activeSpeakerMonitorTimer?.invalidate()
			}
			self.grid.reloadData()
			
		}
	}
			
	init() {
		
		let layout: UICollectionViewFlowLayout = UICollectionViewFlowLayout()
		layout.minimumInteritemSpacing = 0
		layout.minimumLineSpacing = 0
		layout.scrollDirection = .horizontal
		layout.itemSize = CGSize(width:cell_width, height:grid_height)
		grid = UICollectionView(frame:.zero, collectionViewLayout: layout)
		
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
		
		
		// Container view that can toggle full screen by ckick
		let fullScreenMutableView = UIView()
		addSubview(fullScreenMutableView)
		fullScreenMutableView.backgroundColor =  VoipTheme.voipBackgroundColor.get()
		fullScreenMutableView.matchParentSideBorders().alignUnder(view:headerView,withMargin: ActiveCallView.center_view_margin_top).alignParentBottom().done()

		
		// Active speaker
		fullScreenMutableView.addSubview(activeSpeakerView)
		activeSpeakerView.layer.cornerRadius = ActiveCallView.center_view_corner_radius
		activeSpeakerView.clipsToBounds = true
		activeSpeakerView.backgroundColor = VoipTheme.voipParticipantBackgroundColor.get()
		activeSpeakerView.matchParentSideBorders().alignParentTop().done()
				
		activeSpeakerView.addSubview(activeSpeakerAvatar)
		activeSpeakerAvatar.square(Avatar.diameter_for_call_views).center().done()
		
		activeSpeakerView.addSubview(activeSpeakerVideoView)
		activeSpeakerVideoView.matchParentDimmensions().done()

		activeSpeakerView.addSubview(activeSpeakerDisplayName)
		activeSpeakerDisplayName.alignParentLeft(withMargin:ActiveCallView.bottom_displayname_margin_left).alignParentRight().alignParentBottom(withMargin:ActiveCallView.bottom_displayname_margin_bottom).done()
		
		// CollectionView
		grid.dataSource = self
		grid.delegate = self
		grid.register(VoipActiveSpeakerParticipantCell.self, forCellWithReuseIdentifier: "VoipActiveSpeakerParticipantCell")
		grid.backgroundColor = .clear
		grid.isScrollEnabled = true
		fullScreenMutableView.addSubview(grid)
				
		grid.matchParentSideBorders().height(grid_height).alignParentBottom().alignUnder(view: activeSpeakerView, withMargin:ActiveCallView.center_view_margin_top).done()
		
		// Full screen video togggle
		activeSpeakerView.onClick {
			ControlsViewModel.shared.toggleFullScreen()
		}
		
		ControlsViewModel.shared.fullScreenMode.observe { (fullScreen) in
			if (self.isHidden) {
				return
			}
			fullScreenMutableView.removeConstraints().done()
			if (fullScreen == true) {
				fullScreenMutableView.removeFromSuperview()
				PhoneMainView.instance().mainViewController.view?.addSubview(fullScreenMutableView)
				fullScreenMutableView.matchParentDimmensions().done()
			} else {
				fullScreenMutableView.removeFromSuperview()
				self.addSubview(fullScreenMutableView)
				fullScreenMutableView.matchParentSideBorders().alignUnder(view:headerView,withMargin: ActiveCallView.center_view_margin_top).alignParentBottom().done()
			}
		}
		
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
		guard let participantsCount = conferenceViewModel?.conferenceParticipantDevices.value?.count else {
			return .zero
		}
		return participantsCount
	}
	
	func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
		let cell:VoipActiveSpeakerParticipantCell = collectionView.dequeueReusableCell(withReuseIdentifier: "VoipActiveSpeakerParticipantCell", for: indexPath) as! VoipActiveSpeakerParticipantCell
		guard let participantData = conferenceViewModel?.conferenceParticipantDevices.value?[indexPath.row] else {
			return cell
		}
		cell.participantData = participantData
		return cell
	}
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
	
	
}
