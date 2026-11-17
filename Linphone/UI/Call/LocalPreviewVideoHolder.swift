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

import SwiftUI
import linphonesw

/// Displays the local camera preview, linked to the Core through `nativePreviewWindow`.
///
/// When the call layout changes (for example from active speaker to mosaic), SwiftUI may
/// attach the new holder's view before the old holder's view is dismantled. If the old holder
/// unconditionally cleared `nativePreviewWindow`, it would remove the window the new layout
/// had just set, leaving the local video disabled while it is still being sent to the remote
/// participants. The window is therefore only cleared when the dismantled view is still the
/// one the Core renders into.
///
/// The preview fades in on appearance: the SDK's video surface resizes over its first frames
/// when attached to a new window, and the fade makes that transient re-layout discreet
/// instead of showing the video grow into place.
struct LocalPreviewVideoHolder: View {
	@State private var fadeIn = false

	var body: some View {
		NativePreviewHolder()
			.opacity(fadeIn ? 1 : 0)
			.onAppear {
				withAnimation(.easeIn(duration: 0.2)) {
					fadeIn = true
				}
			}
	}
}

private struct NativePreviewHolder: UIViewRepresentable {
	// Accessed only from the Core queue.
	private weak static var currentPreviewView: UIView?

	func makeUIView(context: Context) -> UIView {
		let view = UIView()
		CoreContext.shared.doOnCoreQueue { core in
			NativePreviewHolder.currentPreviewView = view
			core.nativePreviewWindow = view
		}
		return view
	}

	func updateUIView(_ uiView: UIView, context: Context) {}

	static func dismantleUIView(_ uiView: UIView, coordinator: ()) {
		CoreContext.shared.doOnCoreQueue { core in
			if NativePreviewHolder.currentPreviewView === uiView {
				NativePreviewHolder.currentPreviewView = nil
				core.nativePreviewWindow = nil
			}
		}
	}
}
