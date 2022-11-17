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
import UIKit

@objc class VoipTheme : NSObject { // Names & values replicated from Android
	
	// Voip Colors
	static let voip_gray_blue_color = UIColor(hex:"#798791")
	static let voip_light_gray = UIColor(hex:"#D0D8DE")
	@objc static let voip_dark_gray = UIColor(hex:"#4B5964")
	@objc static let voip_gray = UIColor(hex:"#96A5B1")
	static let voip_gray_background = UIColor(hex:"#AFAFAF")
	static let voip_call_record_background = UIColor(hex:"#EBEBEB")
	static let voip_calls_list_inactive_background = UIColor(hex:"#F0F1F2")
	static let voip_translucent_popup_background = UIColor(hex:"#A64B5964")
	static let voip_translucent_popup_alt_background = UIColor(hex:"#E64B5964")
	static let voip_numpad_background = UIColor(hex:"#E4E4E4")
	static let voip_contact_avatar_background_alt = UIColor(hex:"#AFAFAF")
	static let voip_contact_avatar_calls_list = UIColor(hex:"#A1A1A1")
	static let voip_conference_participant_paused_background = UIColor(hex:"#303030")
	static let voip_drawable_color = UIColor(hex:"#A6B2BC")
	static let voip_dark_color = UIColor(hex:"#252E35")
	static let voip_dark_color2 = UIColor(hex:"#3F464B")
	static let voip_dark_color3 = UIColor(hex:"#475663")
	static let voip_dark_color4 = UIColor(hex:"#2D3841")
	
	// General colors (used by VoIP)
	
