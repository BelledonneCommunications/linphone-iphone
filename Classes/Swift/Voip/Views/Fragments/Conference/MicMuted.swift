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

class MicMuted : UIImageView {
	required init?(coder: NSCoder) {
		super.init(coder: coder)
	}
	init (_ diameter: Int) {
		super.init(image: UIImage(named: "voip_micro_off")?.tinted(with: .white))
		layer.cornerRadius = CGFloat(diameter/2)
		clipsToBounds = true
		backgroundColor = VoipTheme.voip_dark_gray
		square(diameter).done()
	}
}
