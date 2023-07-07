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
import linphonesw

@objc class ScheduledConferencesView:  BackNextNavigationView, UICompositeViewDelegate, UITableViewDataSource, UITableViewDelegate {
			
	let conferenceListView = UITableView()
	let noConference = StyledLabel(VoipTheme.empty_list_font,VoipTexts.conference_no_schedule)
	let filters = UIStackView()
	let selectAllButton = CallControlButton(buttonTheme:VoipTheme.nav_button("deselect_all"))
	let separator = UIView()

	static let compositeDescription = UICompositeViewDescription(ScheduledConferencesView.self, statusBar: StatusBarView.self, tabBar: nil, sideMenu: SideMenuView.self, fullscreen: false, isLeftFragment: false,fragmentWith: nil)
	static func compositeViewDescription() -> UICompositeViewDescription! { return compositeDescription }
	func compositeViewDescription() -> UICompositeViewDescription! { return type(of: self).compositeDescription }
	
	override func viewDidLoad() {
		
		super.viewDidLoad(
			backAction: {
				if (ScheduledConferencesViewModel.shared.editionEnabled.value == true) {
					ScheduledConferencesViewModel.shared.editionEnabled.value  = false
				} else {
					PhoneMainView.instance().popView(self.compositeViewDescription())
				}
			},nextAction: {
				if (ScheduledConferencesViewModel.shared.editionEnabled.value == true) {
					self.deleteSelection()
				} else {
					ConferenceSchedulingViewModel.shared.reset()
					PhoneMainView.instance().changeCurrentView(ConferenceSchedulingView.compositeDescription)
				}
			},
			nextActionEnableCondition: MutableLiveData(),
			title:VoipTexts.conference_scheduled)
		super.titleLabel.applyStyle(VoipTheme.navigation_header_font)
		
		// Select all
		selectAllButton.setImage(UIImage(named: "deselect_all"), for: .selected)
		selectAllButton.setImage(UIImage(named: "select_all_default"), for: .normal)
		topBar.addSubview(selectAllButton)
		selectAllButton.toLeftOf(nextButton,withRightMargin: CGFloat(side_buttons_margin)).matchParentHeight().done()
		
		// Filter buttons
		let showTerminated = getFilterButton(title: VoipTexts.conference_scheduled_terminated_filter)
		showTerminated.onClick {
			ScheduledConferencesViewModel.shared.showTerminated.value = true
		}
		filters.addArrangedSubview(showTerminated)
		
		let showScheduled = getFilterButton(title: VoipTexts.conference_scheduled_future_filter)
		showScheduled.onClick {
			ScheduledConferencesViewModel.shared.showTerminated.value = false
			
		}
		filters.addArrangedSubview(showScheduled)
		
		ScheduledConferencesViewModel.shared.showTerminated.readCurrentAndObserve { it in
			showTerminated.isSelected = it == true
			showScheduled.isSelected = it != true
			self.noConference.text = it != true ? VoipTexts.conference_no_schedule : VoipTexts.conference_no_terminated_schedule
			ScheduledConferencesViewModel.shared.computeConferenceInfoList()
			self.conferenceListView.reloadData()
			self.noConference.isHidden = !ScheduledConferencesViewModel.shared.daySplitted.isEmpty
		}
		
		self.view.addSubview(filters)
		filters.spacing = 10
		filters.alignParentLeft(withMargin: 10).alignUnder(view: super.topBar,withMargin: self.form_margin).done()

		self.view.addSubview(separator)
		separator.matchParentSideBorders().height(1).alignUnder(view: filters,withMargin: self.form_margin).done()

		// Conference list
		
		self.view.addSubview(conferenceListView)
		conferenceListView.alignUnder(view: filters).done()
		conferenceListView.isScrollEnabled = true
		conferenceListView.dataSource = self
		conferenceListView.delegate = self
		conferenceListView.register(ScheduledConferencesCell.self, forCellReuseIdentifier: "ScheduledConferencesCell")
		conferenceListView.allowsSelection = false
		conferenceListView.rowHeight = UITableView.automaticDimension
		if #available(iOS 15.0, *) {
			conferenceListView.allowsFocus = false
		}
		conferenceListView.separatorStyle = .singleLine
		conferenceListView.backgroundColor = .clear
		
		view.addSubview(noConference)
		noConference.center().done()
		
		ScheduledConferencesViewModel.shared.editionEnabled.readCurrentAndObserve { editing in
			if (editing == true) {
				self.selectAllButton.isSelected = false
				self.selectAllButton.isHidden = false
				super.nextButton.applyTintedIcons(tintedIcons: VoipTheme.generic_delete_button)
				super.backButton.applyTintedIcons(tintedIcons: VoipTheme.generic_cancel)
				self.nextButton.isEnabled = ScheduledConferencesViewModel.shared.conferences.value?.filter{$0.selectedForDeletion.value == true}.count ?? 0 > 0
			} else {
				self.selectAllButton.isHidden = true
				ScheduledConferencesViewModel.shared.conferences.value?.forEach {$0.selectedForDeletion.value = false}
				super.nextButton.applyTintedIcons(tintedIcons: VoipTheme.conference_create_button)
				super.backButton.applyTintedIcons(tintedIcons: VoipTheme.generic_back)
				self.nextButton.isEnabled = true
			}
		}
		
