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

class VoipConferenceGridView: UIView, UICollectionViewDataSource, UICollectionViewDelegate, UICollectionViewDelegateFlowLayout {
	
	// Layout constants :
	let inter_cell = 10.0
	let record_pause_button_margin = 10.0
	let duration_margin_top = 4.0
	let record_pause_button_size = 40
	let record_pause_button_inset = UIEdgeInsets(top: 7, left: 7, bottom: 7, right: 7)
	
	
	let subjectLabel = StyledLabel(VoipTheme.call_display_name_duration)
	let duration = CallTimer(nil, VoipTheme.call_display_name_duration)
	
	let remotelyRecording =  RemotelyRecordingView(height: ActiveCallView.remote_recording_height,text: VoipTexts.call_remote_recording)
	var recordCallButtons : [CallControlButton] = []
	var pauseCallButtons :  [CallControlButton] = []
	var grid : UICollectionView
	var gridContainer = UIView()

	
	var conferenceViewModel: ConferenceViewModel? = nil {
		didSet {
			if let model = conferenceViewModel {
				model.subject.clearObservers()
				model.subject.readCurrentAndObserve { (subject) in
					self.subjectLabel.text = subject
				}
				duration.conference = model.conference.value
				self.remotelyRecording.isRemotelyRecorded = model.isRemotelyRecorded
				model.conferenceParticipantDevices.clearObservers()
				model.conferenceParticipantDevices.readCurrentAndObserve { (devices) in
					if (devices!.count > model.maxParticipantsForMosaicLayout && model.conference.value?.currentParams?.videoEnabled == true && model.conferenceDisplayMode.value == .Grid) {
						Log.w("[Conference] \(model.conference) More than \(model.maxParticipantsForMosaicLayout) participants \(devices!.count), forcing active speaker layout from Grid")
						model.conferenceDisplayMode.value = .ActiveSpeaker
						model.changeLayout(layout: .ActiveSpeaker)
						VoipDialog.toast(message: VoipTexts.conference_too_many_participants_for_mosaic_layout)
					} else {
						self.reloadData()
					}
				}
				model.isConferenceLocallyPaused.readCurrentAndObserve { (paused) in
					self.pauseCallButtons.forEach {
						$0.isSelected = paused == true
					}
				}
				model.isRecording.clearObservers()
				model.isRecording.readCurrentAndObserve { (selected) in
					self.recordCallButtons.forEach {
						$0.isSelected = selected == true
					}
				}
			}
			self.reloadData()
		}
	}
			
	init() {
		
		let layout: UICollectionViewFlowLayout = UICollectionViewFlowLayout()
		layout.minimumInteritemSpacing = 0
		layout.minimumLineSpacing = 0
		layout.estimatedItemSize = .zero
		grid = UICollectionView(frame:.zero, collectionViewLayout: layout)
		
		super.init(frame: .zero)
		
		let headerView = UIStackView()
		addSubview(headerView)
		
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
		
		// CollectionView
		grid.dataSource = self
		grid.delegate = self
		grid.register(VoipGridParticipantCell.self, forCellWithReuseIdentifier: "VoipGridParticipantCell")
		grid.backgroundColor = .clear
		grid.isScrollEnabled = false
		addSubview(gridContainer)
		gridContainer.addSubview(grid)
		gridContainer.backgroundColor = ControlsViewModel.shared.fullScreenMode.value == true ? .black : VoipTheme.voipBackgroundColor.get()
		
		gridContainer.matchParentSideBorders(insetedByDx: inter_cell).alignUnder(view:headerView,withMargin: ActiveCallView.center_view_margin_top).alignParentBottom(withMargin: inter_cell).done()
		grid.matchParentDimmensions().done()
	
		headerView.matchParentSideBorders().alignParentTop().done()
				
		
		// Full screen video togggle
		gridContainer.onClick {
			ControlsViewModel.shared.toggleFullScreen()
		}
		
		ControlsViewModel.shared.fullScreenMode.observe { (fullScreen) in
			if (self.superview?.superview?.superview == nil || self.conferenceViewModel?.conference.value?.call?.params?.conferenceVideoLayout != .Grid) {
				return
			}
			self.gridContainer.removeConstraints().done()
			if (fullScreen == true) {
				self.gridContainer.removeFromSuperview()
				PhoneMainView.instance().mainViewController.view?.addSubview(self.gridContainer)
				self.gridContainer.matchParentDimmensions().center().done()
				self.gridContainer.backgroundColor = .black
			} else {
				self.gridContainer.removeFromSuperview()
				self.addSubview(self.gridContainer)
				self.gridContainer.matchParentSideBorders().alignUnder(view:headerView,withMargin: ActiveCallView.center_view_margin_top).alignParentBottom().done()
				self.gridContainer.backgroundColor = VoipTheme.voipBackgroundColor.get()
			}
			DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
				self.reloadData()
			}
		}
		
