//
//  MultilineMessageCell.swift
//  linphone
//
//  Created by Benoît Martins on 01/03/2023.
//

import UIKit
import Foundation
import linphonesw

class MultilineMessageCell: UICollectionViewCell, UICollectionViewDataSource, UICollectionViewDelegate {
	static let reuseId = "MultilineMessageCellReuseId"
	
	private let label: UILabel = UILabel(frame: .zero)
	private let preContentViewBubble: UIView = UIView(frame: .zero)
	private let contentViewBubble: UIView = UIView(frame: .zero)
	private let contentBubble: UIView = UIView(frame: .zero)
	private let bubble: UIView = UIView(frame: .zero)
	private let imageUser: UIView = UIView(frame: .zero)
	private let chatRead = UIImageView(image: UIImage(named: "chat_read.png"))

	let labelInset = UIEdgeInsets(top: 10, left: 10, bottom: -10, right: -10)
	
	var constraintLeadingBubble : NSLayoutConstraint? = nil
	var constraintTrailingBubble : NSLayoutConstraint? = nil
	var preContentViewBubbleConstraints : [NSLayoutConstraint] = []
	var preContentViewBubbleConstraintsHidden : [NSLayoutConstraint] = []
	var contentViewBubbleConstraints : [NSLayoutConstraint] = []
	var forwardConstraints : [NSLayoutConstraint] = []
	var replyConstraints : [NSLayoutConstraint] = []
	var labelConstraints: [NSLayoutConstraint] = []
	var imageConstraints: [NSLayoutConstraint] = []
	var videoConstraints: [NSLayoutConstraint] = []
	var playButtonConstraints: [NSLayoutConstraint] = []
	var recordingConstraints: [NSLayoutConstraint] = []
	var recordingWaveConstraints: [NSLayoutConstraint] = []
	var meetingConstraints: [NSLayoutConstraint] = []
	
	let forwardView = UIView()
	let forwardIcon = UIImageView(image: UIImage(named: "menu_forward_default"))
	let forwardLabel = StyledLabel(VoipTheme.chat_conversation_forward_label)
	
	let replyView = UIView()
	let replyIcon = UIImageView(image: UIImage(named: "menu_reply_default"))
	let replyLabel = StyledLabel(VoipTheme.chat_conversation_forward_label)
	let replyContent = UIView()
	let replyColorContent = UIView()
	let replyLabelContent = StyledLabel(VoipTheme.chat_conversation_forward_label)
	var stackViewReply = UIStackView()
	let replyLabelTextView = StyledLabel(VoipTheme.chat_conversation_reply_label)
	let replyLabelContentTextSpacing = UIView()
	let replyContentTextView = StyledLabel(VoipTheme.chat_conversation_reply_content)
	let replyContentTextSpacing = UIView()
	let replyContentForMeetingTextView = StyledLabel(VoipTheme.chat_conversation_reply_content)
	let replyContentForMeetingSpacing = UIView()
	let replyMeetingSchedule = UIImageView()
	let mediaSelectorReply  = UIView()
	var collectionViewReply: UICollectionView = {
		let collection_view_reply_height = 60.0
		let layout: UICollectionViewFlowLayout = UICollectionViewFlowLayout()
		layout.itemSize = CGSize(width: collection_view_reply_height, height: collection_view_reply_height)

		layout.scrollDirection = .horizontal
		layout.minimumLineSpacing = 4
		layout.minimumInteritemSpacing = 4

		let collectionViewReply = UICollectionView(frame: .zero, collectionViewLayout: layout)
		collectionViewReply.translatesAutoresizingMaskIntoConstraints = false
		collectionViewReply.backgroundColor = .clear
		return collectionViewReply
	}()

	var replyCollectionView : [UIImage] = []
	var replyURLCollection : [URL] = []
	
