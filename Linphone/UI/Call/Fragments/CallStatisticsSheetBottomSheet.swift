/*
 * Copyright (c) 2010-2023 Belledonne Communications SARL.
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

struct CallStatisticsSheetBottomSheet: View {
	@Environment(\.dismiss) private var dismiss
	
	@ObservedObject var callViewModel: CallViewModel
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	@State private var orientation = UIDevice.current.orientation
	
	@Binding var callStatisticsSheet: Bool
	
	var body: some View {
		VStack {
			if idiom != .pad && (orientation == .landscapeLeft
								 || orientation == .landscapeRight
								 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height) {
				Spacer()
				HStack {
					Spacer()
					Button("dialog_close") {
						callStatisticsSheet = false
						dismiss()
					}
				}
				.padding(.trailing)
			} else {
				Capsule()
					.fill(Color.grayMain2c300)
					.frame(width: 75, height: 5)
					.padding(15)
			}
			
			Text("call_stats_audio_title")
				.default_text_style_white_600(styleSize: 15)
				.padding(.top, 10)
			
			Spacer()
			
			Text(callViewModel.callStatsModel.audioCodec)
				.default_text_style_white(styleSize: 15)
			
			Spacer()
			
			Text(callViewModel.callStatsModel.audioBandwidth)
				.default_text_style_white(styleSize: 15)
			
			Spacer()
			
			Text(callViewModel.callStatsModel.audioLossRate)
				   .default_text_style_white(styleSize: 15)
			   
			Spacer()
			
			Text(callViewModel.callStatsModel.audioJitterBufferSize)
				   .default_text_style_white(styleSize: 15)
			   
			Spacer()
			
			if callViewModel.callStatsModel.isVideoEnabled {
				Text("call_stats_video_title")
					.default_text_style_white_600(styleSize: 15)
					.padding(.top, 10)
				
				Spacer()
				
				Text(callViewModel.callStatsModel.videoCodec)
					.default_text_style_white(styleSize: 15)
				
				Spacer()
				
				Text(callViewModel.callStatsModel.videoBandwidth)
					.default_text_style_white(styleSize: 15)
				
				Spacer()
				
				Text(callViewModel.callStatsModel.videoLossRate)
					.default_text_style_white(styleSize: 15)
				
				Spacer()
				
				Text(callViewModel.callStatsModel.videoResolution)
					.default_text_style_white(styleSize: 15)
				
				Spacer()
				
				Text(callViewModel.callStatsModel.videoFps)
					.default_text_style_white(styleSize: 15)
				
				Spacer()
			}
		}
		.frame(maxWidth: .infinity)
		.background(Color.gray600)
	}
}
