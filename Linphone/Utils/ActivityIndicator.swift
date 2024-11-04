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

struct ActivityIndicator: View {
	
	let style = StrokeStyle(lineWidth: 3, lineCap: .round)
	@State var animate = false
	let color: Color
	
	var body: some View {
		ZStack {
			Circle()
				.trim(from: 0, to: 0.7)
				.stroke(
					AngularGradient(gradient: .init(colors: [color, color.opacity(0.5)]), center: .center), style: style)
				.rotationEffect(Angle(degrees: animate ? 360: 0))
				.animation(Animation.linear(duration: 0.7).repeatForever(autoreverses: false), value: UUID())
		}.onAppear {
			self.animate.toggle()
		}
	}
}

#Preview {
	ActivityIndicator(color: .white)
}