	let imageViewBubble = UIImageView(image: UIImage(named: "chat_error"))
	let imageVideoViewBubble = UIImageView(image: UIImage(named: "file_video_default"))
	let imagePlayViewBubble = UIImageView(image: UIImage(named: "vr_play"))
	
	let meetingView = UIView()
	
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
		bubble.topAnchor.constraint(equalTo: contentView.topAnchor).isActive = true
		bubble.bottomAnchor.constraint(equalTo: contentView.bottomAnchor).isActive = true
		bubble.leadingAnchor.constraint(equalTo: contentBubble.leadingAnchor).isActive = true
		bubble.trailingAnchor.constraint(equalTo: contentBubble.trailingAnchor).isActive = true
		bubble.layer.cornerRadius = 10.0
		
		contentBubble.addSubview(chatRead)
		chatRead.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -2).isActive = true
		chatRead.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -8).isActive = true
		chatRead.size(w: 10, h: 10).done()
		chatRead.isHidden = true
		
		
		
	//PreContentViewBubble
		bubble.addSubview(preContentViewBubble)
		preContentViewBubble.translatesAutoresizingMaskIntoConstraints = false
		preContentViewBubbleConstraints = [
			preContentViewBubble.topAnchor.constraint(equalTo: contentBubble.topAnchor),
			preContentViewBubble.leadingAnchor.constraint(equalTo: contentBubble.leadingAnchor, constant: 0),
			preContentViewBubble.trailingAnchor.constraint(equalTo: contentBubble.trailingAnchor, constant: -16),
		]
		preContentViewBubbleConstraintsHidden = [
			preContentViewBubble.topAnchor.constraint(equalTo: contentBubble.topAnchor),
			preContentViewBubble.heightAnchor.constraint(equalToConstant: 0)
		]
		
	//Forward
		preContentViewBubble.addSubview(forwardView)
		forwardView.size(w: 90, h: 10).done()
		
		forwardView.addSubview(forwardIcon)
		forwardIcon.size(w: 10, h: 10).done()
		
		forwardView.addSubview(forwardLabel)
		forwardLabel.text = VoipTexts.bubble_chat_transferred
		forwardLabel.size(w: 90, h: 10).done()
		forwardConstraints = [
			forwardView.topAnchor.constraint(equalTo: preContentViewBubble.topAnchor, constant: 0),
			forwardView.bottomAnchor.constraint(equalTo: preContentViewBubble.bottomAnchor, constant: 0),
			forwardView.leadingAnchor.constraint(equalTo: preContentViewBubble.leadingAnchor, constant: 0),
			forwardView.trailingAnchor.constraint(equalTo: preContentViewBubble.trailingAnchor, constant: 0),
			
			forwardIcon.topAnchor.constraint(equalTo: preContentViewBubble.topAnchor, constant: 6),
			forwardIcon.leadingAnchor.constraint(equalTo: preContentViewBubble.leadingAnchor, constant: 6),
			
			forwardLabel.topAnchor.constraint(equalTo: preContentViewBubble.topAnchor, constant: 6),
			forwardLabel.leadingAnchor.constraint(equalTo: preContentViewBubble.leadingAnchor, constant: 20),
			forwardLabel.trailingAnchor.constraint(equalTo: preContentViewBubble.trailingAnchor, constant: 0)
		]
		forwardView.isHidden = true
		
	//Reply
		preContentViewBubble.addSubview(replyView)
		replyView.size(w: 90, h: 10).done()
		
		replyView.addSubview(replyIcon)
		replyIcon.size(w: 10, h: 10).done()
		
		replyView.addSubview(replyLabel)
		replyLabel.text = VoipTexts.bubble_chat_reply
		replyLabel.size(w: 90, h: 10).done()
		
		preContentViewBubble.addSubview(replyContent)
		//replyContent.maxHeight(100).done()
		replyContent.minWidth(200).done()
		replyContent.layer.cornerRadius = 5
		replyContent.clipsToBounds = true
		replyContent.translatesAutoresizingMaskIntoConstraints = false
		
		replyContent.addSubview(replyColorContent)
		replyColorContent.width(10).done()
		replyColorContent.layer.cornerRadius = 5
		replyColorContent.clipsToBounds = true
		replyColorContent.layer.maskedCorners = [.layerMinXMinYCorner, .layerMinXMaxYCorner]
		
		initReplyView()
		
		replyConstraints = [
			replyView.topAnchor.constraint(equalTo: preContentViewBubble.topAnchor, constant: 0),
			replyView.leadingAnchor.constraint(equalTo: preContentViewBubble.leadingAnchor, constant: 0),
			
			replyIcon.topAnchor.constraint(equalTo: preContentViewBubble.topAnchor, constant: 6),
			replyIcon.leadingAnchor.constraint(equalTo: preContentViewBubble.leadingAnchor, constant: 6),
			
			replyLabel.topAnchor.constraint(equalTo: preContentViewBubble.topAnchor, constant: 6),
			replyLabel.leadingAnchor.constraint(equalTo: preContentViewBubble.leadingAnchor, constant: 20),
			
			replyContent.topAnchor.constraint(equalTo: preContentViewBubble.topAnchor, constant: 20),
			replyContent.leadingAnchor.constraint(equalTo: preContentViewBubble.leadingAnchor, constant: 8),
			replyContent.trailingAnchor.constraint(equalTo: preContentViewBubble.trailingAnchor, constant: 8),
			replyContent.bottomAnchor.constraint(equalTo: preContentViewBubble.bottomAnchor, constant: 0),
			
			replyColorContent.topAnchor.constraint(equalTo: replyContent.topAnchor),
			replyColorContent.bottomAnchor.constraint(equalTo: replyContent.bottomAnchor),
			
			stackViewReply.topAnchor.constraint(equalTo: replyContent.topAnchor, constant: 4),
			stackViewReply.bottomAnchor.constraint(equalTo: replyContent.bottomAnchor, constant: -4),
			stackViewReply.leadingAnchor.constraint(equalTo: replyContent.leadingAnchor, constant: 14),
			stackViewReply.trailingAnchor.constraint(equalTo: replyContent.trailingAnchor, constant: 14),
			stackViewReply.widthAnchor.constraint(equalTo: replyContent.widthAnchor),
			
			replyContentTextView.leadingAnchor.constraint(equalTo: stackViewReply.leadingAnchor, constant: 0),
			replyContentTextView.trailingAnchor.constraint(equalTo: stackViewReply.trailingAnchor, constant: -20),
			
			mediaSelectorReply.leadingAnchor.constraint(equalTo: stackViewReply.leadingAnchor, constant: 0),
			mediaSelectorReply.trailingAnchor.constraint(equalTo: stackViewReply.trailingAnchor, constant: -20),
			
			collectionViewReply.topAnchor.constraint(equalTo: mediaSelectorReply.topAnchor),
			collectionViewReply.bottomAnchor.constraint(equalTo: mediaSelectorReply.bottomAnchor),
			collectionViewReply.leadingAnchor.constraint(equalTo: mediaSelectorReply.leadingAnchor),
			collectionViewReply.trailingAnchor.constraint(equalTo: mediaSelectorReply.trailingAnchor),
		]
		
		replyView.isHidden = true
		
		
		
	//ContentViewBubble
		bubble.addSubview(contentViewBubble)
		contentViewBubble.translatesAutoresizingMaskIntoConstraints = false
		contentViewBubbleConstraints = [
			//contentViewBubble.topAnchor.constraint(equalTo: contentView.topAnchor),
			contentViewBubble.topAnchor.constraint(equalTo: preContentViewBubble.bottomAnchor),
			contentViewBubble.bottomAnchor.constraint(equalTo: contentView.bottomAnchor),
			contentViewBubble.leadingAnchor.constraint(equalTo: contentBubble.leadingAnchor),
			contentViewBubble.trailingAnchor.constraint(equalTo: contentBubble.trailingAnchor)
		]
		NSLayoutConstraint.activate(contentViewBubbleConstraints)
		
	//Text
		label.numberOfLines = 0
		label.lineBreakMode = .byWordWrapping
		
		contentViewBubble.addSubview(label)
		label.translatesAutoresizingMaskIntoConstraints = false
		labelConstraints = [
			label.topAnchor.constraint(equalTo: contentViewBubble.topAnchor, constant: labelInset.top),
			label.bottomAnchor.constraint(equalTo: contentViewBubble.bottomAnchor, constant: labelInset.bottom),
			label.leadingAnchor.constraint(equalTo: contentViewBubble.leadingAnchor, constant: labelInset.left),
			label.trailingAnchor.constraint(equalTo: contentViewBubble.trailingAnchor, constant: labelInset.right)
		]
		NSLayoutConstraint.activate(labelConstraints)

	//Image
		contentViewBubble.addSubview(imageViewBubble)
		imageViewBubble.translatesAutoresizingMaskIntoConstraints = false
		imageConstraints = [
			imageViewBubble.topAnchor.constraint(equalTo: contentViewBubble.topAnchor, constant: labelInset.top),
			imageViewBubble.bottomAnchor.constraint(equalTo: contentViewBubble.bottomAnchor, constant: labelInset.bottom),
			imageViewBubble.leadingAnchor.constraint(equalTo: contentViewBubble.leadingAnchor, constant: labelInset.left),
			imageViewBubble.trailingAnchor.constraint(equalTo: contentViewBubble.trailingAnchor, constant: labelInset.right),
		]
		imageViewBubble.isHidden = true
		
	//Video
		contentViewBubble.addSubview(imageVideoViewBubble)
		imageVideoViewBubble.translatesAutoresizingMaskIntoConstraints = false
		videoConstraints = [
			imageVideoViewBubble.topAnchor.constraint(equalTo: contentViewBubble.topAnchor, constant: labelInset.top),
			imageVideoViewBubble.bottomAnchor.constraint(equalTo: contentViewBubble.bottomAnchor, constant: labelInset.bottom),
			imageVideoViewBubble.leadingAnchor.constraint(equalTo: contentViewBubble.leadingAnchor, constant: labelInset.left),
			imageVideoViewBubble.trailingAnchor.constraint(equalTo: contentViewBubble.trailingAnchor, constant: labelInset.right)
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
		contentViewBubble.addSubview(recordingView)
		recordingView.translatesAutoresizingMaskIntoConstraints = false
		recordingConstraints = [
			recordingView.topAnchor.constraint(equalTo: contentViewBubble.topAnchor, constant: labelInset.top),
			recordingView.bottomAnchor.constraint(equalTo: contentViewBubble.bottomAnchor, constant: labelInset.bottom),
			recordingView.leadingAnchor.constraint(equalTo: contentViewBubble.leadingAnchor, constant: labelInset.left),
			recordingView.trailingAnchor.constraint(equalTo: contentViewBubble.trailingAnchor, constant: labelInset.right)
		]
		recordingView.height(50.0).width(280).done()
		recordingView.isHidden = true
		
	//Meeting
		contentViewBubble.addSubview(meetingView)
		meetingView.translatesAutoresizingMaskIntoConstraints = false
		meetingConstraints = [
			meetingView.topAnchor.constraint(equalTo: contentViewBubble.topAnchor, constant: labelInset.top),
			meetingView.bottomAnchor.constraint(equalTo: contentViewBubble.bottomAnchor, constant: labelInset.bottom),
			meetingView.leadingAnchor.constraint(equalTo: contentViewBubble.leadingAnchor, constant: labelInset.left),
			meetingView.trailingAnchor.constraint(equalTo: contentViewBubble.trailingAnchor, constant: labelInset.right)
		]
		meetingView.isHidden = true
		
		UIDeviceBridge.displayModeSwitched.readCurrentAndObserve { _ in
			self.replyContent.backgroundColor = VoipTheme.backgroundWhiteBlack.get()
		}
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
			recordingWaveView.topAnchor.constraint(equalTo: contentViewBubble.topAnchor, constant: labelInset.top),
			recordingWaveView.bottomAnchor.constraint(equalTo: contentViewBubble.bottomAnchor, constant: labelInset.bottom),
			recordingWaveView.leadingAnchor.constraint(equalTo: contentViewBubble.leadingAnchor, constant: labelInset.left),
			recordingWaveView.trailingAnchor.constraint(equalTo: contentViewBubble.trailingAnchor, constant: labelInset.right)
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
	
	func initReplyView(){
	//Reply - Contents
		
		stackViewReply.axis = .vertical;
		stackViewReply.distribution = .fill;
		stackViewReply.alignment = .leading;
		stackViewReply.maxWidth((UIScreen.main.bounds.size.width*3/4)).done()
		
		replyContent.addSubview(stackViewReply)
		replyContent.backgroundColor = VoipTheme.backgroundWhiteBlack.get()
		stackViewReply.translatesAutoresizingMaskIntoConstraints = false

		stackViewReply.addArrangedSubview(replyLabelTextView)
		replyLabelTextView.height(24).done()
		
		stackViewReply.addArrangedSubview(replyLabelContentTextSpacing)
		replyLabelContentTextSpacing.height(6).wrapContentY().done()
		
		stackViewReply.addArrangedSubview(replyMeetingSchedule)
		replyMeetingSchedule.size(w: 100, h: 40).wrapContentY().done()
		replyMeetingSchedule.contentMode = .scaleAspectFit
		replyMeetingSchedule.isHidden = true
		
		stackViewReply.addArrangedSubview(replyContentForMeetingSpacing)
		replyContentForMeetingSpacing.height(4).done()
		replyMeetingSchedule.isHidden = true
		
		stackViewReply.addArrangedSubview(replyContentForMeetingTextView)
		replyContentForMeetingTextView.width(100).wrapContentY().done()
		replyContentForMeetingTextView.textAlignment = .center
		replyContentForMeetingTextView.numberOfLines = 5
		replyContentForMeetingTextView.isHidden = true
		
		stackViewReply.addArrangedSubview(replyContentTextView)
		replyContentTextView.wrapContentY().done()
		replyContentTextView.numberOfLines = 5
		
		stackViewReply.addArrangedSubview(replyContentTextSpacing)
		replyContentTextSpacing.height(6).wrapContentY().done()
		replyContentTextSpacing.isHidden = true
		
		stackViewReply.addArrangedSubview(mediaSelectorReply)
		mediaSelectorReply.height(60).done()
		mediaSelectorReply.isHidden = true
		mediaSelectorReply.translatesAutoresizingMaskIntoConstraints = false
		
		mediaSelectorReply.addSubview(collectionViewReply)
		collectionViewReply.dataSource = self
		collectionViewReply.delegate = self
		collectionViewReply.register(UICollectionViewCell.self, forCellWithReuseIdentifier: "cellReplyMessage")
	}
	
	required init?(coder aDecoder: NSCoder) {
		fatalError("Storyboards are quicker, easier, more seductive. Not stronger then Code.")
	}
	
	override func prepareForReuse() {
		super.prepareForReuse()
	}
	
	func configure(message: ChatMessage, isBasic: Bool) {
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
		
		if message.isForward {
			NSLayoutConstraint.activate(preContentViewBubbleConstraints)
			NSLayoutConstraint.activate(forwardConstraints)
			NSLayoutConstraint.deactivate(replyConstraints)
			contentViewBubble.minWidth(90).done()
			forwardView.isHidden = false
			replyView.isHidden = true
		}else if message.isReply{
			NSLayoutConstraint.activate(preContentViewBubbleConstraints)
			NSLayoutConstraint.deactivate(forwardConstraints)
			NSLayoutConstraint.activate(replyConstraints)
			contentViewBubble.minWidth(216).done()
			forwardView.isHidden = true
			replyView.isHidden = false
			replyColorContent.backgroundColor = message.replyMessage!.isOutgoing ? UIColor("A") : UIColor("D")
			
			let isIcal = ICSBubbleView.isConferenceInvitationMessage(cmessage: (message.replyMessage?.getCobject)!)
			let content : String? = (isIcal ? ICSBubbleView.getSubjectFromContent(cmessage: (message.replyMessage?.getCobject)!) : ChatMessage.getSwiftObject(cObject: (message.replyMessage?.getCobject)!).utf8Text)
			let contentList = linphone_chat_message_get_contents(message.replyMessage?.getCobject)
			//replyLabelTextView.text = message.replyMessage!.fromAddress?.displayName
			let fromAdress = FastAddressBook.displayName(for: message.replyMessage!.fromAddress?.getCobject)
			replyLabelTextView.text = String.localizedStringWithFormat(NSLocalizedString("%@", comment: ""), fromAdress!)
			
			replyContentTextView.text = content
			replyContentForMeetingTextView.text = content
			if(isIcal){
				replyMeetingSchedule.image = UIImage(named: "voip_meeting_schedule")
				replyMeetingSchedule.isHidden = false
				replyContentForMeetingTextView.isHidden = false
				replyContentForMeetingSpacing.isHidden = false
				replyContentTextView.isHidden = true
				mediaSelectorReply.isHidden = true
				replyContentTextSpacing.isHidden = true
			}else{

				if(bctbx_list_size(contentList) > 1 || content == ""){
					mediaSelectorReply.isHidden = false
					replyContentTextSpacing.isHidden = true
					ChatMessage.getSwiftObject(cObject: (message.replyMessage?.getCobject)!).contents.forEach({ content in
						if(content.isFile){
							let indexPath = IndexPath(row: replyCollectionView.count, section: 0)
							replyURLCollection.append(URL(string: content.filePath)!)
							replyCollectionView.append(getImageFrom(content.getCobject, filePath: content.filePath, forReplyBubble: true)!)
							collectionViewReply.insertItems(at: [indexPath])
						}else if(content.isText){
							replyContentTextSpacing.isHidden = false
						}
					})
					
				}else{
					mediaSelectorReply.isHidden = true
				}
				replyMeetingSchedule.isHidden = true
				replyContentForMeetingTextView.isHidden = true
				replyContentForMeetingSpacing.isHidden = true
				replyContentTextView.isHidden = false
				
			}
			replyContentTextView.text = message.replyMessage!.contents.first?.utf8Text
		}else{
			NSLayoutConstraint.activate(preContentViewBubbleConstraintsHidden)
			NSLayoutConstraint.deactivate(forwardConstraints)
			NSLayoutConstraint.deactivate(replyConstraints)
			contentViewBubble.minWidth(0).done()
			forwardView.isHidden = true
			replyView.isHidden = true
		}
		let isIcal = ICSBubbleView.isConferenceInvitationMessage(cmessage: (message.getCobject)!)
		if(isIcal){
			
			let icsBubbleView = ICSBubbleView.init()
			icsBubbleView.setFromChatMessageSwift(message: message)
			
			meetingView.addSubview(icsBubbleView)
			icsBubbleView.size(w: 280, h: 200).done()
			
			icsBubbleView.translatesAutoresizingMaskIntoConstraints = false
			let icsConstraints = [
				icsBubbleView.topAnchor.constraint(equalTo: meetingView.topAnchor),
				icsBubbleView.bottomAnchor.constraint(equalTo: meetingView.bottomAnchor),
				icsBubbleView.leadingAnchor.constraint(equalTo: meetingView.leadingAnchor),
				icsBubbleView.trailingAnchor.constraint(equalTo: meetingView.trailingAnchor)
			]
			NSLayoutConstraint.activate(icsConstraints)
			
			
			NSLayoutConstraint.deactivate(labelConstraints)
			NSLayoutConstraint.deactivate(imageConstraints)
			NSLayoutConstraint.deactivate(videoConstraints)
			NSLayoutConstraint.deactivate(playButtonConstraints)
			NSLayoutConstraint.deactivate(recordingConstraints)
			NSLayoutConstraint.deactivate(recordingWaveConstraints)
			NSLayoutConstraint.activate(meetingConstraints)
			label.isHidden = false
			imageViewBubble.isHidden = true
			imageVideoViewBubble.isHidden = true
			recordingView.isHidden = true
			
			imageViewBubble.image = nil
			imageVideoViewBubble.image = nil
			
			meetingView.isHidden = false
			
		}else {
			message.contents.forEach { content in
				if content.type == "text"{
					label.text = content.utf8Text.trimmingCharacters(in: .whitespacesAndNewlines)
					
					NSLayoutConstraint.activate(labelConstraints)
					NSLayoutConstraint.deactivate(imageConstraints)
					NSLayoutConstraint.deactivate(videoConstraints)
					NSLayoutConstraint.deactivate(playButtonConstraints)
					NSLayoutConstraint.deactivate(recordingConstraints)
					NSLayoutConstraint.deactivate(recordingWaveConstraints)
					NSLayoutConstraint.deactivate(meetingConstraints)
					label.isHidden = false
					imageViewBubble.isHidden = true
					imageVideoViewBubble.isHidden = true
					recordingView.isHidden = true
					
					imageViewBubble.image = nil
					imageVideoViewBubble.image = nil
					
					meetingView.isHidden = true
					
				}else if content.type == "image"{
					if let imageMessage = UIImage(named: content.filePath){
						imageViewBubble.image = resizeImage(image: imageMessage, targetSize: CGSizeMake(UIScreen.main.bounds.size.width*3/4, 300.0))
					}
					
					NSLayoutConstraint.deactivate(labelConstraints)
					NSLayoutConstraint.activate(imageConstraints)
					NSLayoutConstraint.deactivate(videoConstraints)
					NSLayoutConstraint.deactivate(playButtonConstraints)
					NSLayoutConstraint.deactivate(recordingConstraints)
					NSLayoutConstraint.deactivate(recordingWaveConstraints)
					NSLayoutConstraint.deactivate(meetingConstraints)
					label.isHidden = true
					imageViewBubble.isHidden = false
					imageVideoViewBubble.isHidden = true
					recordingView.isHidden = true
					
					imageVideoViewBubble.image = nil
					
					meetingView.isHidden = true
					
				}else if content.type == "video"{
					if let imageMessage = createThumbnailOfVideoFromFileURL(videoURL: content.filePath){
						imageVideoViewBubble.image = resizeImage(image: imageMessage, targetSize: CGSizeMake(UIScreen.main.bounds.size.width*3/4, 300.0))
					}
					
					NSLayoutConstraint.deactivate(labelConstraints)
					NSLayoutConstraint.deactivate(imageConstraints)
					NSLayoutConstraint.activate(videoConstraints)
					NSLayoutConstraint.activate(playButtonConstraints)
					NSLayoutConstraint.deactivate(recordingConstraints)
					NSLayoutConstraint.deactivate(recordingWaveConstraints)
					NSLayoutConstraint.deactivate(meetingConstraints)
					label.isHidden = true
					imageViewBubble.isHidden = true
					imageVideoViewBubble.isHidden = false
					recordingView.isHidden = true
					
					imageViewBubble.image = nil
					
					meetingView.isHidden = true
					
				}else if content.type == "audio"{
					
					recordingView.subviews.forEach({ view in
						view.removeFromSuperview()
					})
					initPlayerAudio(message: message)
					
				}else{
					//createBubbleOther()
				}}
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
	
	func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
		return replyCollectionView.count
	}

	@objc(collectionView:cellForItemAtIndexPath:) func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
		let cell = collectionView.dequeueReusableCell(withReuseIdentifier: "cellReplyMessage", for: indexPath)
		let viewCell: UIView = UIView(frame: cell.contentView.frame)
		cell.addSubview(viewCell)
		let imageCell = replyCollectionView[indexPath.row]
		var myImageView = UIImageView()
		
		if(FileType.init(replyURLCollection[indexPath.row].pathExtension)?.getGroupTypeFromFile() == FileType.file_picture_default.rawValue || FileType.init(replyURLCollection[indexPath.row].pathExtension)?.getGroupTypeFromFile() == FileType.file_video_default.rawValue){
			myImageView = UIImageView(image: imageCell)
		}else{
			let fileNameText = replyURLCollection[indexPath.row].lastPathComponent
			let fileName = SwiftUtil.textToImage(drawText:fileNameText, inImage:imageCell, forReplyBubble:false)
			myImageView = UIImageView(image: fileName)
		}
		
		myImageView.size(w: (viewCell.frame.width), h: (viewCell.frame.height)).done()
		viewCell.addSubview(myImageView)
		
		if(FileType.init(replyURLCollection[indexPath.row].pathExtension)?.getGroupTypeFromFile() == FileType.file_video_default.rawValue){
			var imagePlay = UIImage()
			if #available(iOS 13.0, *) {
				imagePlay = (UIImage(named: "vr_play")!.withTintColor(.white))
			} else {
				imagePlay = UIImage(named: "vr_play")!
			}
			let myImagePlayView = UIImageView(image: imagePlay)
			viewCell.addSubview(myImagePlayView)
			myImagePlayView.size(w: viewCell.frame.width/4, h: viewCell.frame.height/4).done()
			myImagePlayView.alignHorizontalCenterWith(viewCell).alignVerticalCenterWith(viewCell).done()
		}
		myImageView.contentMode = .scaleAspectFill
		myImageView.clipsToBounds = true
		
		return cell
	}
	
	func getImageFrom(_ content: OpaquePointer?, filePath: String?, forReplyBubble: Bool) -> UIImage? {
		var filePath = filePath
		let type = String(utf8String: linphone_content_get_type(content))
		let name = String(utf8String: linphone_content_get_name(content))
		if filePath == nil {
			filePath = LinphoneManager.validFilePath(name)
		}

		var image: UIImage? = nil
		if type == "video" {
			image = UIChatBubbleTextCell.getImageFromVideoUrl(URL(fileURLWithPath: filePath ?? ""))
		} else if type == "image" {
			let data = NSData(contentsOfFile: filePath ?? "") as Data?
			if let data {
				image = UIImage(data: data)
			}
		}
		if let image {
			return image
		} else {
			return getImageFromFileName(name, forReplyBubble: forReplyBubble)
		}
	}
	
	func getImageFromFileName(_ fileName: String?, forReplyBubble forReplyBubbble: Bool) -> UIImage? {
		let `extension` = fileName?.lowercased().components(separatedBy: ".").last
		var image: UIImage?
		var text = fileName
		if fileName?.contains("voice-recording") ?? false {
			image = UIImage(named: "file_voice_default")
			text = recordingDuration(LinphoneManager.validFilePath(fileName))
		} else {
			if `extension` == "pdf" {
				image = UIImage(named: "file_pdf_default")
			} else if ["png", "jpg", "jpeg", "bmp", "heic"].contains(`extension` ?? "") {
				image = UIImage(named: "file_picture_default")
			} else if ["mkv", "avi", "mov", "mp4"].contains(`extension` ?? "") {
				image = UIImage(named: "file_video_default")
			} else if ["wav", "au", "m4a"].contains(`extension` ?? "") {
				image = UIImage(named: "file_audio_default")
			} else {
				image = UIImage(named: "file_default")
			}
		}

		return SwiftUtil.textToImage(drawText: text!, inImage: image!, forReplyBubble: forReplyBubbble)
	}
}
