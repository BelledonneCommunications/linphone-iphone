//
//  MultilineMessageCell.swift
//  linphone
//
//  Created by Benoît Martins on 01/03/2023.
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

	let labelInset = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 0)
	
	var constraintLeadingBubble : NSLayoutConstraint? = nil
	var constraintTrailingBubble : NSLayoutConstraint? = nil
	var labelConstraints: [NSLayoutConstraint] = []
	var imageConstraints: [NSLayoutConstraint] = []
	var videoConstraints: [NSLayoutConstraint] = []
	var playButtonConstraints: [NSLayoutConstraint] = []
	var recordingConstraints: [NSLayoutConstraint] = []
	var recordingWaveConstraints: [NSLayoutConstraint] = []
	
	let imageViewBubble = UIImageView(image: UIImage(named: "chat_error"))
	let imageVideoViewBubble = UIImageView(image: UIImage(named: "file_video_default"))
	let imagePlayViewBubble = UIImageView(image: UIImage(named: "vr_play"))
	
	let recordingView = UIView()
	
	var isPlayingVoiceRecording = false
	

	override init(frame: CGRect) {
		super.init(frame: frame)

		contentView.addSubview(contentBubble)
		contentBubble.translatesAutoresizingMaskIntoConstraints = false
		contentBubble.topAnchor.constraint(equalTo: contentView.topAnchor, constant: 0).isActive = true
		contentBubble.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: 0).isActive = true
		constraintLeadingBubble = contentBubble.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 40)
		constraintTrailingBubble = contentBubble.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -22)
		constraintLeadingBubble!.isActive = true
		
		contentBubble.addSubview(imageUser)
		imageUser.topAnchor.constraint(equalTo: contentView.topAnchor).isActive = true
		imageUser.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 6).isActive = true
		imageUser.backgroundColor = UIColor("D").withAlphaComponent(0.2)
		imageUser.layer.cornerRadius = 15.0
		imageUser.size(w: 30, h: 30).done()
		
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
		
	//Text
		label.numberOfLines = 0
		label.lineBreakMode = .byWordWrapping
		
		bubble.addSubview(label)
		label.translatesAutoresizingMaskIntoConstraints = false
		labelConstraints = [
			label.topAnchor.constraint(equalTo: contentView.topAnchor, constant: labelInset.top+10),
			label.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: labelInset.bottom-10),
			label.leadingAnchor.constraint(equalTo: contentBubble.leadingAnchor, constant: labelInset.left+10),
			label.trailingAnchor.constraint(equalTo: contentBubble.trailingAnchor, constant: labelInset.right-10)
		]
		NSLayoutConstraint.activate(labelConstraints)

		
	//Image
		bubble.addSubview(imageViewBubble)
		imageViewBubble.translatesAutoresizingMaskIntoConstraints = false
		imageConstraints = [
			imageViewBubble.topAnchor.constraint(equalTo: contentView.topAnchor, constant: labelInset.top+10),
			imageViewBubble.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: labelInset.bottom-10),
			imageViewBubble.leadingAnchor.constraint(equalTo: contentBubble.leadingAnchor, constant: labelInset.left+10),
			imageViewBubble.trailingAnchor.constraint(equalTo: contentBubble.trailingAnchor, constant: labelInset.right-10),
		]
		imageViewBubble.isHidden = true
		
	//Video
		bubble.addSubview(imageVideoViewBubble)
		imageVideoViewBubble.translatesAutoresizingMaskIntoConstraints = false
		videoConstraints = [
			imageVideoViewBubble.topAnchor.constraint(equalTo: contentView.topAnchor, constant: labelInset.top+10),
			imageVideoViewBubble.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: labelInset.bottom-10),
			imageVideoViewBubble.leadingAnchor.constraint(equalTo: contentBubble.leadingAnchor, constant: labelInset.left+10),
			imageVideoViewBubble.trailingAnchor.constraint(equalTo: contentBubble.trailingAnchor, constant: labelInset.right-10)
		]
		if #available(iOS 13.0, *) {
			imagePlayViewBubble.image = (UIImage(named: "vr_play")!.withTintColor(.white))
		}
		
		imageVideoViewBubble.addSubview(imagePlayViewBubble)
		playButtonConstraints = [
			imagePlayViewBubble.centerXAnchor.constraint(equalTo: imageVideoViewBubble.centerXAnchor),
			imagePlayViewBubble.centerYAnchor.constraint(equalTo: imageVideoViewBubble.centerYAnchor)
		]
		imagePlayViewBubble.size(w: 40, h: 40).done()
		
		imageVideoViewBubble.isHidden = true
		
	//RecordingPlayer
		bubble.addSubview(recordingView)
		recordingView.translatesAutoresizingMaskIntoConstraints = false
		recordingConstraints = [
			recordingView.topAnchor.constraint(equalTo: contentView.topAnchor, constant: labelInset.top+10),
			recordingView.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: labelInset.bottom-10),
			recordingView.leadingAnchor.constraint(equalTo: contentBubble.leadingAnchor, constant: labelInset.left+10),
			recordingView.trailingAnchor.constraint(equalTo: contentBubble.trailingAnchor, constant: labelInset.right-10)
		]
		recordingView.height(50.0).width(280).done()
		recordingView.isHidden = true

	}
	
	func initPlayerAudio(message: ChatMessage){
		let recordingPlayButton = CallControlButton(width: 40, height: 40, buttonTheme:VoipTheme.nav_button("vr_play"))
		let recordingStopButton = CallControlButton(width: 40, height: 40, buttonTheme:VoipTheme.nav_button("vr_stop"))
		let recordingWaveView = UIProgressView()
		let recordingDurationTextView = StyledLabel(VoipTheme.chat_conversation_recording_duration)
		let recordingWaveImage = UIImageView(image: UIImage(named: "vr_wave.png"))
		
		recordingView.addSubview(recordingWaveView)
		recordingWaveView.translatesAutoresizingMaskIntoConstraints = false
		recordingWaveConstraints = [
			recordingWaveView.topAnchor.constraint(equalTo: contentView.topAnchor, constant: labelInset.top+10),
			recordingWaveView.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: labelInset.bottom-10),
			recordingWaveView.leadingAnchor.constraint(equalTo: contentBubble.leadingAnchor, constant: labelInset.left+10),
			recordingWaveView.trailingAnchor.constraint(equalTo: contentBubble.trailingAnchor, constant: labelInset.right-10)
		]
		
		recordingWaveView.progressViewStyle = .bar
		recordingWaveView.layer.cornerRadius = 5
		recordingWaveView.clipsToBounds = true
		
		recordingWaveView.addSubview(recordingPlayButton)
		recordingPlayButton.alignParentLeft(withMargin: 10).matchParentHeight().done()
		
		recordingWaveView.addSubview(recordingStopButton)
		recordingStopButton.alignParentLeft(withMargin: 10).matchParentHeight().done()
		recordingStopButton.isHidden = true
		
		recordingWaveView.addSubview(recordingWaveImage)
		recordingWaveImage.alignParentLeft(withMargin: 60).alignParentRight(withMargin: 60).height(26).alignHorizontalCenterWith(recordingView).done()
		
		recordingWaveView.addSubview(recordingDurationTextView)
		recordingDurationTextView.alignParentRight(withMargin: 10).matchParentHeight().done()
		
		let img = message.isOutgoing ? UIImage.withColor(UIColor("A")) : UIImage.withColor(UIColor("D"))
		recordingWaveView.progressImage = img
		
		//recordingWaveView.progressTintColor = message.isOutgoing ? UIColor("A").withAlphaComponent(1.0) : UIColor("D").withAlphaComponent(1.0)
		
		recordingDurationTextView.text = recordingDuration(message.contents.first?.filePath)
		
		recordingPlayButton.onClickAction = {
			self.playRecordedMessage(voiceRecorder: message.contents.first?.filePath, recordingPlayButton: recordingPlayButton, recordingStopButton: recordingStopButton, recordingWaveView: recordingWaveView, message: message)
		}
		recordingStopButton.onClickAction = {
			self.stopVoiceRecordPlayer(recordingPlayButton: recordingPlayButton, recordingStopButton: recordingStopButton, recordingWaveView: recordingWaveView, message: message)
		}
		
		NSLayoutConstraint.deactivate(labelConstraints)
		NSLayoutConstraint.deactivate(imageConstraints)
		NSLayoutConstraint.deactivate(videoConstraints)
		NSLayoutConstraint.deactivate(playButtonConstraints)
		NSLayoutConstraint.activate(recordingConstraints)
		NSLayoutConstraint.activate(recordingWaveConstraints)
		label.isHidden = true
		imageViewBubble.isHidden = true
		imageVideoViewBubble.isHidden = true
		recordingView.isHidden = false
		
		imageViewBubble.image = nil
		imageVideoViewBubble.image = nil
	}
	
	required init?(coder aDecoder: NSCoder) {
		fatalError("Storyboards are quicker, easier, more seductive. Not stronger then Code.")
	}
	
	override func prepareForReuse() {
		super.prepareForReuse()
	}
	
	func configure(message: ChatMessage, isBasic: Bool) {
		
	/*
		For Multimedia
		message.contents.forEach { content in
			label.text = content.utf8Text
		}
	*/
		if !message.isOutgoing {
			constraintLeadingBubble?.isActive = true
			constraintTrailingBubble?.isActive = false
			imageUser.isHidden = false
			bubble.backgroundColor = UIColor("D").withAlphaComponent(0.2)
			chatRead.isHidden = true
		}else{
			constraintLeadingBubble?.isActive = false
			constraintTrailingBubble?.isActive = true
			imageUser.isHidden = true
			bubble.backgroundColor = UIColor("A").withAlphaComponent(0.2)
			chatRead.isHidden = false
		}
		
		if isBasic {
			if message.contents.first?.type == "text"{
				label.text = message.contents.first?.utf8Text.trimmingCharacters(in: .whitespacesAndNewlines)
				
				NSLayoutConstraint.activate(labelConstraints)
				NSLayoutConstraint.deactivate(imageConstraints)
				NSLayoutConstraint.deactivate(videoConstraints)
				NSLayoutConstraint.deactivate(playButtonConstraints)
				NSLayoutConstraint.deactivate(recordingConstraints)
				NSLayoutConstraint.deactivate(recordingWaveConstraints)
				label.isHidden = false
				imageViewBubble.isHidden = true
				imageVideoViewBubble.isHidden = true
				recordingView.isHidden = true
				
				imageViewBubble.image = nil
				imageVideoViewBubble.image = nil
				
			}else if message.contents.first?.type == "image"{
				if let imageMessage = UIImage(named: message.contents.first!.filePath){
					imageViewBubble.image = resizeImage(image: imageMessage, targetSize: CGSizeMake(UIScreen.main.bounds.size.width*3/4, 300.0))
				}
				
				NSLayoutConstraint.deactivate(labelConstraints)
				NSLayoutConstraint.activate(imageConstraints)
				NSLayoutConstraint.deactivate(videoConstraints)
				NSLayoutConstraint.deactivate(playButtonConstraints)
				NSLayoutConstraint.deactivate(recordingConstraints)
				NSLayoutConstraint.deactivate(recordingWaveConstraints)
				label.isHidden = true
				imageViewBubble.isHidden = false
				imageVideoViewBubble.isHidden = true
				recordingView.isHidden = true
				
				imageVideoViewBubble.image = nil
				
			}else if message.contents.first?.type == "video"{
				if let imageMessage = createThumbnailOfVideoFromFileURL(videoURL: message.contents.first!.filePath){
					imageVideoViewBubble.image = resizeImage(image: imageMessage, targetSize: CGSizeMake(UIScreen.main.bounds.size.width*3/4, 300.0))
				}
				
				NSLayoutConstraint.deactivate(labelConstraints)
				NSLayoutConstraint.deactivate(imageConstraints)
				NSLayoutConstraint.activate(videoConstraints)
				NSLayoutConstraint.activate(playButtonConstraints)
				NSLayoutConstraint.deactivate(recordingConstraints)
				NSLayoutConstraint.deactivate(recordingWaveConstraints)
				label.isHidden = true
				imageViewBubble.isHidden = true
				imageVideoViewBubble.isHidden = false
				recordingView.isHidden = true
				
				imageViewBubble.image = nil
				
			}else if message.contents.first?.type == "audio"{
				
				recordingView.subviews.forEach({ view in
					view.removeFromSuperview()
				})
				initPlayerAudio(message: message)
				
			}else{
				//createBubbleOthe()
			}
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
	func playRecordedMessage(voiceRecorder: String?, recordingPlayButton: CallControlButton, recordingStopButton: CallControlButton, recordingWaveView: UIProgressView, message: ChatMessage) {
		AudioPlayer.initSharedPlayer()
		AudioPlayer.sharedModel.fileChanged.value = voiceRecorder
		recordingPlayButton.isHidden = true
		recordingStopButton.isHidden = false
		
		AudioPlayer.startSharedPlayer(voiceRecorder)
		recordingWaveView.progress = 0.0
		isPlayingVoiceRecording = true
		
		AudioPlayer.sharedModel.fileChanged.observe { file in
			if (file != voiceRecorder && self.isPlayingVoiceRecording) {
				self.stopVoiceRecordPlayer(recordingPlayButton: recordingPlayButton, recordingStopButton: recordingStopButton, recordingWaveView: recordingWaveView, message: message)
			}
		}
		
		recordingWaveView.progress = 1.0
		UIView.animate(withDuration: TimeInterval(Double(AudioPlayer.getSharedPlayer()!.duration) / 1000.00), delay: 0.0, options: .curveLinear, animations: {
			recordingWaveView.layoutIfNeeded()
		}, completion: { (finished: Bool) in
			if (self.isPlayingVoiceRecording) {
				self.stopVoiceRecordPlayer(recordingPlayButton: recordingPlayButton, recordingStopButton: recordingStopButton, recordingWaveView: recordingWaveView, message: message)
			}
		})
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
	
	func stopVoiceRecordPlayer(recordingPlayButton: CallControlButton, recordingStopButton: CallControlButton, recordingWaveView: UIProgressView, message: ChatMessage) {
		print("MultilineMessageCell stopVoiceRecordPlayer")
		recordingView.subviews.forEach({ view in
			view.removeFromSuperview()
		})
		if(!recordingView.isHidden){
			initPlayerAudio(message: message)
		}
		recordingWaveView.progress = 0.0
		recordingWaveView.setProgress(recordingWaveView.progress, animated: false)
		AudioPlayer.stopSharedPlayer()
		recordingPlayButton.isHidden = false
		recordingStopButton.isHidden = true
		isPlayingVoiceRecording = false
	}
}