	@objc static let primary_color = UIColor(hex:"#ff5e00")
	static let primary_dark_color = UIColor(hex:"#e65000")
	static let green_color = UIColor(hex:"#96c11f")
	static let dark_green_color = UIColor(hex:"#7d9f21")
	@objc static let toolbar_color = UIColor(hex:"#e1e1e1")
	static let form_field_gray_background = UIColor(hex:"#F7F7F7")
	static let light_grey_color = UIColor(hex:"#c4c4c4")
	static let header_background_color = UIColor(hex:"#f3f3f3")
	static let dark_grey_color = UIColor(hex:"#444444")
	static let voip_conference_invite_out = UIColor(hex:"ffeee5")
	static let voip_conference_invite_in = header_background_color
	static let voip_conference_updated = UIColor(hex:"#EFAE00")
	static let  voip_conference_cancelled_bg_color = UIColor(hex:"#FFE6E6")
	static let  voip_dark_color5 = UIColor(hex:"#353B3F")

	
	
	
	// Light / Dark variations
	static let voipBackgroundColor = LightDarkColor(voip_gray_blue_color,voip_dark_color)
	@objc static let voipBackgroundBWColor = LightDarkColor(.white,voip_dark_color)
	@objc static let backgroundWhiteBlack = LightDarkColor(.white,.black)
	static let voipParticipantBackgroundColor = LightDarkColor(voip_gray_background,voip_dark_color2)
	static let voipParticipantMeBackgroundColor = LightDarkColor(voip_dark_color3,voip_dark_color3)
	static let voipExtraButtonsBackgroundColor = LightDarkColor(voip_gray,voip_dark_color3)
	@objc static let voipToolbarBackgroundColor = LightDarkColor(toolbar_color,voip_dark_color4)
	static let voipDrawableColor = LightDarkColor(voip_dark_gray,.white)
	static let voipDrawableColorHighlighted = LightDarkColor(voip_gray,voip_gray)
	static let voipTextColor = LightDarkColor(voip_dark_gray,.white)
	static let voipFormBackgroundColor = LightDarkColor(form_field_gray_background,voip_dark_color4)
	static let voipFormFieldBackgroundColor = LightDarkColor(light_grey_color,voip_dark_color4)
	static let voipFormDisabledFieldBackgroundColor = LightDarkColor(header_background_color,voip_dark_color4)
	static let primarySubtextLightColor = LightDarkColor(light_grey_color,toolbar_color)
	static let primaryTextColor = LightDarkColor(dark_grey_color,.white)
	static let backgroundColor3 = LightDarkColor(voip_light_gray,voip_dark_color5)
	static let backgroundColor4 = LightDarkColor(header_background_color,voip_dark_color5)
	static let separatorColor = LightDarkColor(light_grey_color,.white)
	static let chatBubbleBGColor = LightDarkColor(voip_light_gray, voip_dark_color5)

	
	// Text styles
	static let fontName = "Roboto"
	static let call_header_title = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .center, font: fontName+"-Bold", size: 18.0)
	static let call_header_subtitle = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .center, font: fontName+"-Regular", size: 14.0)
	static let call_generated_avatar_large = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: true, align: .center, font: fontName+"-Regular", size: 53.0)
	static let call_generated_avatar_medium = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: true, align: .center, font: fontName+"-Regular", size: 27.0)
	static let call_generated_avatar_small = TextStyle(fgColor: LightDarkColor(.white,voip_dark_gray), bgColor: LightDarkColor(.clear,.clear), allCaps: true, align: .center, font: fontName+"-Bold", size: 25.0)

	static let dtmf_label = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .center, font: fontName+"-Regular", size: 30.0)
	static let call_remote_name = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .left, font: fontName+"-Regular", size: 18.0)
	static let call_remote_recording = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .center, font: fontName+"-Regular", size: 16.0)
	static let call_or_conference_title = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .center, font: fontName+"-Bold", size: 30.0)
	static let call_or_conference_subtitle = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .center, font: fontName+"-Bold", size: 20.0)
	static let basic_popup_title = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .center, font: fontName+"-Regular", size: 21.0)
	static let form_button_bold = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: true, align: .center, font: fontName+"-Bold", size: 17.0)
	static let form_button_light = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: true, align: .center, font: fontName+"-Regular", size: 17.0)

	static let call_display_name_duration = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .left, font: fontName+"-Regular", size: 17.0)
	static let call_sip_address = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .left, font: fontName+"-Regular", size: 14.0)
	static let voip_extra_button = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .center, font: fontName+"-Regular", size: 12.0)
	static let unread_count_font = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .center, font: fontName+"-Regular", size: 11.0)
	static let call_stats_font = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .center, font: fontName+"-Regular", size: 12.0)
	static let call_stats_font_title = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .center, font: fontName+"-Regular", size: 18.0)
	static let calls_list_header_font = TextStyle(fgColor: voipTextColor, bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .center, font: fontName+"-Regular", size: 20.0)
	static let navigation_header_font = TextStyle(fgColor: LightDarkColor(primary_color,primary_color), bgColor: LightDarkColor(.clear,.clear), allCaps: true, align: .center, font: fontName+"-Bold", size: 27.0)

	static let call_list_active_name_font = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .center, font: fontName+"-Regular", size: 18.0)
	static let call_list_active_sip_uri_font = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .center, font: fontName+"-Regular", size: 12.0)

	static let call_list_name_font = TextStyle(fgColor: voipTextColor, bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .center, font: fontName+"-Regular", size: 18.0)
	static let call_list_sip_uri_font = TextStyle(fgColor: voipTextColor, bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .center, font: fontName+"-Regular", size: 12.0)

	static let call_context_menu_item_font = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: true, align: .left, font: fontName+"-Bold", size: 16.0)
	
	
	static let conference_participant_admin_label = TextStyle(fgColor: primarySubtextLightColor, bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .left, font: fontName+"-Bold", size: 13.0)
	static let conference_participant_name_font = TextStyle(fgColor: LightDarkColor(dark_grey_color,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .left, font: fontName+"-Regular", size: 18.0)
	static let conference_participant_sip_uri_font = TextStyle(fgColor: LightDarkColor(primary_color,primary_color), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .left, font: fontName+"-Regular", size: 12.0)
	static let conference_participant_name_font_grid = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .left, font: fontName+"-Bold", size: 15.0)
	static let conference_participant_name_font_as = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .left, font: fontName+"-Bold", size: 12.0)
	static let conference_participant_name_font_audio_only = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .left, font: fontName, size: 14.0)

	static let conference_mode_title = TextStyle(fgColor: LightDarkColor(dark_grey_color,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .left, font: fontName+"-Regular", size: 17.0)
	static let conference_mode_title_selected = conference_mode_title.boldEd()
	static let conference_scheduling_font = TextStyle(fgColor: voipTextColor, bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .left, font: fontName+"-Regular", size: 17.0)
	static let conference_invite_desc_font = TextStyle(fgColor: LightDarkColor(dark_grey_color,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .left, font: fontName+"-Regular", size: 14.0)
	static let conference_invite_desc_title_font = TextStyle(fgColor: LightDarkColor(voip_dark_gray,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .left, font: fontName+"-Bold", size: 14.0)
	static let conference_invite_subject_font = TextStyle(fgColor: LightDarkColor(voip_dark_gray,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .left, font: fontName+"-Bold", size: 14.0)
	static let conference_invite_title_font = TextStyle(fgColor: LightDarkColor(dark_grey_color,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .left, font: fontName+"-Bold", size: 16.0)
	static let conference_cancelled_title_font = TextStyle(fgColor: LightDarkColor(.red,.red), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .left, font: fontName+"-Bold", size: 16.0)
	static let conference_updated_title_font = TextStyle(fgColor: LightDarkColor(voip_conference_updated,voip_conference_updated), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .left, font: fontName+"-Bold", size: 16.0)

	static let conference_preview_subject_font = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .left, font: fontName+"-Regular", size: 24.0)
	static let conference_waiting_room_no_video_font = TextStyle(fgColor: LightDarkColor(.white,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .center, font: fontName+"-Regular", size: 16.0)
		
	static let empty_list_font = TextStyle(fgColor: primaryTextColor, bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .center, font: fontName+"-Regular", size: 18.0)
	static let conf_list_filter_button_font = TextStyle(fgColor: LightDarkColor(.black,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .center, font: fontName+"-Regular", size: 14.0)
	static let conference_list_subject_font = TextStyle(fgColor: LightDarkColor(voip_dark_gray,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .left, font: fontName+"-Bold", size: 18.0)
	static let conference_list_address_desc_font = TextStyle(fgColor: LightDarkColor(voip_dark_gray,.white), bgColor: LightDarkColor(.clear,.clear), allCaps: false, align: .left, font: fontName+"-Regular", size: 18.0)

	
	// Buttons Background (State colors)
	
	static let button_background =  [
		UIButton.State.normal.rawValue : LightDarkColor(voip_gray,voip_gray),
		UIButton.State.highlighted.rawValue : LightDarkColor(voip_dark_gray,voip_dark_gray),
		UIButton.State.selected.union(.highlighted).rawValue : LightDarkColor(voip_dark_gray,voip_dark_gray),
		UIButton.State.disabled.rawValue : LightDarkColor(voip_light_gray,voip_light_gray)
   ]
	
	static let button_background_reverse =  [
		UIButton.State.normal.rawValue : LightDarkColor(voip_dark_gray,voip_dark_gray),
		UIButton.State.highlighted.rawValue : LightDarkColor(voip_gray,voip_gray),
		UIButton.State.selected.union(.highlighted).rawValue : LightDarkColor(voip_gray,voip_gray),
		UIButton.State.disabled.rawValue : LightDarkColor(voip_light_gray,voip_light_gray)
   ]
	
	static let button_call_recording_background =  [
		UIButton.State.normal.rawValue : LightDarkColor(voip_call_record_background,voip_call_record_background),
		UIButton.State.selected.rawValue : LightDarkColor(primary_color,primary_color),
		UIButton.State.disabled.rawValue : LightDarkColor(voip_light_gray,voip_light_gray)
   ]
	
	static let button_toggle_background =  [
		UIButton.State.normal.rawValue : LightDarkColor(voip_gray,voip_gray),
		UIButton.State.selected.rawValue : LightDarkColor(primary_color,primary_color),
		UIButton.State.highlighted.rawValue : LightDarkColor(voip_dark_gray,voip_dark_gray),
		UIButton.State.disabled.rawValue : LightDarkColor(voip_light_gray,voip_light_gray)
   ]
	
	static let button_toggle_background_reverse =  [
		UIButton.State.normal.rawValue : LightDarkColor(voip_dark_gray,voip_dark_gray),
		UIButton.State.selected.rawValue : LightDarkColor(primary_color,primary_color),
		UIButton.State.highlighted.rawValue : LightDarkColor(voip_gray,voip_gray),
		UIButton.State.disabled.rawValue : LightDarkColor(voip_light_gray,voip_light_gray)
   ]
	
	static let primary_colors_background =  [
		UIButton.State.normal.rawValue : LightDarkColor(primary_color,primary_color),
		UIButton.State.highlighted.rawValue : LightDarkColor(primary_dark_color,primary_dark_color),
	]
	
	static let button_green_background =  [
		UIButton.State.normal.rawValue : LightDarkColor(green_color,green_color),
		UIButton.State.highlighted.rawValue : LightDarkColor(primary_color,primary_color),
	]
	
	static let primary_colors_background_gray =  [
		UIButton.State.normal.rawValue : LightDarkColor(voip_gray,voip_gray),
		UIButton.State.highlighted.rawValue : LightDarkColor(voip_dark_gray,voip_dark_gray),
	]
		
	static let numpad_digit_background = [
		UIButton.State.normal.rawValue : LightDarkColor(voip_numpad_background,voip_numpad_background),
		UIButton.State.highlighted.rawValue : LightDarkColor(voip_gray_blue_color,voip_gray_blue_color)
	]
	
	static let button_round_background =  [
		UIButton.State.normal.rawValue : LightDarkColor(primary_color,primary_color),
		UIButton.State.highlighted.rawValue : LightDarkColor(dark_grey_color,dark_grey_color),
		UIButton.State.disabled.rawValue : LightDarkColor(voip_light_gray,voip_light_gray)
	]
	
	static let button_call_context_menu_background =  [
		UIButton.State.normal.rawValue : LightDarkColor(voip_gray,voip_gray),
		UIButton.State.highlighted.rawValue : LightDarkColor(primary_color,primary_color),
	]
	
	static let button_conference_list_filter =  [
		UIButton.State.normal.rawValue : LightDarkColor(light_grey_color,dark_grey_color),
		UIButton.State.selected.rawValue : LightDarkColor(primary_color.withAlphaComponent(0.24),primary_color.withAlphaComponent(0.24)),
	]
	
	// Buttons Icons (State colors) + Background colors

	static let call_terminate = ButtonTheme(
		tintableStateIcons:[UIButton.State.normal.rawValue : TintableIcon(name: "voip_hangup",tintColor: LightDarkColor(.white,.white))],
		backgroundStateColors: [
			UIButton.State.normal.rawValue : LightDarkColor(primary_color,primary_color),
			UIButton.State.highlighted.rawValue : LightDarkColor(primary_dark_color,primary_dark_color)
	])
	
	static let call_record = ButtonTheme(
		tintableStateIcons:[
			UIButton.State.normal.rawValue : TintableIcon(name: "voip_call_record",tintColor: LightDarkColor(.white,.white)),
		],
		backgroundStateColors: button_toggle_background)
	
	static let call_pause = ButtonTheme(
		tintableStateIcons:[
			UIButton.State.normal.rawValue : TintableIcon(name: "voip_pause",tintColor: LightDarkColor(.white,.white)),
		],
		backgroundStateColors: button_toggle_background)
		
	static let call_accept = ButtonTheme(
		tintableStateIcons:[UIButton.State.normal.rawValue : TintableIcon(name: "voip_call",tintColor: LightDarkColor(.white,.white))],
		backgroundStateColors: [
			UIButton.State.normal.rawValue : LightDarkColor(green_color,green_color),
			UIButton.State.highlighted.rawValue : LightDarkColor(dark_green_color,dark_green_color)
	])
	
	static let call_mute = ButtonTheme(
		tintableStateIcons:[
			UIButton.State.normal.rawValue : TintableIcon(name: "voip_micro_on",tintColor: LightDarkColor(.white,.white)),
			UIButton.State.selected.rawValue : TintableIcon(name: "voip_micro_off",tintColor: LightDarkColor(.white,.white)),
		],
		backgroundStateColors: button_background_reverse)
	
	static let call_speaker = ButtonTheme(
		tintableStateIcons:[
			UIButton.State.normal.rawValue : TintableIcon(name: "voip_speaker_off",tintColor: LightDarkColor(.white,.white)),
			UIButton.State.selected.rawValue : TintableIcon(name: "voip_speaker_on",tintColor: LightDarkColor(.white,.white)),
		],
		backgroundStateColors: button_background_reverse)
	
	static let call_audio_route = ButtonTheme(
		tintableStateIcons:[
			UIButton.State.normal.rawValue : TintableIcon(name: "voip_audio_routes",tintColor: LightDarkColor(.white,.white)),
		],
		backgroundStateColors: button_toggle_background_reverse)
	
	
	static let call_video = ButtonTheme(
		tintableStateIcons:[
			UIButton.State.normal.rawValue : TintableIcon(name: "voip_camera_off",tintColor: LightDarkColor(.white,.white)),
			UIButton.State.selected.rawValue : TintableIcon(name: "voip_camera_on",tintColor: LightDarkColor(.white,.white)),
		],
		backgroundStateColors: button_background_reverse)
	
	static let call_numpad = ButtonTheme(
		tintableStateIcons:[
			UIButton.State.normal.rawValue : TintableIcon(name: "voip_call_numpad",tintColor: LightDarkColor(.white,.white)),
			UIButton.State.highlighted.rawValue : TintableIcon(name: "voip_call_numpad",tintColor: LightDarkColor(voip_dark_gray,voip_dark_gray)),
			UIButton.State.disabled.rawValue : TintableIcon(name: "voip_call_numpad",tintColor: LightDarkColor(voip_light_gray,voip_light_gray)),
		],
		backgroundStateColors: button_background)
	
	// Waiting room layout picker
	
	static let conf_waiting_room_layout_picker = ButtonTheme(
		tintableStateIcons:[:],
		backgroundStateColors: button_toggle_background_reverse)
	
	// AUuio routes
	
	static let route_bluetooth = ButtonTheme(
		tintableStateIcons:[
			UIButton.State.normal.rawValue : TintableIcon(name: "voip_bluetooth",tintColor: LightDarkColor(.white,.white)),
		],
		backgroundStateColors: button_toggle_background_reverse)
	
	static let route_earpiece = ButtonTheme(
		tintableStateIcons:[
			UIButton.State.normal.rawValue : TintableIcon(name: "voip_earpiece",tintColor: LightDarkColor(.white,.white)),
		],
		backgroundStateColors: button_toggle_background_reverse)
	
	static let route_speaker = ButtonTheme(
		tintableStateIcons:[
			UIButton.State.normal.rawValue : TintableIcon(name: "voip_speaker_on",tintColor: LightDarkColor(.white,.white)),
		],
		backgroundStateColors: button_toggle_background_reverse)
	
	
	
	static let call_more = ButtonTheme(
		tintableStateIcons:[
			UIButton.State.normal.rawValue : TintableIcon(name: "voip_call_more",tintColor: LightDarkColor(.white,.white))
		],
		backgroundStateColors: button_background)
	
	
	static let voip_cancel = ButtonTheme(
		tintableStateIcons:[
			UIButton.State.normal.rawValue : TintableIcon(name: "voip_cancel",tintColor: voipDrawableColor),
			UIButton.State.highlighted.rawValue : TintableIcon(name: "voip_cancel",tintColor: voipDrawableColorHighlighted)
		],
		backgroundStateColors: [UIButton.State.normal.rawValue : LightDarkColor(.clear,.clear)])
	
	
	static let voip_cancel_light = ButtonTheme(
		tintableStateIcons:[
			UIButton.State.normal.rawValue : TintableIcon(name: "voip_cancel",tintColor: LightDarkColor(voip_gray,voip_gray)),
			UIButton.State.highlighted.rawValue : TintableIcon(name: "voip_cancel",tintColor: LightDarkColor(voip_dark_gray,voip_dark_gray))
		],
		backgroundStateColors: [UIButton.State.normal.rawValue : LightDarkColor(.clear,.clear)])
	
	static let voip_edit = ButtonTheme(
		tintableStateIcons:[
			UIButton.State.normal.rawValue : TintableIcon(name: "voip_edit",tintColor: LightDarkColor(dark_grey_color,.white)),
			UIButton.State.highlighted.rawValue : TintableIcon(name: "voip_edit",tintColor: voipDrawableColorHighlighted)
		],
		backgroundStateColors: [UIButton.State.normal.rawValue : LightDarkColor(.clear,.clear)])
	
	static let radio_button = ButtonTheme(
		tintableStateIcons:[
			UIButton.State.normal.rawValue : TintableIcon(name: "voip_radio_off",tintColor: LightDarkColor(dark_grey_color,.white)),
			UIButton.State.selected.rawValue : TintableIcon(name: "voip_radio_on",tintColor: LightDarkColor(primary_color,primary_color))
		],
		backgroundStateColors: [UIButton.State.normal.rawValue : LightDarkColor(.clear,.clear)])
	
	static let voip_call_list_active_menu = ButtonTheme(
		tintableStateIcons:[
			UIButton.State.normal.rawValue : TintableIcon(name: "voip_call_list_menu",tintColor: LightDarkColor(.white,.white)),
			UIButton.State.highlighted.rawValue : TintableIcon(name: "voip_call_list_menu",tintColor: voipDrawableColorHighlighted)
		],
		backgroundStateColors: [UIButton.State.normal.rawValue : LightDarkColor(.clear,.clear)])
	
	static let voip_call_list_menu = ButtonTheme(
		tintableStateIcons:[
			UIButton.State.normal.rawValue : TintableIcon(name: "voip_call_list_menu",tintColor: voipTextColor),
			UIButton.State.highlighted.rawValue : TintableIcon(name: "voip_call_list_menu",tintColor: voipDrawableColorHighlighted)
		],
		backgroundStateColors: [UIButton.State.normal.rawValue : LightDarkColor(.clear,.clear)])
	
	
	static func call_action(_ iconName:String) -> ButtonTheme {
		return ButtonTheme(
			tintableStateIcons:[
			 UIButton.State.normal.rawValue : TintableIcon(name: iconName,tintColor: LightDarkColor(.white,.white)),
			 UIButton.State.highlighted.rawValue : TintableIcon(name: iconName,tintColor: LightDarkColor(voip_dark_gray,voip_dark_gray)),
			 UIButton.State.disabled.rawValue : TintableIcon(name: iconName,tintColor: LightDarkColor(voip_light_gray,voip_light_gray)),
		 ],
			backgroundStateColors: [:])
	}
	
	static let call_add = ButtonTheme(
		tintableStateIcons:[UIButton.State.normal.rawValue : TintableIcon(name: "voip_call_add",tintColor: LightDarkColor(.white,.white))],
		backgroundStateColors: button_round_background)

	static let call_merge = ButtonTheme(
		tintableStateIcons:[UIButton.State.normal.rawValue : TintableIcon(name: "voip_merge_calls",tintColor: LightDarkColor(.white,.white))],
		backgroundStateColors: button_round_background)

	// Navigation
	
	static func nav_button(_ iconName:String) -> ButtonTheme {
		return ButtonTheme(
		tintableStateIcons:[
			UIButton.State.normal.rawValue : TintableIcon(name: iconName,tintColor: LightDarkColor(voip_dark_gray,.white)),
			UIButton.State.highlighted.rawValue : TintableIcon(name: iconName,tintColor: LightDarkColor(primary_color,primary_color)),
			UIButton.State.disabled.rawValue : TintableIcon(name: iconName,tintColor: LightDarkColor(light_grey_color,.white)),
		],
		backgroundStateColors: [:])
	}
	
	// Conference scheduling
	static func scheduled_conference_action(_ iconName:String) -> ButtonTheme {
		return ButtonTheme(
		tintableStateIcons:[UIButton.State.normal.rawValue : TintableIcon(name: iconName,tintColor: LightDarkColor(.white,.white))],
		backgroundStateColors: button_background)
	}
	
	static let conference_info_button = [
		UIButton.State.normal.rawValue : TintableIcon(name: "voip_info",tintColor: LightDarkColor(voip_drawable_color,voip_drawable_color)),
		UIButton.State.selected.rawValue : TintableIcon(name: "voip_info",tintColor: LightDarkColor(primary_color,primary_color)),
	   ]
	
	static let conference_create_button = [
		UIButton.State.normal.rawValue : TintableIcon(name: "voip_conference_new",tintColor:  LightDarkColor(.darkGray,.white)),
		UIButton.State.highlighted.rawValue : TintableIcon(name: "voip_conference_new",tintColor: LightDarkColor(primary_color,primary_color)),
		UIButton.State.disabled.rawValue : TintableIcon(name: "voip_conference_new",tintColor: LightDarkColor(voip_light_gray,voip_light_gray)),
	   ]
	
	static let generic_delete_button = [
		UIButton.State.normal.rawValue : TintableIcon(name: "delete_default",tintColor:  LightDarkColor(.darkGray,.white)),
		UIButton.State.highlighted.rawValue : TintableIcon(name: "delete_default",tintColor: LightDarkColor(primary_color,primary_color)),
		UIButton.State.disabled.rawValue : TintableIcon(name: "delete_disabled",tintColor: LightDarkColor(voip_light_gray,.white)),
		 ]
	
	static let generic_back = [
		UIButton.State.normal.rawValue : TintableIcon(name: "back_default",tintColor:  LightDarkColor(.darkGray,.white)),
		UIButton.State.highlighted.rawValue : TintableIcon(name: "back_default",tintColor: LightDarkColor(primary_color,primary_color)),
		UIButton.State.disabled.rawValue : TintableIcon(name: "back_default",tintColor: LightDarkColor(voip_light_gray,voip_light_gray)),
		 ]
	
	static let generic_cancel = [
		UIButton.State.normal.rawValue : TintableIcon(name: "cancel_edit_default",tintColor:  LightDarkColor(.darkGray,.white)),
		 ]

}


