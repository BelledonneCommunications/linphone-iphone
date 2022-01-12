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

@objc class ScheduledConferencesView:  BackNextNavigationView, UICompositeViewDelegate, UITableViewDataSource {
	
	let conferenceListView = UITableView()
	let noConference = StyledLabel(VoipTheme.empty_list_font,VoipTexts.conference_no_schedule)

	static let compositeDescription = UICompositeViewDescription(ScheduledConferencesView.self, statusBar: StatusBarView.self, tabBar: nil, sideMenu: SideMenuView.self, fullscreen: false, isLeftFragment: false,fragmentWith: nil)
	static func compositeViewDescription() -> UICompositeViewDescription! { return compositeDescription }
	func compositeViewDescription() -> UICompositeViewDescription! { return type(of: self).compositeDescription }
	
	override func viewDidLoad() {
		
		super.viewDidLoad(
			backAction: {
				PhoneMainView.instance().popView(self.compositeViewDescription())
			},nextAction: {
			},
			nextActionEnableCondition: MutableLiveData(false),
			title:VoipTexts.conference_scheduled)
		super.nextButton.isHidden = true
	
	
		contentView.addSubview(conferenceListView)
		conferenceListView.isScrollEnabled = false
		conferenceListView.dataSource = self
		conferenceListView.register(ScheduledConferencesCell.self, forCellReuseIdentifier: "ScheduledConferencesCell")
		conferenceListView.allowsSelection = false
		if #available(iOS 15.0, *) {
			conferenceListView.allowsFocus = false
		}
		conferenceListView.separatorStyle = .singleLine
		conferenceListView.separatorColor = VoipTheme.light_grey_color
		
		view.addSubview(noConference)
		noConference.center().done()

		
	}
		

	override func viewWillAppear(_ animated: Bool) {
		ScheduledConferencesViewModel.shared.computeConferenceInfoList()
		super.viewWillAppear(animated)
		self.conferenceListView.reloadData()
		self.conferenceListView.removeConstraints().done()
		self.conferenceListView.matchParentSideBorders().alignUnder(view: super.topBar,withMargin: self.form_margin).alignParentBottom().done()
		noConference.isHidden = !ScheduledConferencesViewModel.shared.daySplitted.isEmpty
	}
		
	// TableView datasource delegate
		
	func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
		let daysArray = Array(ScheduledConferencesViewModel.shared.daySplitted.keys)
		let day = daysArray[section]
		return TimestampUtils.dateToString(date: day)
	}
	
	func tableView(_ tableView: UITableView, willDisplayHeaderView view: UIView, forSection section: Int) {
		guard let header = view as? UITableViewHeaderFooterView else { return }
		header.textLabel?.applyStyle(VoipTheme.conference_invite_title_font)
	}
	
	func numberOfSections(in tableView: UITableView) -> Int {
		return ScheduledConferencesViewModel.shared.daySplitted.keys.count
	}
	
	func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		let daysArray = Array(ScheduledConferencesViewModel.shared.daySplitted.keys)
		let day = daysArray[section]
		return ScheduledConferencesViewModel.shared.daySplitted[day]!.count
	}
	
	func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		let cell:ScheduledConferencesCell = tableView.dequeueReusableCell(withIdentifier: "ScheduledConferencesCell") as! ScheduledConferencesCell
		let daysArray = Array(ScheduledConferencesViewModel.shared.daySplitted.keys)
		let day = daysArray[indexPath.section]
		guard let data = ScheduledConferencesViewModel.shared.daySplitted[day]?[indexPath.row] else {
			return cell
		}
		cell.conferenceData = data
		return cell
	}
	
	
}
