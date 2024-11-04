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
import linphonesw
import UniformTypeIdentifiers

struct SideMenuEntry: View {
	var iconName: String
	var title: String
	var body: some View {
		HStack {
			Image(iconName)
				.renderingMode(.template)
				.resizable()
				.foregroundStyle(Color.grayMain2c500)
				.frame(width: 20, height: 20)
			Text(NSLocalizedString(title, comment: title))
				.default_text_style_600(styleSize: 13)
				.padding(.leading, 4)
			Spacer()
			Image("caret-right")
				.renderingMode(.template)
				.resizable()
				.foregroundStyle(Color.grayMain2c600)
				.frame(width: 20, height: 20)
		}
		.background()
	}
}

#Preview {
	SideMenuEntry(
		iconName: "linphone",
		title: "some text"
	)
}
