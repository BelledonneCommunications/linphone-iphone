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

	var pipController: AVPictureInPictureController?
	var pipRemoteVideoView = SampleBufferVideoCallView()
	var videoCallView = UIView()
	
	var callStateChangedDelegate: CallDelegate?
	
	func setupPiPViewController(remoteView: UIView) {
		Log.info("debugtrace setupPiPViewController")
		videoCallView = remoteView
		let pipVideoCallController = PictureInPictureVideoCallViewController()
		pipRemoteVideoView = pipVideoCallController.pipRemoteVideoView
		let pipContentSource = AVPictureInPictureController.ContentSource(
			activeVideoCallSourceView: videoCallView,
			contentViewController: pipVideoCallController)
		pipController = AVPictureInPictureController(contentSource: pipContentSource)
		pipController?.delegate = self
		pipController?.canStartPictureInPictureAutomaticallyFromInline = true
		
		CoreContext.shared.doOnCoreQueue { core in
			if let call = core.currentCall {
				self.callStateChangedDelegate = CallDelegateStub(onStateChanged: { (_: Call, cstate: Call.State, _: String) in
					if cstate != .StreamsRunning && CoreContext.shared.pipViewModel.pipController?.isPictureInPictureActive ?? false {
						Log.info("debugtrace -- callstate changed stop pip")
						CoreContext.shared.pipViewModel.pipController?.stopPictureInPicture()
						if cstate == .End || cstate == .Error {
							self.callStateChangedDelegate = nil
						}
					}
				})
				call.addDelegate(delegate: self.callStateChangedDelegate!)
				Log.info("debugtrace -- added callstatechanged delegate")
			} else {
				Log.info("debugtrace -- no current call")
			}
		}
		/*
		ControlsViewModel.shared.isVideoEnabled.readCurrentAndObserve{ (video) in
			pipVideoCallController.matchVideoDimension()
			self.pipController?.canStartPictureInPictureAutomaticallyFromInline = video == true
		}
		
		CallsViewModel.shared.currentCallData.observe(onChange: { callData in
			if (callData??.call.state != .StreamsRunning && self.pipController?.isPictureInPictureActive) {
				self.pipController?.stopPictureInPicture()
			}
		})
		*/
	}
	
	
	func pictureInPictureControllerWillStartPictureInPicture(_ pictureInPictureController: AVPictureInPictureController) {
		Log.info("debugtrace pictureInPictureControllerWillStartPictureInPicture")
		CoreContext.shared.doOnCoreQueue { core in
			core.nativeVideoWindow = self.pipRemoteVideoView
		}
	}
	
	func pictureInPictureControllerDidStopPictureInPicture(_ pictureInPictureController: AVPictureInPictureController) {
		Log.info("debugtrace pictureInPictureControllerDidStopPictureInPicture")
		CoreContext.shared.doOnCoreQueue { core in
			core.nativeVideoWindow = self.videoCallView
		}
	}
	
	func pictureInPictureController(_ pictureInPictureController: AVPictureInPictureController, failedToStartPictureInPictureWithError error: Error) {
		CoreContext.shared.doOnCoreQueue { core in
			core.nativeVideoWindow = self.videoCallView
		}
		Log.error("Start Picture in Picture video call error : \(error)")
		// DispatchQueue.main.async { self.configurationPiPViewController() }
	}
	
	func pictureInPictureController(_ pictureInPictureController: AVPictureInPictureController, restoreUserInterfaceForPictureInPictureStopWithCompletionHandler completionHandler: @escaping (Bool) -> Void) {
		Log.info("debugtrace restoreUserInterfaceForPictureInPictureStopWithCompletionHandler")
		
		
		TelecomManager.shared.callDisplayed = true
		/* a
		if (CallsViewModel.shared.currentCallData.value??.call.state == .StreamsRunning && PhoneMainView.instance().currentView != self.compositeViewDescription()) {
			PhoneMainView.instance().changeCurrentView(self.compositeViewDescription())
			//Core.get().nativeVideoWindow = pipRemoteVideoView // let the video on the pip view during the stop animation
		}
		//pictureInPictureController.contentSource?.activeVideoCallContentViewController.view.layer.cornerRadius = ActiveCallView.center_view_corner_radius
		 */
		completionHandler(true)
	}
}

class PictureInPictureVideoCallViewController: AVPictureInPictureVideoCallViewController {
	
	var pipRemoteVideoView = SampleBufferVideoCallView()
	
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
		matchVideoDimension()
		super.viewDidLayoutSubviews()
	}
	
	func matchVideoDimension() {
		Log.info("debugtrace - matchVideoDimension")
		self.preferredContentSize = CGSize(width: Double(720), height: Double(480))
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
