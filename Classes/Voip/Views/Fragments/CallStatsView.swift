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

import Foundation
import linphonesw

@objc class CallStatsView: UIView {
	
	// Layout constants
	let side_margins = 10.0
	let margin_top = 50
	let corner_radius = 20.0
	let view_height = 600
	let audio_video_margin = 20
	
	init(superView:UIView, callData:CallData, onDismissAction : @escaping ()->Void) {
		super.init(frame:.zero)
		backgroundColor = VoipTheme.voip_translucent_popup_background
		layer.cornerRadius = corner_radius
		clipsToBounds = true
		superView.addSubview(self)
		snp.makeConstraints { make in
			make.left.equalToSuperview().offset(side_margins)
			make.right.equalToSuperview().offset(-side_margins)
			make.height.equalTo(view_height)
			make.bottom.equalToSuperview().offset(-side_margins)
		}
		callData.callState.observe { state in
			if (state == Call.State.End) {
				onDismissAction()
			}
		}
		
		let hide = CallControlButton(buttonTheme: VoipTheme.voip_cancel_light, onClickAction: {
			onDismissAction()
		})
		addSubview(hide)
		hide.alignParentRight(withMargin: side_margins).alignParentTop(withMargin: side_margins).done()

		
		let model = CallStatisticsData(call: callData.call)
		let audioTitle = StyledLabel(VoipTheme.call_stats_font_title,NSLocalizedString("Audio", comment: ""))
		addSubview(audioTitle)
		audioTitle.matchParentSideBorders().alignParentTop(withMargin: margin_top).done()

		let audioStats = StyledLabel(VoipTheme.call_stats_font)
		
		audioStats.numberOfLines = 0
		addSubview(audioStats)
		audioStats.matchParentSideBorders().alignUnder(view: audioTitle).done()
		
		let videoTitle = StyledLabel(VoipTheme.call_stats_font_title,NSLocalizedString("Video", comment: ""))
		addSubview(videoTitle)
		videoTitle.alignUnder(view: audioStats, withMargin:audio_video_margin).matchParentSideBorders().done()
		
		let videoStats = StyledLabel(VoipTheme.call_stats_font)
		
		videoStats.numberOfLines = 0
		addSubview(videoStats)
		videoStats.matchParentSideBorders().alignUnder(view: videoTitle).done()
		
		model.isVideoEnabled.readCurrentAndObserve { (video) in
			videoTitle.isHidden  = video != true
			videoStats.isHidden  = video != true
		}
		
		model.statsUpdated.readCurrentAndObserve { (updated) in
			var stats = ""
			model.audioStats.forEach {
				stats += "\n\($0.getTypeTitle())\($0.value.value ?? "n/a")"
			}
			audioStats.text = stats
			stats = ""
			model.videoStats.forEach {
				stats += "\n\($0.getTypeTitle())\($0.value.value ?? "n/a")"
			}
			videoStats.text = stats
		}
				
	}
	
	required init?(coder: NSCoder) {
		super.init(coder: coder)
	}
	
}