		//Appearance
		UIDeviceBridge.displayModeSwitched.observe { _ in
			self.gridContainer.backgroundColor = ControlsViewModel.shared.fullScreenMode.value == true ? .black : VoipTheme.voipBackgroundColor.get()
			self.reloadData()
		}
		
	}
	
	
	// UICollectionView related delegates
	
	func reloadData() {
		conferenceViewModel?.conferenceParticipantDevices.value?.forEach {
			$0.clearObservers()
		}
		if let participantCount = conferenceViewModel?.conferenceParticipantDevices.value!.count, participantCount > conferenceViewModel!.maxParticipantsForMosaicLayout {
			return
		}
		if (self.isHidden) {
			self.grid.reloadData()
			return
		}
		computeCellSize()
		self.grid.reloadData()
		DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
			let width:CGFloat = CGFloat(self.columnCount) * self.cellSize.width + (CGFloat(self.columnCount)-1.0)*self.inter_cell
			let height:CGFloat = CGFloat(self.rowCount) * self.cellSize.height + (CGFloat(self.rowCount)-1.0)*self.inter_cell
			if (width > 0) {
				self.grid.removeConstraints().width(width).height(height).center().done()
			}
		}
	}
	
	
	func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, minimumInteritemSpacingForSectionAt section: Int) -> CGFloat {
	   return inter_cell
	}

	func collectionView(_ collectionView: UICollectionView, layout
				collectionViewLayout: UICollectionViewLayout,
								minimumLineSpacingForSectionAt section: Int) -> CGFloat {
	 return inter_cell
	}
	
	func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
		if (self.isHidden) {
			return 0
		}
		guard let participantsCount = conferenceViewModel?.conferenceParticipantDevices.value?.count else {
			return .zero
		}
		return participantsCount
	}
	
	func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
		let cell:VoipGridParticipantCell = collectionView.dequeueReusableCell(withReuseIdentifier: "VoipGridParticipantCell", for: indexPath) as! VoipGridParticipantCell
		guard let participantData = conferenceViewModel?.conferenceParticipantDevices.value?[indexPath.row] else {
			return cell
		}
		cell.participantData = participantData
		return cell
	}
	
	let placement = [[1, 2, 3, 4, 5, 6], [1, 1, 2, 2, 3,3], [1, 1, 1, 2, 2, 2],  [1, 1, 1, 1, 2, 2],  [1, 1, 1, 1, 1, 2],  [1, 1, 1, 1, 1, 1]]
	var cellSize: CGSize = .zero
	var columnCount: Int = 0
	var rowCount: Int = 0
	
	func computeCellSize() {
		let participantsCount = self.collectionView(self.grid, numberOfItemsInSection: 0)
		if (participantsCount == 0) {
			return
		}
		let availableSize = gridContainer.frame.size
		var maxSize = 0.0
		for rowCount in 1...participantsCount {
			let neededColumns = placement[rowCount-1][participantsCount-1]
			let candidateWidth = availableSize.width / CGFloat(neededColumns) - CGFloat((neededColumns-1) * Int(inter_cell))
			let candidateHeight = availableSize.height / CGFloat(rowCount) - CGFloat((rowCount - 1) * Int(inter_cell))
			let candidateSize = min(candidateWidth,candidateHeight)
			if (candidateSize > maxSize) {
				self.columnCount = neededColumns
				self.rowCount = rowCount
				maxSize = candidateSize
			}
			Log.i("neededColumns \(neededColumns) rowCount \(rowCount) availableSize \(availableSize) participantsCount \(participantsCount) candidateWidth \(candidateWidth) candidateHeight \(candidateHeight) candidateSize \(candidateSize) maxSize \(maxSize)")
		}
				
		cellSize =  CGSize(width: maxSize ,height: maxSize)
	}
	
	func collectionView(_ collectionView: UICollectionView,
						layout collectionViewLayout: UICollectionViewLayout,
						sizeForItemAt indexPath: IndexPath) -> CGSize {
		
		guard let _ = conferenceViewModel?.conferenceParticipantDevices.value?.count else {
			return .zero
		}
		
		return cellSize
	}
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
	
	
}
