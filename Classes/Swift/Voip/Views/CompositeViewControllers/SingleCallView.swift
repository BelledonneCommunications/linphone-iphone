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


import UIKit
import linphonesw
import AVKit


@objc class SingleCallView: AbstractCallView, UICompositeViewDelegate {
	
	var callPausedByRemoteView : PausedCallOrConferenceView? = nil
	var callPausedByLocalView : PausedCallOrConferenceView? = nil
	var currentCallView : ActiveCallView? = nil
	
	private var pipController: AVPictureInPictureController!
	private var pipRemoteVideoView = SampleBufferVideoCallView()
	
	static let compositeDescription = UICompositeViewDescription(SingleCallView.self, statusBar: StatusBarView.self, tabBar: nil, sideMenu: nil, fullscreen: false, isLeftFragment: false,fragmentWith: nil)
	static func compositeViewDescription() -> UICompositeViewDescription! { return compositeDescription }
	func compositeViewDescription() -> UICompositeViewDescription! { return type(of: self).compositeDescription }
	
	override func viewDidLoad() {
		super.viewDidLoad()
	
		// Current (Single) Call (VoipCallView)
		currentCallView = ActiveCallView()
		fullScreenMutableContainerView.addSubview(currentCallView!)
		CallsViewModel.shared.currentCallData.readCurrentAndObserve { (currentCallData) in
			guard currentCallData??.isOngoingSingleCall() == true else {
				return
			}
			self.currentCallView!.callData = currentCallData != nil ? currentCallData! : nil
			currentCallData??.isRemotelyPaused.readCurrentAndObserve { remotelyPaused in
				self.callPausedByRemoteView?.isHidden = remotelyPaused != true
			}
			currentCallData??.isPaused.readCurrentAndObserve { locallyPaused in
				self.callPausedByLocalView?.isHidden = locallyPaused != true
			}
			if (currentCallData == nil) {
				self.callPausedByRemoteView?.isHidden = true
				self.callPausedByLocalView?.isHidden = true
				
			}
			self.extraButtonsView.isHidden = true
		}
		
		currentCallView!.matchParentDimmensions().done()
		
		// Paused by remote (Call)
		callPausedByRemoteView = PausedCallOrConferenceView(iconName: "voip_conference_paused_big",titleText: VoipTexts.call_remotely_paused_title,subTitleText: nil)
		view.addSubview(callPausedByRemoteView!)
		callPausedByRemoteView?.matchParentSideBorders().matchParentHeight().alignAbove(view:controlsView,withMargin:SharedLayoutConstants.buttons_bottom_margin).done()
		callPausedByRemoteView?.isHidden = true
		
		// Paused by local (Call)
		callPausedByLocalView = PausedCallOrConferenceView(iconName: "voip_conference_play_big",titleText: VoipTexts.call_locally_paused_title,subTitleText: VoipTexts.call_locally_paused_subtitle, onClickAction: {
			CallsViewModel.shared.currentCallData.value??.togglePause()
		})
		view.addSubview(callPausedByLocalView!)
		callPausedByLocalView?.matchParentSideBorders().matchParentHeight().alignAbove(view:controlsView,withMargin:SharedLayoutConstants.buttons_bottom_margin).done()
		callPausedByLocalView?.isHidden = true
				
		// Video activation dialog request
		CallsViewModel.shared.callUpdateEvent.observe { (call) in
			let core = Core.get()
			if (call?.state == .StreamsRunning) {
				self.videoAcceptDialog?.removeFromSuperview()
				self.videoAcceptDialog = nil
			} else if (call?.state == .UpdatedByRemote) {
				if ((core.videoCaptureEnabled || core.videoDisplayEnabled) && Core.get().config?.getBool(section: "app", key: "disable_video_feature", defaultValue: false) == false) {
					if (call?.currentParams?.videoEnabled != call?.remoteParams?.videoEnabled) {
						let accept = ButtonAttributes(text:VoipTexts.dialog_accept, action: {call?.answerVideoUpdateRequest(accept: true)}, isDestructive:false)
						let cancel = ButtonAttributes(text:VoipTexts.dialog_decline, action: {call?.answerVideoUpdateRequest(accept: false)}, isDestructive:true)
						self.videoAcceptDialog = VoipDialog(message:VoipTexts.call_video_update_requested_dialog, givenButtons:  [cancel,accept])
						self.videoAcceptDialog?.show()
					}
				} else {
					if (Core.get().config?.getBool(section: "app", key: "disable_video_feature", defaultValue: false) == true) {
						call?.answerVideoUpdateRequest(accept: false)
					}
					Log.w("[Call] Video display & capture are disabled, don't show video dialog")
				}
			}
		}
		view.onClick {
			ControlsViewModel.shared.audioRoutesSelected.value = false
		}
		
		// picture in picture init
		if #available(iOS 15.0, *) {
			DispatchQueue.main.async { self.configurationPiPViewController() }
		}
	}
	
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(true)
		CallsViewModel.shared.currentCallData.notifyValue()
	}
	
	override func didRotate(from fromInterfaceOrientation: UIInterfaceOrientation) {
		super.didRotate(from: fromInterfaceOrientation)
		self.currentCallView?.layoutRotatableElements()
	}
	
	// Picture in picture on video call
	override func viewWillDisappear(_ animated: Bool) {
		super.viewWillDisappear(animated)
		if (CallsViewModel.shared.currentCallData.value??.call.state == .StreamsRunning && pipController.isPictureInPicturePossible) {
			pipController.startPictureInPicture()
		}
	}
	
	override func viewDidAppear(_ animated: Bool) {
		super.viewDidAppear(animated)
		if pipController.isPictureInPictureActive {
			pipController.stopPictureInPicture()
		}
	}
	
}