		self.selectAllButton.onClick {
			let selectIt = !self.selectAllButton.isSelected
			ScheduledConferencesViewModel.shared.conferences.value?.forEach {$0.selectedForDeletion.value = selectIt}
		}
		
		UIDeviceBridge.displayModeSwitched.readCurrentAndObserve { _ in
			self.view.backgroundColor = VoipTheme.voipBackgroundBWColor.get()
			self.separator.backgroundColor  = VoipTheme.separatorColor.get()
			self.conferenceListView.separatorColor = .clear
			self.conferenceListView.reloadData()
		}
	}
	
	func getFilterButton(title:String) -> UIButton {
		let filter_button_height = 35.0
		let button = ButtonWithStateBackgrounds(backgroundStateColors: VoipTheme.button_conference_list_filter)
		button.setTitle(title, for: .normal)
		button.setTitleColor(.black, for: .normal)
		button.setTitleColor(VoipTheme.primary_color, for: .selected)
		button.height(filter_button_height).done()
		button.layer.cornerRadius = filter_button_height / 2
		button.clipsToBounds = true
		button.applyTitleStyle(VoipTheme.conf_list_filter_button_font)
		button.width(0).done()
		button.addSidePadding()
		return button
	}
		

	override func viewWillAppear(_ animated: Bool) {
		ScheduledConferencesViewModel.shared.computeConferenceInfoList()
		super.viewWillAppear(animated)
		self.conferenceListView.reloadData()
		self.conferenceListView.removeConstraints().done()
		self.conferenceListView.matchParentSideBorders(insetedByDx: 10).alignUnder(view: separator).alignParentBottom().done()
		noConference.isHidden = !ScheduledConferencesViewModel.shared.daySplitted.isEmpty
		super.nextButton.isEnabled = Core.get().defaultAccount != nil
		ScheduledConferencesViewModel.shared.editionEnabled.value = false
	}
		
	// TableView datasource delegate
		
	func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
		let daysArray = Array(ScheduledConferencesViewModel.shared.daySplitted.keys.sorted().reversed())
		let day = daysArray[section]
		return TimestampUtils.dateLongToString(date: day)
	}
	
	func tableView(_ tableView: UITableView, willDisplayHeaderView view: UIView, forSection section: Int) {
		guard let header = view as? UITableViewHeaderFooterView else { return }
		header.textLabel?.applyStyle(VoipTheme.conference_invite_title_font)
		header.textLabel?.matchParentSideBorders().done()
	}
	
	func numberOfSections(in tableView: UITableView) -> Int {
		return ScheduledConferencesViewModel.shared.daySplitted.keys.count
	}
	
	func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		let daysArray = Array(ScheduledConferencesViewModel.shared.daySplitted.keys.sorted().reversed())
		let day = daysArray[section]
		return ScheduledConferencesViewModel.shared.daySplitted[day]!.count
	}
	
	func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
		return UITableView.automaticDimension
	}
	
	
	func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		let cell:ScheduledConferencesCell = tableView.dequeueReusableCell(withIdentifier: "ScheduledConferencesCell") as! ScheduledConferencesCell
		let daysArray = Array(ScheduledConferencesViewModel.shared.daySplitted.keys.sorted().reversed())
		let day = daysArray[indexPath.section]
		guard let data = ScheduledConferencesViewModel.shared.daySplitted[day]?[indexPath.row] else {
			return cell
		}
		cell.conferenceData = data
		cell.owningTableView = tableView
		data.selectedForDeletion.readCurrentAndObserve { selected in
			let selectedCount = ScheduledConferencesViewModel.shared.conferences.value?.filter{$0.selectedForDeletion.value == true}.count ?? 0
			let totalCount = ScheduledConferencesViewModel.shared.conferences.value?.count ?? 0
			self.nextButton.isEnabled = ScheduledConferencesViewModel.shared.editionEnabled.value == false || selectedCount > 0
			self.selectAllButton.isSelected = selectedCount == totalCount
		}
		return cell
	}
	
	
	
	func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCell.EditingStyle, forRowAt indexPath: IndexPath) {
			if editingStyle == .delete {
				let cell = tableView.cellForRow(at: indexPath) as! ScheduledConferencesCell
				cell.askConfirmationTodeleteEntry()
			}
	}
	
	func deleteSelection () {
		let selectedCount = ScheduledConferencesViewModel.shared.conferences.value?.filter{$0.selectedForDeletion.value == true}.count ?? 0
		let delete = ButtonAttributes(text:VoipTexts.conference_info_confirm_removal_delete, action: {
			ScheduledConferencesViewModel.shared.conferences.value?.forEach   {
				$0.deleteConference()
			}
			ScheduledConferencesViewModel.shared.computeConferenceInfoList()
			self.conferenceListView.reloadData()
			VoipDialog.toast(message: selectedCount == 1 ? VoipTexts.conference_info_removed : VoipTexts.conference_infos_removed)
			ScheduledConferencesViewModel.shared.editionEnabled.value = false
		}, isDestructive:false)
		let cancel = ButtonAttributes(text:VoipTexts.cancel, action: {}, isDestructive:true)
		VoipDialog(message:selectedCount == 1 ? VoipTexts.conference_info_confirm_removal : VoipTexts.conference_infos_confirm_removal, givenButtons:  [cancel,delete]).show()
		
	}
	
}
