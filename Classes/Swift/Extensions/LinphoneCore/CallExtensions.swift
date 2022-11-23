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

extension Call {
	func answerVideoUpdateRequest(accept:Bool) {
		guard let params = try?core? .createCallParams(call: self) else {
			Log.i("[Call] \(self) unable to answerVideoUpdateRequest : could not create params ")
			return
		}
		if (accept) {
			params.videoEnabled = true
			core?.videoCaptureEnabled = true
			core?.videoDisplayEnabled = true
		} else {
			params.videoEnabled = false
		}
		try?acceptUpdate(params: params)
	}
}

extension Call : CustomStringConvertible {
	public var description: String {
		return "<Call-ID: \(callLog?.callId ?? "pending") pointer:\(Unmanaged.passUnretained(self).toOpaque()) is conference:\(conference != nil) >"
 }
}

