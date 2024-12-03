/*
 * Copyright (c) 2010-2024 Belledonne Communications SARL.
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

struct SideMenuShortcut: View {
	@ObservedObject var shortcutModel: ShortcutModel
	var body: some View {
		Link(destination: shortcutModel.linkUrl) {
			HStack {
				AsyncImage(url: shortcutModel.iconLinkUrl) { phase in
					if let image = phase.image {
						image
							.resizable()
							.aspectRatio(contentMode: .fit)
					} else if phase.error != nil {
						Image("link-break")
							.renderingMode(.template)
							.resizable()
					} else {
						ProgressView()
					}
				}
				.frame(width: 20, height: 20)
				Text(NSLocalizedString(shortcutModel.name, comment: shortcutModel.name))
					.default_text_style_600(styleSize: 13)
					.padding(.leading, 4)
				Spacer()
				Image("arrow-square-out")
					.renderingMode(.template)
					.resizable()
					.foregroundStyle(Color.grayMain2c600)
					.frame(width: 20, height: 20)
			}
			.background()
		}
	}
}
