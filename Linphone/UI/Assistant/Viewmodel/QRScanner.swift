//
//  QRScanner.swift
//  Linphone
//
//  Created by Benoît Martins on 05/10/2023.
//

import Foundation
import SwiftUI
import AVFoundation

struct QRScanner: UIViewControllerRepresentable {
	
	@Binding var result: String
	
	func makeUIViewController(context: Context) -> QRScannerController {
		let controller = QRScannerController()
		controller.delegate = context.coordinator
		
		return controller
	}
	
	func makeCoordinator() -> Coordinator {
		Coordinator($result)
	}
	
	func updateUIViewController(_ uiViewController: QRScannerController, context: Context) {
	}
}

class Coordinator: NSObject, AVCaptureMetadataOutputObjectsDelegate {
	
	private var coreContext = CoreContext.shared
	
	@Binding var scanResult: String
	private var lastResult: String = ""
	
	init(_ scanResult: Binding<String>) {
		self._scanResult = scanResult
	}
	
	func metadataOutput(_ output: AVCaptureMetadataOutput, didOutput metadataObjects: [AVMetadataObject], from connection: AVCaptureConnection) {
		
		// Check if the metadataObjects array is not nil and it contains at least one object.
		if metadataObjects.count == 0 {
			scanResult = "Scan a QR code"
			return
		}
		
		// Get the metadata object.
		let metadataObj = metadataObjects[0] as! AVMetadataMachineReadableCodeObject
		
		if metadataObj.type == AVMetadataObject.ObjectType.qr,
		   let result = metadataObj.stringValue {
			if !result.isEmpty && result != lastResult {
				if let url = NSURL(string: result) {
					if UIApplication.shared.canOpenURL(url as URL) {
						lastResult = result
						//scanResult = result
						do {
							try coreContext.mCore.setProvisioninguri(newValue: result)
							coreContext.mCore.stop()
							try coreContext.mCore.start()
						}catch {
							
						}
						
					} else {
						coreContext.configuringSuccessful = "Invalide URI"
					}
				} else {
					coreContext.configuringSuccessful = "Invalide URI"
				}
			}
		}
	}
}