// Picture in picture on video call
@available(iOS 15.0, *)
extension SingleCallView : AVPictureInPictureControllerDelegate {

	func configurationPiPViewController() {
		let pipVideoCallController = PictureInPictureVideoCallViewController()
		pipRemoteVideoView = pipVideoCallController.pipRemoteVideoView
		let pipContentSource = AVPictureInPictureController.ContentSource(
			activeVideoCallSourceView: currentCallView!.remoteVideo,
			contentViewController: pipVideoCallController)
		pipController = AVPictureInPictureController(contentSource: pipContentSource)
		if (pipController != nil) {
			pipController.delegate = self
			
			ControlsViewModel.shared.isVideoEnabled.readCurrentAndObserve{ (video) in
				pipVideoCallController.matchVideoDimension()
				self.pipController.canStartPictureInPictureAutomaticallyFromInline = video == true
			}
			
			CallsViewModel.shared.currentCallData.observe(onChange: { callData in
				if (callData??.call.state != .StreamsRunning && self.pipController.isPictureInPictureActive) {
					self.pipController.stopPictureInPicture()
				}
			})
		}
	}
	
	
	func pictureInPictureControllerWillStartPictureInPicture(_ pictureInPictureController: AVPictureInPictureController) {
		Core.get().nativeVideoWindow = pipRemoteVideoView
	}
	
	func pictureInPictureControllerDidStopPictureInPicture(_ pictureInPictureController: AVPictureInPictureController) {
		Core.get().nativeVideoWindow = currentCallView?.remoteVideo
	}
	
	func pictureInPictureController(_ pictureInPictureController: AVPictureInPictureController, failedToStartPictureInPictureWithError error: Error) {
		Core.get().nativeVideoWindow = currentCallView?.remoteVideo
		Log.e("Start Picture in Picture video call error : \(error)")
		DispatchQueue.main.async { self.configurationPiPViewController() }
	}
	
	func pictureInPictureController(_ pictureInPictureController: AVPictureInPictureController, restoreUserInterfaceForPictureInPictureStopWithCompletionHandler completionHandler: @escaping (Bool) -> Void) {
		if (CallsViewModel.shared.currentCallData.value??.call.state == .StreamsRunning && PhoneMainView.instance().currentView != self.compositeViewDescription()) {
			PhoneMainView.instance().changeCurrentView(self.compositeViewDescription())
			Core.get().nativeVideoWindow = pipRemoteVideoView // let the video on the pip view during the stop animation
		}
		pictureInPictureController.contentSource?.activeVideoCallContentViewController.view.layer.cornerRadius = ActiveCallView.center_view_corner_radius
		completionHandler(true)
	}
}

@available(iOS 15.0, *)
class PictureInPictureVideoCallViewController : AVPictureInPictureVideoCallViewController {
	
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
		super.viewDidLayoutSubviews()
		matchVideoDimension()
	}
	
	func matchVideoDimension() {
		let videoDefinition = CallsViewModel.shared.currentCallData.value??.call.currentParams?.receivedVideoDefinition
		if (videoDefinition != nil) {
			self.preferredContentSize = CGSize(width: Double(videoDefinition!.width), height: Double(videoDefinition!.height))
			pipRemoteVideoView.frame = view.bounds
		}
	}
}

class SampleBufferVideoCallView: UIView {
	override class var layerClass: AnyClass {
		AVSampleBufferDisplayLayer.self
	}
	
	var sampleBufferDisplayLayer: AVSampleBufferDisplayLayer {
		layer as! AVSampleBufferDisplayLayer
	}
}

