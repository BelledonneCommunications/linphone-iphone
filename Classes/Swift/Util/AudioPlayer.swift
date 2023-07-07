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


class AudioPlayer {
	static let sharedModel = AudioPlayer()
	
	static var linphonePlayer : Player? = nil
	
	var fileChanged = MutableLiveData<String>()
	
	static func getSharedPlayer() -> Player?{
		return linphonePlayer
	}
	
	static func initSharedPlayer(){
		Log.i("[Voice Message] Creating shared player")
		
		let core = Core.getSwiftObject(cObject: LinphoneManager.getLc())
		do{
			if linphonePlayer?.userData == nil {
				linphonePlayer = try core.createLocalPlayer(soundCardName: CallManager.instance().getSpeakerSoundCard(), videoDisplayName: nil, windowId: nil)
			}
		}catch{
			Log.e(error.localizedDescription)
		}
	}
	
	static func startSharedPlayer(_ path: String?) {
		Log.i("[Voice Message] Starting shared player path = \(String(describing: path))")
		if ((linphonePlayer!.userData) != nil) {
			Log.i("[Voice Message] a play was requested (\(String(describing: path)), but there is already one going (\(String(describing: linphonePlayer?.userData))")
			let userInfo = [
				"path": linphonePlayer!.userData
			]
			NotificationCenter.default.post(name: NSNotification.Name(rawValue: "LinphoneVoiceMessagePlayerEOF"), object: nil, userInfo: userInfo as [AnyHashable : Any])
		}
		CallManager.instance().changeRouteToSpeaker()
		do{
			try linphonePlayer?.open(filename: path!)
			try linphonePlayer?.start()
		}catch{
			Log.e(error.localizedDescription)
		}
	}
	
	static func cancelVoiceRecordingVM(_ voiceRecorder: Recorder?) {
		voiceRecorder?.close()
		if let recordingFile = voiceRecorder?.file {
			AppManager.removeFile(file: String(utf8String: recordingFile)!)
		}
	}
	
	static func stopSharedPlayer() {
		Log.i("[Voice Message] Stopping shared player path = \(String(describing: linphonePlayer?.userData))")
		do{
			try linphonePlayer?.pause()
			try linphonePlayer?.seek(timeMs: 0)
			//linphonePlayer?.close()
			linphonePlayer?.userData = nil
		}catch{
			Log.e(error.localizedDescription)
		}
	}
}
