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
import AVFoundation
import AVKit
import PDFKit

class MediaViewer:  BackNextNavigationView, UICompositeViewDelegate, UIScrollViewDelegate, QLPreviewControllerDelegate, QLPreviewControllerDataSource {
	
	static let compositeDescription = UICompositeViewDescription(MediaViewer.self, statusBar: StatusBarView.self, tabBar: nil, sideMenu: SideMenuView.self, fullscreen: false, isLeftFragment: false,fragmentWith: nil)
	static func compositeViewDescription() -> UICompositeViewDescription! { return compositeDescription }
	func compositeViewDescription() -> UICompositeViewDescription! { return type(of: self).compositeDescription }

	var imageNameViewer = ""
	var imagePathViewer = ""
	var imageViewer = UIImage()
	var previewItems : [QLPreviewItem?] = []
	var contentType : String?
	let shareButton = CallControlButton(buttonTheme:VoipTheme.nav_button("voip_export"))
	
	//Image
	var newImageView = UIImageView()
	let imageViewViewer = UIImageView()
	let imageScrollView = UIScrollView()
	
	//Video
	var player: AVPlayer? = AVPlayer()
	var playerLayer = AVPlayerLayer()

	//PDF
	let pdfView = PDFView()

	override func viewDidLoad() {
		super.viewDidLoad(
			backAction: {
				PhoneMainView.instance().popView(self.compositeViewDescription())
			},nextAction: {
			},
			nextActionEnableCondition: MutableLiveData(false),
			title:"")
		super.nextButton.isHidden = true
		
		super.topBar.addSubview(shareButton)
		shareButton.alignParentRight(withMargin: side_buttons_margin).alignParentBottom(withMargin: 18).alignParentTop(withMargin: 18).done()

		shareButton.addTarget(self, action: #selector(shareMediaButton), for: .touchUpInside)
		try! AVAudioSession.sharedInstance().setCategory(.playback)
		UIDeviceBridge.displayModeSwitched.readCurrentAndObserve { _ in
			self.view.backgroundColor = VoipTheme.voipBackgroundBWColor.get()
		}
	}
	
	override func viewWillAppear(_ animated: Bool) {
		newImageView.removeFromSuperview()
		imageViewViewer.removeFromSuperview()
		imageScrollView.removeFromSuperview()
		playerLayer.removeFromSuperlayer()
		pdfView.removeFromSuperview()
		if contentType == "image" {
			setUpImageView()
		} else if contentType == "video" {
			setUpPlayerContainerView()
		} else if contentType == "file" {
			if imageNameViewer.lowercased().components(separatedBy: ".").last == "pdf" {
				displayPDF()
			}
		}
		
		try! AVAudioSession.sharedInstance().setActive(true)
		titleLabel.text = imageNameViewer
		titleLabel.toRightOf(backButton).toLeftOf(shareButton).done()
	}
	
	override func viewDidAppear(_ animated: Bool) {
		self.navigationController?.isNavigationBarHidden = false
		self.tabBarController?.tabBar.isHidden = false
		PhoneMainView.instance().hideStatusBar(false)
	}
	
	override func viewWillDisappear(_ animated: Bool) {
		stopPlayer()
	}
	
	func stopPlayer() {
		if let play = player {
			play.pause()
			play.replaceCurrentItem(with: nil)
			try! AVAudioSession.sharedInstance().setActive(false)
			print("Player deallocated")
		} else {
			print("Player was already deallocated")
		}
	}
	
	override func didRotate(from fromInterfaceOrientation: UIInterfaceOrientation) {
		dismissFullscreenImageRotated()
		stopPlayer()
		self.viewWillAppear(true)
	}
	
	@IBAction func imageTapped(_ sender: UITapGestureRecognizer) {
		let imageView = sender.view as! UIImageView
		newImageView = UIImageView(image: imageView.image)
		newImageView.frame = CGRect(x: 0, y: 0, width: UIScreen.main.bounds.size.width, height: UIScreen.main.bounds.size.height-20)
		newImageView.backgroundColor = .black
		newImageView.contentMode = .scaleAspectFit
		newImageView.isUserInteractionEnabled = true
		let tap = UITapGestureRecognizer(target: self, action: #selector(dismissFullscreenImage))
		newImageView.addGestureRecognizer(tap)
		self.view.addSubview(newImageView)
		self.navigationController?.isNavigationBarHidden = true
		self.tabBarController?.tabBar.isHidden = true
		PhoneMainView.instance().hideStatusBar(true)
	}
	
	func dismissFullscreenImageRotated() {
		self.navigationController?.isNavigationBarHidden = false
		self.tabBarController?.tabBar.isHidden = false
		PhoneMainView.instance().hideStatusBar(false)
	}

	@objc func dismissFullscreenImage(_ sender: UITapGestureRecognizer) {
		self.navigationController?.isNavigationBarHidden = false
		self.tabBarController?.tabBar.isHidden = false
		PhoneMainView.instance().hideStatusBar(false)
		sender.view?.removeFromSuperview()
	}
	
	func viewForZooming(in scrollView: UIScrollView) -> UIView? {
		return self.imageViewViewer
	}
	
	@objc func shareMediaButton(_ sender: UIButton) {
		let previewController = QLPreviewController()
		self.previewItems = []
		
		self.previewItems.append(self.getPreviewItem(filePath: imagePathViewer))
		
		previewController.dataSource = self
		previewController.delegate = self
		PhoneMainView.instance().mainViewController.present(previewController, animated: true, completion: nil)
		
	}
	
	private func setUpImageView() {
		let vWidth = self.view.bounds.size.width
		let vHeight = self.view.bounds.size.height-66
		
		imageScrollView.delegate = self
		imageScrollView.frame = CGRect(x: 0, y: 66, width: vWidth, height: vHeight)
		imageScrollView.showsVerticalScrollIndicator = true

		imageScrollView.minimumZoomScale = 1.0
		imageScrollView.maximumZoomScale = 10.0

		self.view.addSubview(imageScrollView)
		
		imageViewViewer.contentMode = .scaleAspectFit
		imageScrollView.addSubview(imageViewViewer)
		self.imageViewViewer.frame = CGRect(x: 0, y: 0, width: vWidth, height: vHeight)
		self.view.bringSubviewToFront(topBar)
		
		let pictureTap = UITapGestureRecognizer(target: self, action: #selector(imageTapped))
		imageViewViewer.addGestureRecognizer(pictureTap)
		imageViewViewer.isUserInteractionEnabled = true
		
		imageViewViewer.image = imageViewer
	}
	
	private func setUpPlayerContainerView() {
		let vWidth = self.view.bounds.size.width
		let vHeight = self.view.bounds.size.height-66
		if let urlEncoded = imagePathViewer.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed){
			if !urlEncoded.isEmpty {
				if let urlVideo = URL(string: "file://" + urlEncoded){
					player = AVPlayer(url: urlVideo)
					playerLayer = AVPlayerLayer(player: player)
					playerLayer.frame = CGRect(x: 0, y: 66, width: vWidth, height: vHeight)
					self.view.layer.addSublayer(playerLayer)
					if player != nil {
						player!.play()
					}
					
					let pictureTap = UITapGestureRecognizer(target: self, action: #selector(videoTapped))
					self.view.addGestureRecognizer(pictureTap)
					self.view.isUserInteractionEnabled = true
				}
			}
		}
	}
	
	func displayPDF() {
		if let urlEncoded = imagePathViewer.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed){
			if !urlEncoded.isEmpty {
				pdfView.translatesAutoresizingMaskIntoConstraints = false
				view.addSubview(pdfView)

				pdfView.leadingAnchor.constraint(equalTo: view.safeAreaLayoutGuide.leadingAnchor).isActive = true
				pdfView.trailingAnchor.constraint(equalTo: view.safeAreaLayoutGuide.trailingAnchor).isActive = true
				pdfView.topAnchor.constraint(equalTo: super.topBar.bottomAnchor).isActive = true
				pdfView.bottomAnchor.constraint(equalTo: view.safeAreaLayoutGuide.bottomAnchor).isActive = true
				pdfView.autoScales = true
				
				if let urlEncoded = imagePathViewer.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed){
					if let path =  URL(string: "file://" + urlEncoded) {
						if let document = PDFDocument(url: path) {
							pdfView.document = document
						}
					}

				}
			}
		}
	}
	
	@objc func videoTapped(){
		if let urlEncoded = imagePathViewer.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed){
			if !urlEncoded.isEmpty {
				if let urlVideo = URL(string: "file://" + urlEncoded){
					let player = AVPlayer(url: urlVideo)
					let playerViewController = AVPlayerViewController()
					playerViewController.player = player
					self.present(playerViewController, animated: true) {
						playerViewController.player!.play()
					}
				}
			}
		}
	}
	
	func getPreviewItem(filePath: String) -> NSURL{
		let url = NSURL(fileURLWithPath: filePath)
		return url
	}
	
	func numberOfPreviewItems(in controller: QLPreviewController) -> Int {
		return previewItems.count
	}
	
	func previewController(_ controller: QLPreviewController, previewItemAt index: Int) -> QLPreviewItem {
		return (previewItems[index] as QLPreviewItem?)!
	}
}
