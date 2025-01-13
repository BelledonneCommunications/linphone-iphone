/*
 * Copyright (c) 2010-2025 Belledonne Communications SARL.
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

import linphonesw
import SwiftUI
import AVKit

class PIPViewModel: NSObject, AVPictureInPictureControllerDelegate {
	static let TAG = "[PIPViewModel]"
	
	var pipController: AVPictureInPictureController?
	var pipVideoCallViewController: PictureInPictureVideoCallViewController?
	var pipRemoteVideoView = SampleBufferVideoCallView()
	var videoCallView = UIView()
	
	var callStateChangedDelegate: CallDelegate?
	
	func setupPiPViewController(remoteView: UIView) {
		Log.info("\(PIPViewModel.TAG) Setup PiPViewController")
		videoCallView = remoteView
		self.pipVideoCallViewController = PictureInPictureVideoCallViewController()
		pipRemoteVideoView = pipVideoCallViewController!.pipRemoteVideoView
		let pipContentSource = AVPictureInPictureController.ContentSource(
			activeVideoCallSourceView: videoCallView,
			contentViewController: pipVideoCallViewController!)
		pipController = AVPictureInPictureController(contentSource: pipContentSource)
		pipController?.delegate = self
		pipController?.canStartPictureInPictureAutomaticallyFromInline = true
		
		CoreContext.shared.doOnCoreQueue { core in
			if let call = core.currentCall {
				self.callStateChangedDelegate = CallDelegateStub(onStateChanged: { (call: Call, cstate: Call.State, _: String) in
					if CoreContext.shared.pipViewModel.pipController?.isPictureInPictureActive ?? false {
						if cstate == .End || cstate == .Error {
							Log.info("\(PIPViewModel.TAG) call state 'End' or 'Error' detected, stopping picture in picture")
							CoreContext.shared.pipViewModel.pipController?.stopPictureInPicture()
							self.callStateChangedDelegate = nil
						}
					}
				})
				call.addDelegate(delegate: self.callStateChangedDelegate!)
			}
		}
	}
	
	
	func pictureInPictureControllerWillStartPictureInPicture(_ pictureInPictureController: AVPictureInPictureController) {
		Log.info("\(PIPViewModel.TAG) pictureInPictureControllerWillStartPictureInPicture")
		self.pipVideoCallViewController?.matchVideoDimension()
		CoreContext.shared.doOnCoreQueue { core in
			core.nativeVideoWindow = self.pipRemoteVideoView
		}
	}
	
	func pictureInPictureControllerDidStopPictureInPicture(_ pictureInPictureController: AVPictureInPictureController) {
		Log.info("\(PIPViewModel.TAG) pictureInPictureControllerDidStopPictureInPicture")
		CoreContext.shared.doOnCoreQueue { core in
			core.nativeVideoWindow = self.videoCallView
		}
	}
	
	func pictureInPictureController(_ pictureInPictureController: AVPictureInPictureController, failedToStartPictureInPictureWithError error: Error) {
		CoreContext.shared.doOnCoreQueue { core in
			core.nativeVideoWindow = self.videoCallView
		}
		Log.error("\(PIPViewModel.TAG) failedToStartPictureInPictureWithError : \(error)")
	}
	
	func pictureInPictureController(_ pictureInPictureController: AVPictureInPictureController, restoreUserInterfaceForPictureInPictureStopWithCompletionHandler completionHandler: @escaping (Bool) -> Void) {
		Log.info("\(PIPViewModel.TAG) restoreUserInterfaceForPictureInPictureStopWithCompletionHandler")
		TelecomManager.shared.callDisplayed = true
		completionHandler(true)
	}
}

class PictureInPictureVideoCallViewController: AVPictureInPictureVideoCallViewController {
	
	var pipRemoteVideoView = SampleBufferVideoCallView()
	var pipWidth: Double = 720
	var pipHeight: Double = 480
	
	override func viewDidLoad() {
		super.viewDidLoad()
		
		view.backgroundColor = .black
		view.clipsToBounds = true
		view.addSubview(pipRemoteVideoView)
	}
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		view.layer.cornerRadius = 0
	}
	
	override func viewDidLayoutSubviews() {
		super.viewDidLayoutSubviews()
		self.matchVideoDimension()
	}
	
	func matchVideoDimension() {
		Log.info("\(PIPViewModel.TAG) - PIPViewController - matchVideoDimension to \(pipWidth)x\(pipHeight)")
		self.preferredContentSize = CGSize(width: pipWidth, height: pipHeight)
		pipRemoteVideoView.frame = view.bounds
	}
}

// swiftlint:disable force_cast
class SampleBufferVideoCallView: UIView {
	override class var layerClass: AnyClass {
		AVSampleBufferDisplayLayer.self
	}
	
	var sampleBufferDisplayLayer: AVSampleBufferDisplayLayer {
		layer as! AVSampleBufferDisplayLayer
	}
}
// swiftlint:enable force_cast
