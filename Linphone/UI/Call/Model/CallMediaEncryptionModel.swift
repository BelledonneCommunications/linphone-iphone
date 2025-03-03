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
import SwiftUI

class CallMediaEncryptionModel: ObservableObject {
	var coreContext = CoreContext.shared
	
	@Published var mediaEncryption = ""

	@Published var isMediaEncryptionZrtp = false
	@Published var zrtpCipher = ""
	@Published var zrtpKeyAgreement = ""
	@Published var zrtpHash = ""
	@Published var zrtpAuthTag = ""
	@Published var zrtpAuthSas = ""
	
	func update(call: Call) {
		coreContext.doOnCoreQueue { _ in
			let stats = call.getStats(type: StreamType.Audio)
			if stats != nil {
				// ZRTP stats are only available when authentication token isn't null !
				if call.currentParams!.mediaEncryption == .ZRTP && call.authenticationToken != nil {
					let isMediaEncryptionZrtpTmp = true
					
					var mediaEncryptionTmp = ""
					if stats!.isZrtpKeyAgreementAlgoPostQuantum {
						mediaEncryptionTmp = "Media encryption: " + "Post Quantum ZRTP"
					} else {
						switch call.currentParams!.mediaEncryption {
						case .None:
							mediaEncryptionTmp = "Media encryption: " + "None"
						case .SRTP:
							mediaEncryptionTmp = "Media encryption: " + "SRTP"
						case .ZRTP:
							mediaEncryptionTmp = "Media encryption: " + "ZRTP"
						case .DTLS:
							mediaEncryptionTmp = "Media encryption: " + "DTLS"
						}
					}
					
					let zrtpCipherTmp = "Cipher algorithm: " + stats!.zrtpCipherAlgo
					
					let zrtpKeyAgreementTmp = "Key agreement algorithm: " + stats!.zrtpKeyAgreementAlgo
					
					let zrtpHashTmp = "Hash algorithm: " + stats!.zrtpHashAlgo
					
					let zrtpAuthTagTmp = "Authentication algorithm: " + stats!.zrtpAuthTagAlgo
					
					let zrtpAuthSasTmp = "SAS algorithm: " + stats!.zrtpSasAlgo
					
					DispatchQueue.main.async {
						self.isMediaEncryptionZrtp = isMediaEncryptionZrtpTmp
						
						self.mediaEncryption = mediaEncryptionTmp
						
						self.zrtpCipher = zrtpCipherTmp
						
						self.zrtpKeyAgreement = zrtpKeyAgreementTmp
						
						self.zrtpHash = zrtpHashTmp
						
						self.zrtpAuthTag = zrtpAuthTagTmp
						
						self.zrtpAuthSas = zrtpAuthSasTmp
					}
				} else {
					let mediaEncryptionTmp = "Media encryption: " + call.currentParams!.mediaEncryption.rawValue.description // call.currentParams.mediaEncryption
					
					DispatchQueue.main.async {
						self.mediaEncryption = mediaEncryptionTmp
					}
				}
				
			}
		}
	}
}
