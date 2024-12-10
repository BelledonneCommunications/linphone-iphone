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

struct MediaEncryptedSheetBottomSheet: View {
	@Environment(\.dismiss) private var dismiss
	
	@ObservedObject var callViewModel: CallViewModel
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	@State private var orientation = UIDevice.current.orientation
	
	@Binding var mediaEncryptedSheet: Bool
	
	var body: some View {
		VStack {
			if idiom != .pad && (orientation == .landscapeLeft
								 || orientation == .landscapeRight
								 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height) {
				Spacer()
				HStack {
					Spacer()
					Button("dialog_close") {
						mediaEncryptedSheet = false
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
			
			Text("call_stats_media_encryption_title")
				.default_text_style_white_600(styleSize: 15)
				.padding(.top, 10)
			
			Spacer()
			
			Text(callViewModel.callMediaEncryptionModel.mediaEncryption)
				.default_text_style_white(styleSize: 15)
			
			Spacer()
			
			Text(callViewModel.callMediaEncryptionModel.zrtpCipher)
				.default_text_style_white(styleSize: 15)
			
			Spacer()
			
			Text(callViewModel.callMediaEncryptionModel.zrtpKeyAgreement)
				.default_text_style_white(styleSize: 15)
			
			Spacer()
			
			Text(callViewModel.callMediaEncryptionModel.zrtpHash)
				.default_text_style_white(styleSize: 15)
			
			Spacer()
			
			Text(callViewModel.callMediaEncryptionModel.zrtpAuthTag)
				.default_text_style_white(styleSize: 15)
			
			Spacer()
			
			Text(callViewModel.callMediaEncryptionModel.zrtpAuthSas)
				.default_text_style_white(styleSize: 15)
				.padding(.bottom, 10)
			
			Spacer()
			
			Button(action: {
				callViewModel.showZrtpSasDialogIfPossible()
				mediaEncryptedSheet = false
				dismiss()
			}, label: {
				Text("call_do_zrtp_sas_validation_again")
					.default_text_style_white_600(styleSize: 20)
					.frame(height: 35)
					.frame(maxWidth: .infinity)
			})
			.padding(.horizontal, 20)
			.padding(.vertical, 10)
			.background(Color.orangeMain500)
			.cornerRadius(60)
			.padding(.bottom)
			.padding(.horizontal, 10)
		}
		.background(Color.gray600)
	}
}
