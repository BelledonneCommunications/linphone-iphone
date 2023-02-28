//
//  MultilineMessageCell.swift
//  linphone
//
//  Created by Benoît Martins on 21/02/2023.
//

import UIKit
import Foundation
import linphonesw

class MultilineMessageCell: UICollectionViewCell {
	static let reuseId = "MultilineMessageCellReuseId"
	
	private let label: UILabel = UILabel(frame: .zero)
	private let contentBubble: UIView = UIView(frame: .zero)
	private let bubble: UIView = UIView(frame: .zero)
	private let imageUser: UIView = UIView(frame: .zero)
	private let chatRead = UIImageView(image: UIImage(named: "chat_read.png"))

	var constraint1 : NSLayoutConstraint? = nil
	var constraint2 : NSLayoutConstraint? = nil
	//var imageConstraint : [NSLayoutConstraint?] = []
	
	let labelInset = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 0)
	
	var isPlayingVoiceRecording = false
	var vrPlayerTimer = Timer()
	
	override init(frame: CGRect) {
		super.init(frame: frame)

		contentView.addSubview(contentBubble)
		contentBubble.translatesAutoresizingMaskIntoConstraints = false
		contentBubble.topAnchor.constraint(equalTo: contentView.topAnchor, constant: 0).isActive = true
		contentBubble.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: 0).isActive = true
		constraint1 = contentBubble.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 40)
		constraint2 = contentBubble.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -22)
		constraint1!.isActive = true
		
		contentBubble.addSubview(imageUser)
		imageUser.topAnchor.constraint(equalTo: contentView.topAnchor).isActive = true
		imageUser.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 6).isActive = true
		imageUser.backgroundColor = UIColor("D").withAlphaComponent(0.2)
		imageUser.layer.cornerRadius = 15.0
		imageUser.size(w: 30, h: 30).done()
		imageUser.isHidden = true
		
		contentBubble.addSubview(bubble)
		bubble.translatesAutoresizingMaskIntoConstraints = false
		bubble.topAnchor.constraint(equalTo: contentView.topAnchor, constant: labelInset.top).isActive = true
		bubble.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: labelInset.bottom).isActive = true
		bubble.leadingAnchor.constraint(equalTo: contentBubble.leadingAnchor, constant: labelInset.left).isActive = true
		bubble.trailingAnchor.constraint(equalTo: contentBubble.trailingAnchor, constant: labelInset.right).isActive = true
		bubble.layer.cornerRadius = 10.0
		
		contentBubble.addSubview(chatRead)
		chatRead.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -2).isActive = true
		chatRead.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -8).isActive = true
		chatRead.size(w: 10, h: 10).done()
		chatRead.isHidden = true
	}
	
	required init?(coder aDecoder: NSCoder) {
		fatalError("Storyboards are quicker, easier, more seductive. Not stronger then Code.")
	}
	
	override func removeFromSuperview() {
		print("MultilineMessageCell configure ChatMessage animPlayerOnce stop stopstop died")
	}
	
	func configure(message: ChatMessage, isBasic: Bool) {
	/*
	 	For Multimedia
		message.contents.forEach { content in
			label.text = content.utf8Text
		}
	*/
		
		if bubble.subviews.count > 0
		{
			bubble.subviews.forEach({ $0.removeFromSuperview()})
		}
		
		
		label.text = message.contents.first?.utf8Text.trimmingCharacters(in: .whitespacesAndNewlines)
		if !message.isOutgoing {
			constraint1?.isActive = true
			constraint2?.isActive = false
			imageUser.isHidden = false
			bubble.backgroundColor = UIColor("D").withAlphaComponent(0.2)
			chatRead.isHidden = true
		}else{
			constraint1?.isActive = false
			constraint2?.isActive = true
			imageUser.isHidden = true
			bubble.backgroundColor = UIColor("A").withAlphaComponent(0.2)
			chatRead.isHidden = false
		}
		
		if isBasic {
			if message.contents.first?.type == "text"{
				createBubbleText()
			}else if message.contents.first?.type == "image"{
				createBubbleImage(message: message)
			}else if message.contents.first?.type == "video"{
				createBubbleVideo(message: message)
			}else if message.contents.first?.type == "audio"{
				createBubbleAudio(message: message)
			}else{
				//createBubbleText()
			}
		}
	}

	func createBubbleText(){
		print("MultilineMessageCell configure ChatMessage other")
		label.numberOfLines = 0
		label.lineBreakMode = .byWordWrapping
		
		bubble.addSubview(label)
		label.translatesAutoresizingMaskIntoConstraints = false
		label.topAnchor.constraint(equalTo: contentView.topAnchor, constant: labelInset.top+10).isActive = true
		label.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: labelInset.bottom-10).isActive = true
		label.leadingAnchor.constraint(equalTo: contentBubble.leadingAnchor, constant: labelInset.left+10).isActive = true
		label.trailingAnchor.constraint(equalTo: contentBubble.trailingAnchor, constant: labelInset.right-10).isActive = true
	}
	
	func createBubbleImage(message: ChatMessage){
		print("MultilineMessageCell configure ChatMessage image")
		
		let imageViewBubble = UIImageView(image: UIImage(named: "chat_error"))
		
		bubble.addSubview(imageViewBubble)
		imageViewBubble.translatesAutoresizingMaskIntoConstraints = false
		imageViewBubble.topAnchor.constraint(equalTo: contentView.topAnchor, constant: labelInset.top+10).isActive = true
		imageViewBubble.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: labelInset.bottom-10).isActive = true
		imageViewBubble.leadingAnchor.constraint(equalTo: contentBubble.leadingAnchor, constant: labelInset.left+10).isActive = true
		imageViewBubble.trailingAnchor.constraint(equalTo: contentBubble.trailingAnchor, constant: labelInset.right-10).isActive = true
		
		if let imageMessage = UIImage(named: message.contents.first!.filePath){
			imageViewBubble.image = resizeImage(image: imageMessage, targetSize: CGSizeMake(UIScreen.main.bounds.size.width*3/4, 300.0))
		}
	}
	
	func createBubbleVideo(message: ChatMessage){
		print("MultilineMessageCell configure ChatMessage video")
		
		let imageViewBubble = UIImageView(image: UIImage(named: "file_video_default"))
		let imagePlayViewBubble = UIImageView(image: UIImage(named: "vr_play"))
		
		bubble.addSubview(imageViewBubble)
		imageViewBubble.translatesAutoresizingMaskIntoConstraints = false
		imageViewBubble.topAnchor.constraint(equalTo: contentView.topAnchor, constant: labelInset.top+10).isActive = true
		imageViewBubble.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: labelInset.bottom-10).isActive = true
		imageViewBubble.leadingAnchor.constraint(equalTo: contentBubble.leadingAnchor, constant: labelInset.left+10).isActive = true
		imageViewBubble.trailingAnchor.constraint(equalTo: contentBubble.trailingAnchor, constant: labelInset.right-10).isActive = true
		
		if #available(iOS 13.0, *) {
			imagePlayViewBubble.image = (UIImage(named: "vr_play")!.withTintColor(.white))
		}
		
		imageViewBubble.addSubview(imagePlayViewBubble)
		imagePlayViewBubble.centerXAnchor.constraint(equalTo: imageViewBubble.centerXAnchor).isActive = true
		imagePlayViewBubble.centerYAnchor.constraint(equalTo: imageViewBubble.centerYAnchor).isActive = true
		imagePlayViewBubble.size(w: 40, h: 40).done()
		
	
		if let imageMessage = createThumbnailOfVideoFromFileURL(videoURL: message.contents.first!.filePath){
			imageViewBubble.image = resizeImage(image: imageMessage, targetSize: CGSizeMake(UIScreen.main.bounds.size.width*3/4, 300.0))
		}
	}
	
	func createBubbleAudio(message: ChatMessage){
		print("MultilineMessageCell configure ChatMessage audio")
		
		let recordingView = UIView()
		let recordingPlayButton = CallControlButton(width: 40, height: 40, buttonTheme:VoipTheme.nav_button("vr_play"))
		let recordingStopButton = CallControlButton(width: 40, height: 40, buttonTheme:VoipTheme.nav_button("vr_stop"))
		let recordingWaveView = UIProgressView()
		let recordingDurationTextView = StyledLabel(VoipTheme.chat_conversation_recording_duration)
		let recordingWaveImage = UIImageView(image: UIImage(named: "vr_wave.png"))
		
		bubble.addSubview(recordingView)
		recordingView.translatesAutoresizingMaskIntoConstraints = false
		recordingView.topAnchor.constraint(equalTo: contentView.topAnchor, constant: labelInset.top+10).isActive = true
		recordingView.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: labelInset.bottom-10).isActive = true
		recordingView.leadingAnchor.constraint(equalTo: contentBubble.leadingAnchor, constant: labelInset.left+10).isActive = true
		recordingView.trailingAnchor.constraint(equalTo: contentBubble.trailingAnchor, constant: labelInset.right-10).isActive = true
		recordingView.height(50.0).width(280).done()
		
		recordingView.addSubview(recordingWaveView)
		recordingWaveView.translatesAutoresizingMaskIntoConstraints = false
		recordingWaveView.topAnchor.constraint(equalTo: contentView.topAnchor, constant: labelInset.top+10).isActive = true
		recordingWaveView.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: labelInset.bottom-10).isActive = true
		recordingWaveView.leadingAnchor.constraint(equalTo: contentBubble.leadingAnchor, constant: labelInset.left+10).isActive = true
		recordingWaveView.trailingAnchor.constraint(equalTo: contentBubble.trailingAnchor, constant: labelInset.right-10).isActive = true
		
		recordingWaveView.progressViewStyle = .bar
		recordingWaveView.layer.cornerRadius = 5
		recordingWaveView.progressTintColor = message.isOutgoing ? UIColor("A") : UIColor("D")
		recordingWaveView.clipsToBounds = true
		recordingWaveView.layer.sublayers![1].cornerRadius = 5
		recordingWaveView.subviews[1].clipsToBounds = true
		
		recordingWaveView.addSubview(recordingPlayButton)
		recordingPlayButton.alignParentLeft(withMargin: 10).matchParentHeight().done()
		
		recordingWaveView.addSubview(recordingStopButton)
		recordingStopButton.alignParentLeft(withMargin: 10).matchParentHeight().done()
		recordingStopButton.isHidden = true
		
		recordingWaveView.addSubview(recordingWaveImage)
		recordingWaveImage.alignParentLeft(withMargin: 60).alignParentRight(withMargin: 60).height(26).alignHorizontalCenterWith(recordingView).done()
		
		recordingWaveView.addSubview(recordingDurationTextView)
		recordingDurationTextView.alignParentRight(withMargin: 10).matchParentHeight().done()
		recordingDurationTextView.text = recordingDuration(message.contents.first?.filePath)
		
		recordingPlayButton.onClickAction = {
			self.playRecordedMessage(recordingPlayButton: recordingPlayButton, recordingStopButton: recordingStopButton, recordingWaveView: recordingWaveView, voiceRecorder: message.contents.first?.filePath)
		}
		recordingStopButton.onClickAction = {
			self.stopVoiceRecordPlayer(recordingPlayButton: recordingPlayButton, recordingStopButton: recordingStopButton, recordingWaveView: recordingWaveView)
		}
	}
	
	func createBubbleAudioFile(message: ChatMessage){
		print("MultilineMessageCell configure ChatMessage audio file")
		let imageAudioBubble = UIImage(named: "file_audio_default")
		let imageViewBubble = UIImageView(image: UIImage(named: "chat_error"))
		if let imageMessage = imageAudioBubble{
			bubble.addSubview(imageViewBubble)
			imageViewBubble.translatesAutoresizingMaskIntoConstraints = false
			imageViewBubble.topAnchor.constraint(equalTo: contentView.topAnchor, constant: labelInset.top+10).isActive = true
			imageViewBubble.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: labelInset.bottom-10).isActive = true
			imageViewBubble.leadingAnchor.constraint(equalTo: contentBubble.leadingAnchor, constant: labelInset.left+10).isActive = true
			imageViewBubble.trailingAnchor.constraint(equalTo: contentBubble.trailingAnchor, constant: labelInset.right-10).isActive = true
			imageViewBubble.image = imageMessage
		}
	}
	
	override func preferredLayoutAttributesFitting(_ layoutAttributes: UICollectionViewLayoutAttributes) -> UICollectionViewLayoutAttributes {
		label.preferredMaxLayoutWidth = (UIScreen.main.bounds.size.width*3/4)
		layoutAttributes.bounds.size.height = systemLayoutSizeFitting(UIView.layoutFittingCompressedSize).height
		
		let cellsPerRow = 1
		let minimumInterItemSpacing = 1.0
		let marginsAndInsets = window!.safeAreaInsets.left + window!.safeAreaInsets.right + minimumInterItemSpacing * CGFloat(cellsPerRow - 1)
		layoutAttributes.bounds.size.width = ((window!.bounds.size.width - marginsAndInsets) / CGFloat(cellsPerRow)).rounded(.down)
				
		return layoutAttributes
	}
	
	func createThumbnailOfVideoFromFileURL(videoURL: String) -> UIImage? {
		let asset = AVAsset(url: URL(string: "file://" + videoURL)!)
		let assetImgGenerate = AVAssetImageGenerator(asset: asset)
		assetImgGenerate.appliesPreferredTrackTransform = true
		do {
			let img = try assetImgGenerate.copyCGImage(at: CMTimeMake(value: 1, timescale: 10), actualTime: nil)
			let thumbnail = UIImage(cgImage: img)
			return thumbnail
		} catch _{
			return nil
		}
	}
	
	func resizeImage(image: UIImage, targetSize: CGSize) -> UIImage {
		let size = image.size
		
		let widthRatio  = targetSize.width  / size.width
		let heightRatio = targetSize.height / size.height
		
		// Figure out what our orientation is, and use that to form the rectangle
		var newSize: CGSize
		if(widthRatio > heightRatio) {
			newSize = CGSize(width: size.width * heightRatio, height: size.height * heightRatio)
		} else {
			newSize = CGSize(width: size.width * widthRatio,  height: size.height * widthRatio)
		}
		
		// This is the rect that we've calculated out and this is what is actually used below
		let rect = CGRect(x: 0, y: 0, width: newSize.width, height: newSize.height)
		
		// Actually do the resizing to the rect using the ImageContext stuff
		UIGraphicsBeginImageContextWithOptions(newSize, true, 2.0)
		image.draw(in: rect)
		let newImage = UIGraphicsGetImageFromCurrentImageContext()
		UIGraphicsEndImageContext()
		
		return newImage!
	}
	
	//Audio
	func playRecordedMessage(recordingPlayButton: UIButton, recordingStopButton:UIButton, recordingWaveView: UIProgressView, voiceRecorder: String?) {
		AudioPlayer.initSharedPlayer()
		AudioPlayer.sharedModel.fileChanged.value = voiceRecorder
		print("MultilineMessageCell configure ChatMessage animPlayerOnce \(String(describing: voiceRecorder))")
		recordingPlayButton.isHidden = true
		recordingStopButton.isHidden = false
		AudioPlayer.startSharedPlayer(voiceRecorder)
		self.animPlayerOnce(recordingPlayButton: recordingPlayButton, recordingStopButton: recordingStopButton, recordingWaveView: recordingWaveView, voiceRecorder: voiceRecorder)
		vrPlayerTimer = Timer.scheduledTimer(withTimeInterval: 1.01, repeats: true) { timer in
			self.animPlayerOnce(recordingPlayButton: recordingPlayButton, recordingStopButton: recordingStopButton, recordingWaveView: recordingWaveView, voiceRecorder: voiceRecorder)
			print("MultilineMessageCell configure ChatMessage animPlayerOnce timer")
		}
		isPlayingVoiceRecording = true
	}
	
	func recordingDuration(_ _voiceRecordingFile: String?) -> String? {
		let core = Core.getSwiftObject(cObject: LinphoneManager.getLc())
		var result = ""
		do{
			let linphonePlayer = try core.createLocalPlayer(soundCardName: nil, videoDisplayName: nil, windowId: nil)
			try linphonePlayer.open(filename: _voiceRecordingFile!)
			result = formattedDuration(linphonePlayer.duration)!
			linphonePlayer.close()
		}catch{
			print(error)
		}
		return result
	}
	
	func formattedDuration(_ valueMs: Int) -> String? {
		return String(format: "%02ld:%02ld", valueMs / 60000, (valueMs % 60000) / 1000)
	}
	
	func animPlayerOnce(recordingPlayButton: UIButton, recordingStopButton:UIButton, recordingWaveView: UIProgressView, voiceRecorder: String?) {
		print("MultilineMessageCell configure ChatMessage animPlayerOnce \(String(describing: AudioPlayer.getSharedPlayer()!.duration))")
		recordingWaveView.progress += floor(1.0 / Float(AudioPlayer.getSharedPlayer()!.duration/1000) * 10) / 10.0
		AudioPlayer.sharedModel.fileChanged.observe { file in
			if (file != voiceRecorder && self.isPlayingVoiceRecording) {
				print("MultilineMessageCell configure ChatMessage animPlayerOnce stop stopstop\(String(describing: AudioPlayer.getSharedPlayer()!.duration))")
				self.stopVoiceRecordPlayer(recordingPlayButton: recordingPlayButton, recordingStopButton: recordingStopButton, recordingWaveView: recordingWaveView)
			}
		}
		UIView.animate(withDuration: 1, delay: 0.0, options: .curveLinear, animations: {
			recordingWaveView.layoutIfNeeded()
		}) { Bool in
			if(recordingWaveView.progress >= 1.0 && self.isPlayingVoiceRecording){
				UIView.animate(withDuration: 1, delay: 0.0, options: .curveLinear, animations: {
					recordingWaveView.layoutIfNeeded()
				})
				self.stopVoiceRecordPlayer(recordingPlayButton: recordingPlayButton, recordingStopButton: recordingStopButton, recordingWaveView: recordingWaveView)
			}
		}
	}
	
	func stopVoiceRecordPlayer(recordingPlayButton: UIButton, recordingStopButton:UIButton, recordingWaveView: UIProgressView) {
		AudioPlayer.stopSharedPlayer()
		recordingWaveView.progress = 0.0
		recordingWaveView.setProgress(recordingWaveView.progress, animated: false)
		recordingPlayButton.isHidden = false
		recordingStopButton.isHidden = true
		isPlayingVoiceRecording = false
		vrPlayerTimer.invalidate()
	}
}
