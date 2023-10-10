/*
 * Copyright (c) 2010-2023 Belledonne Communications SARL.
 *
 * This file is part of Linphone
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
import SwiftUI
import AVFoundation

class QRScannerController: UIViewController {
	var captureSession = AVCaptureSession()
	var videoPreviewLayer: AVCaptureVideoPreviewLayer?
	var qrCodeFrameView: UIView?
	
	var delegate: AVCaptureMetadataOutputObjectsDelegate?
	
	override func viewDidLoad() {
		super.viewDidLoad()
		
		// Get the back-facing camera for capturing videos
		guard let captureDevice = AVCaptureDevice.default(.builtInWideAngleCamera, for: .video, position: .back) else {
			print("Failed to get the camera device")
			return
		}
		
		let videoInput: AVCaptureDeviceInput
		
		do {
			// Get an instance of the AVCaptureDeviceInput class using the previous device object.
			videoInput = try AVCaptureDeviceInput(device: captureDevice)
			
		} catch {
			// If any error occurs, simply print it out and don't continue any more.
			print(error)
			return
		}
		
		// Set the input device on the capture session.
		captureSession.addInput(videoInput)
		
		// Initialize a AVCaptureMetadataOutput object and set it as the output device to the capture session.
		let captureMetadataOutput = AVCaptureMetadataOutput()
		captureSession.addOutput(captureMetadataOutput)
		
		// Set delegate and use the default dispatch queue to execute the call back
		captureMetadataOutput.setMetadataObjectsDelegate(delegate, queue: DispatchQueue.main)
		captureMetadataOutput.metadataObjectTypes = [ .qr ]
		
		// Initialize the video preview layer and add it as a sublayer to the viewPreview view's layer.
		videoPreviewLayer = AVCaptureVideoPreviewLayer(session: captureSession)
		videoPreviewLayer?.videoGravity = AVLayerVideoGravity.resizeAspectFill
		videoPreviewLayer?.frame = view.layer.bounds
		view.layer.addSublayer(videoPreviewLayer!)
		
		// Start video capture.
		DispatchQueue.global(qos: .background).async {
			self.captureSession.startRunning()
		}
		
	}
	
}
