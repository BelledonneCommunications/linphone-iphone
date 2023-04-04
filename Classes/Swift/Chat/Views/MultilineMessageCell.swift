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
	
	let label: UILabel = UILabel(frame: .zero)
	let eventMessageView: UIView = UIView(frame: .zero)
	let preContentViewBubble: UIView = UIView(frame: .zero)
	let contentViewBubble: UIView = UIView(frame: .zero)
	let contentMediaViewBubble: UIView = UIView(frame: .zero)
	let contentBubble: UIView = UIView(frame: .zero)
	let bubble: UIView = UIView(frame: .zero)
	let imageUser = Avatar(color:VoipTheme.primaryTextColor, textStyle: VoipTheme.chat_conversation_avatar_small)
	let contactDateLabel = StyledLabel(VoipTheme.chat_conversation_forward_label)
	let chatRead = UIImageView(image: UIImage(named: "chat_delivered.png"))

	let labelInset = UIEdgeInsets(top: 10, left: 10, bottom: -10, right: -10)
	
	var constraintEventMesssage : [NSLayoutConstraint] = []
	var constraintEventMesssageLabel : [NSLayoutConstraint] = []
	var constraintBubble : [NSLayoutConstraint] = []
	var constraintLeadingBubble : NSLayoutConstraint? = nil
	var constraintTrailingBubble : NSLayoutConstraint? = nil
	var constraintDateLeadingBubble : NSLayoutConstraint? = nil
	var constraintDateTrailingBubble : NSLayoutConstraint? = nil
	var constraintDateBubble : NSLayoutConstraint? = nil
	var constraintDateBubbleHidden : NSLayoutConstraint? = nil
	var preContentViewBubbleConstraints : [NSLayoutConstraint] = []
	var preContentViewBubbleConstraintsHidden : [NSLayoutConstraint] = []
	var contentViewBubbleConstraints : [NSLayoutConstraint] = []
	var contentMediaViewBubbleConstraints : [NSLayoutConstraint] = []
	var forwardConstraints : [NSLayoutConstraint] = []
	var replyConstraints : [NSLayoutConstraint] = []
	var labelConstraints: [NSLayoutConstraint] = []
	var labelTopConstraints: [NSLayoutConstraint] = []
	var labelHiddenConstraints: [NSLayoutConstraint] = []
	var imagesGridConstraints : [NSLayoutConstraint] = []
	var imageConstraints: [NSLayoutConstraint] = []
	var videoConstraints: [NSLayoutConstraint] = []
	var playButtonConstraints: [NSLayoutConstraint] = []
	var recordingConstraints: [NSLayoutConstraint] = []
	var recordingWaveConstraints: [NSLayoutConstraint] = []
	var meetingConstraints: [NSLayoutConstraint] = []
	
	let eventMessageLineView: UIView = UIView(frame: .zero)
	let eventMessageLabelView: UIView = UIView(frame: .zero)
	let eventMessageLabel = StyledLabel(VoipTheme.chat_conversation_forward_label)
	
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
	var collectionViewImagesGrid: DynamicHeightCollectionView = {
		let collection_view_reply_height = 138.0
		let layout: UICollectionViewFlowLayout = UICollectionViewFlowLayout()
		layout.itemSize = CGSize(width: collection_view_reply_height, height: collection_view_reply_height)
		layout.scrollDirection = .vertical
		layout.minimumLineSpacing = 4
		layout.minimumInteritemSpacing = 4

		let collectionViewImagesGrid = DynamicHeightCollectionView(frame: .zero, collectionViewLayout: layout)
		collectionViewImagesGrid.translatesAutoresizingMaskIntoConstraints = false
		collectionViewImagesGrid.backgroundColor = .clear
		return collectionViewImagesGrid
	}()

	var replyCollectionView : [UIImage] = []
	var replyURLCollection : [Content] = []
	
	var imagesGridCollectionView : [UIImage?] = []
	var downloadContentCollection: [DownloadMessageCell?] = []
	var uploadContentCollection: [UploadMessageCell?] = []
	
	let imageViewBubble = UIImageView(image: UIImage(named: "chat_error"))
	let imageVideoViewBubble = UIImageView(image: UIImage(named: "file_video_default"))
	let imagePlayViewBubble = UIImageView(image: UIImage(named: "vr_play"))
	
	let meetingView = UIView()
	
	let recordingView = UIView()
	
	let ephemeralIcon = UIImageView(image: UIImage(named: "ephemeral_messages_color_A.png"))
	let ephemeralTimerLabel = StyledLabel(VoipTheme.chat_conversation_ephemeral_timer)
	var ephemeralTimer : Timer? = nil
	
	
	var isPlayingVoiceRecording = false
    
    var chatMessage: ChatMessage?
	var chatMessageDelegate: ChatMessageDelegate? = nil
	
	var indexTransferProgress: Int = -1
	var indexUploadTransferProgress: Int = 0
	
	var selfIndexMessage: Int = -1
	
	var deleteItemCheckBox = StyledCheckBox()
	
	override init(frame: CGRect) {
		super.init(frame: frame)
		
	//CheckBox for select item to delete
		contentView.addSubview(deleteItemCheckBox)
		
	//Event Message
		contentView.addSubview(eventMessageView)
		constraintEventMesssage = [
			eventMessageView.topAnchor.constraint(equalTo: contentView.topAnchor, constant: 0),
			eventMessageView.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: 0),
			eventMessageView.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 0),
			eventMessageView.trailingAnchor.constraint(equalTo: deleteItemCheckBox.leadingAnchor, constant: 0)
		]
		eventMessageView.height(40).done()
		
		eventMessageView.addSubview(eventMessageLineView)
		eventMessageLineView.height(1).alignParentLeft().alignParentRight().matchCenterYOf(view: eventMessageView).done()
		
		eventMessageView.addSubview(eventMessageLabelView)
		eventMessageLabelView.translatesAutoresizingMaskIntoConstraints = false
		eventMessageLabelView.backgroundColor = VoipTheme.backgroundWhiteBlack.get()
		
		eventMessageLabelView.addSubview(eventMessageLabel)
		eventMessageLabel.text = ""
		eventMessageLabel.height(40).matchCenterYOf(view: eventMessageView).matchCenterXOf(view: eventMessageView).done()
		constraintEventMesssageLabel = [
			eventMessageLabel.topAnchor.constraint(equalTo: eventMessageLabelView.topAnchor, constant: 0),
			eventMessageLabel.bottomAnchor.constraint(equalTo: eventMessageLabelView.bottomAnchor, constant: 0),
			eventMessageLabel.leadingAnchor.constraint(equalTo: eventMessageLabelView.leadingAnchor, constant: 6),
			eventMessageLabel.trailingAnchor.constraint(equalTo: eventMessageLabelView.trailingAnchor, constant: -6)
		]
		
		eventMessageView.isHidden = true
		
		
	//Message
		contentView.addSubview(contactDateLabel)
		
		constraintDateBubble = contactDateLabel.topAnchor.constraint(equalTo: contentView.topAnchor, constant: 4)
		constraintDateBubbleHidden = contactDateLabel.topAnchor.constraint(equalTo: contentView.topAnchor)
		constraintDateLeadingBubble = contactDateLabel.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 40)
		constraintDateTrailingBubble = contactDateLabel.trailingAnchor.constraint(equalTo: deleteItemCheckBox.leadingAnchor, constant: -22)
		constraintDateBubble!.isActive = true
		contactDateLabel.isHidden = true
		
		contentView.addSubview(contentBubble)
		contentBubble.translatesAutoresizingMaskIntoConstraints = false
		constraintBubble = [
			contentBubble.topAnchor.constraint(equalTo: contactDateLabel.bottomAnchor, constant: 0),
			contentBubble.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: 0)
		]
		constraintLeadingBubble = contentBubble.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 40)
		constraintTrailingBubble = contentBubble.trailingAnchor.constraint(equalTo: deleteItemCheckBox.leadingAnchor, constant: -22)
		
		NSLayoutConstraint.activate(constraintBubble)
		constraintLeadingBubble!.isActive = true
		
		contentBubble.addSubview(imageUser)
		imageUser.topAnchor.constraint(equalTo: contactDateLabel.bottomAnchor).isActive = true
		imageUser.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 6).isActive = true
		imageUser.layer.cornerRadius = 15.0
		imageUser.size(w: 30, h: 30).done()
		
		contentBubble.addSubview(bubble)
		bubble.translatesAutoresizingMaskIntoConstraints = false
		bubble.topAnchor.constraint(equalTo: contactDateLabel.bottomAnchor).isActive = true
		bubble.bottomAnchor.constraint(equalTo: contentView.bottomAnchor).isActive = true
		bubble.leadingAnchor.constraint(equalTo: contentBubble.leadingAnchor).isActive = true
		bubble.trailingAnchor.constraint(equalTo: contentBubble.trailingAnchor).isActive = true
		bubble.layer.cornerRadius = 10.0
		
		contentBubble.addSubview(chatRead)
		chatRead.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -2).isActive = true
		chatRead.trailingAnchor.constraint(equalTo: deleteItemCheckBox.leadingAnchor, constant: -8).isActive = true
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
			
			forwardIcon.topAnchor.constraint(equalTo: preContentViewBubble.topAnchor, constant: 6),
			forwardIcon.leadingAnchor.constraint(equalTo: preContentViewBubble.leadingAnchor, constant: 6),
			
			forwardLabel.topAnchor.constraint(equalTo: preContentViewBubble.topAnchor, constant: 6),
			forwardLabel.leadingAnchor.constraint(equalTo: preContentViewBubble.leadingAnchor, constant: 20)
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
			contentViewBubble.topAnchor.constraint(equalTo: preContentViewBubble.bottomAnchor),
			contentViewBubble.bottomAnchor.constraint(equalTo: contentView.bottomAnchor),
			contentViewBubble.leadingAnchor.constraint(equalTo: contentBubble.leadingAnchor),
			contentViewBubble.trailingAnchor.constraint(equalTo: contentBubble.trailingAnchor)
		]
		NSLayoutConstraint.activate(contentViewBubbleConstraints)
		
	//Content Media View
		contentViewBubble.addSubview(contentMediaViewBubble)
		contentMediaViewBubble.translatesAutoresizingMaskIntoConstraints = false
		contentMediaViewBubbleConstraints = [
			contentMediaViewBubble.topAnchor.constraint(equalTo: contentViewBubble.topAnchor),
			contentMediaViewBubble.leadingAnchor.constraint(equalTo: contentViewBubble.leadingAnchor),
			contentMediaViewBubble.trailingAnchor.constraint(equalTo: contentViewBubble.trailingAnchor)
		]
		NSLayoutConstraint.activate(contentMediaViewBubbleConstraints)
		
	//Images Grid
		contentMediaViewBubble.addSubview(collectionViewImagesGrid)
		collectionViewImagesGrid.translatesAutoresizingMaskIntoConstraints = false
		imagesGridConstraints = [
			collectionViewImagesGrid.topAnchor.constraint(equalTo: contentMediaViewBubble.topAnchor, constant: labelInset.top),
			collectionViewImagesGrid.bottomAnchor.constraint(equalTo: contentMediaViewBubble.bottomAnchor, constant: labelInset.bottom),
			collectionViewImagesGrid.leadingAnchor.constraint(equalTo: contentMediaViewBubble.leadingAnchor, constant: labelInset.left),
			collectionViewImagesGrid.trailingAnchor.constraint(equalTo: contentMediaViewBubble.trailingAnchor, constant: labelInset.right),
		]
		
		collectionViewImagesGrid.dataSource = self
		collectionViewImagesGrid.delegate = self
		collectionViewImagesGrid.register(UICollectionViewCell.self, forCellWithReuseIdentifier: "cellImagesGridMessage")
		collectionViewImagesGrid.width(280).done()
		collectionViewImagesGrid.isHidden = true

	//Image
		contentMediaViewBubble.addSubview(imageViewBubble)
		imageViewBubble.translatesAutoresizingMaskIntoConstraints = false
		imageConstraints = [
			imageViewBubble.topAnchor.constraint(equalTo: contentMediaViewBubble.topAnchor, constant: labelInset.top),
			imageViewBubble.bottomAnchor.constraint(equalTo: contentMediaViewBubble.bottomAnchor, constant: labelInset.bottom),
			imageViewBubble.leadingAnchor.constraint(equalTo: contentMediaViewBubble.leadingAnchor, constant: labelInset.left),
			imageViewBubble.trailingAnchor.constraint(equalTo: contentMediaViewBubble.trailingAnchor, constant: labelInset.right),
		]
		imageViewBubble.isHidden = true
		
	//Video
		contentMediaViewBubble.addSubview(imageVideoViewBubble)
		imageVideoViewBubble.translatesAutoresizingMaskIntoConstraints = false
		videoConstraints = [
			imageVideoViewBubble.topAnchor.constraint(equalTo: contentMediaViewBubble.topAnchor, constant: labelInset.top),
			imageVideoViewBubble.bottomAnchor.constraint(equalTo: contentMediaViewBubble.bottomAnchor, constant: labelInset.bottom),
			imageVideoViewBubble.leadingAnchor.constraint(equalTo: contentMediaViewBubble.leadingAnchor, constant: labelInset.left),
			imageVideoViewBubble.trailingAnchor.constraint(equalTo: contentMediaViewBubble.trailingAnchor, constant: labelInset.right)
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
		contentMediaViewBubble.addSubview(recordingView)
		recordingView.translatesAutoresizingMaskIntoConstraints = false
		recordingConstraints = [
			recordingView.topAnchor.constraint(equalTo: contentMediaViewBubble.topAnchor, constant: labelInset.top),
			recordingView.bottomAnchor.constraint(equalTo: contentMediaViewBubble.bottomAnchor, constant: labelInset.bottom),
			recordingView.leadingAnchor.constraint(equalTo: contentMediaViewBubble.leadingAnchor, constant: labelInset.left),
			recordingView.trailingAnchor.constraint(equalTo: contentMediaViewBubble.trailingAnchor, constant: labelInset.right)
		]
		recordingView.height(50.0).width(280).done()
		recordingView.isHidden = true
		
	//Text
		label.numberOfLines = 0
		label.lineBreakMode = .byWordWrapping
		
		contentViewBubble.addSubview(label)
		label.translatesAutoresizingMaskIntoConstraints = false
		labelConstraints = [
			label.topAnchor.constraint(equalTo: contentMediaViewBubble.bottomAnchor, constant: labelInset.top),
			label.bottomAnchor.constraint(equalTo: contentViewBubble.bottomAnchor, constant: labelInset.bottom),
			label.leadingAnchor.constraint(equalTo: contentViewBubble.leadingAnchor, constant: labelInset.left),
			label.trailingAnchor.constraint(equalTo: contentViewBubble.trailingAnchor, constant: labelInset.right)
		]
		
		labelTopConstraints = [
			label.topAnchor.constraint(equalTo: contentMediaViewBubble.bottomAnchor),
			label.bottomAnchor.constraint(equalTo: contentViewBubble.bottomAnchor, constant: labelInset.bottom),
			label.leadingAnchor.constraint(equalTo: contentViewBubble.leadingAnchor, constant: labelInset.left),
			label.trailingAnchor.constraint(equalTo: contentViewBubble.trailingAnchor, constant: labelInset.right)
		]
		
		labelHiddenConstraints = [
			label.topAnchor.constraint(equalTo: contentMediaViewBubble.bottomAnchor),
			label.bottomAnchor.constraint(equalTo: contentViewBubble.bottomAnchor)
		]
		
		NSLayoutConstraint.activate(labelConstraints)
		
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
		
	//Ephemeral
		contentViewBubble.addSubview(ephemeralTimerLabel)
		ephemeralTimerLabel.bottomAnchor.constraint(equalTo: contentViewBubble.bottomAnchor, constant: -2).isActive = true
		ephemeralTimerLabel.trailingAnchor.constraint(equalTo: contentViewBubble.trailingAnchor, constant: -14).isActive = true
		ephemeralTimerLabel.text = "00:00"
		ephemeralTimerLabel.height(10).done()
		ephemeralTimerLabel.isHidden = true
		
		contentViewBubble.addSubview(ephemeralIcon)
		ephemeralIcon.bottomAnchor.constraint(equalTo: contentViewBubble.bottomAnchor, constant: -3).isActive = true
		ephemeralIcon.trailingAnchor.constraint(equalTo: contentViewBubble.trailingAnchor, constant: -6).isActive = true
		ephemeralIcon.size(w: 7, h: 8).done()
		ephemeralIcon.isHidden = true
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
			recordingWaveView.topAnchor.constraint(equalTo: contentMediaViewBubble.topAnchor, constant: labelInset.top),
			recordingWaveView.bottomAnchor.constraint(equalTo: contentMediaViewBubble.bottomAnchor, constant: labelInset.bottom),
			recordingWaveView.leadingAnchor.constraint(equalTo: contentMediaViewBubble.leadingAnchor, constant: labelInset.left),
			recordingWaveView.trailingAnchor.constraint(equalTo: contentMediaViewBubble.trailingAnchor, constant: labelInset.right)
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
		
		NSLayoutConstraint.activate(recordingConstraints)
		NSLayoutConstraint.activate(recordingWaveConstraints)
		recordingView.isHidden = false
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
	
	func configure(event: EventLog, selfIndexPathConfigure: IndexPath, editMode: Bool, selected: Bool) {
		selfIndexMessage = selfIndexPathConfigure.row
        chatMessage = event.chatMessage
		addMessageDelegate()
		
		if event.chatMessage != nil {
			contentBubble.isHidden = false
			eventMessageView.isHidden = true
			NSLayoutConstraint.activate(constraintBubble)
			NSLayoutConstraint.deactivate(constraintEventMesssage)
			NSLayoutConstraint.deactivate(constraintEventMesssageLabel)
			if !event.chatMessage!.isOutgoing {
				if editMode {
					constraintLeadingBubble?.isActive = false
					constraintTrailingBubble?.isActive = true
				}else{
					constraintLeadingBubble?.isActive = true
					constraintTrailingBubble?.isActive = false
				}
				
				imageUser.isHidden = false
				if isFirstIndexInTableView(indexPath: selfIndexPathConfigure, chat: event.chatMessage!) {
					imageUser.fillFromAddress(address: (event.chatMessage?.fromAddress)!)
					contactDateLabel.text = contactDateForChat(message: event.chatMessage!)
					contactDateLabel.isHidden = false
					if editMode {
						constraintDateTrailingBubble?.isActive = true
						contactDateLabel.textAlignment = .right
					}else{
						constraintDateLeadingBubble?.isActive = true
					}
					contactDateLabel.size(w: 200, h: 20).done()
				}else{
					constraintDateBubble?.isActive = false
					constraintDateBubbleHidden?.isActive = true
					contactDateLabel.size(w: 200, h: 0).done()
				}

				bubble.backgroundColor = UIColor("D").withAlphaComponent(0.2)
			}else{
				constraintLeadingBubble?.isActive = false
				constraintTrailingBubble?.isActive = true
				
				imageUser.isHidden = true
				if isFirstIndexInTableView(indexPath: selfIndexPathConfigure, chat: event.chatMessage!) {
					contactDateLabel.text = LinphoneUtils.time(toString: event.chatMessage!.time, with: LinphoneDateChatBubble)
					contactDateLabel.isHidden = false
					contactDateLabel.textAlignment = .right
					constraintDateTrailingBubble?.isActive = true
					contactDateLabel.size(w: 200, h: 20).done()
				}else{
					constraintDateBubble?.isActive = false
					constraintDateBubbleHidden?.isActive = true
					contactDateLabel.size(w: 200, h: 0).done()
				}
				
				bubble.backgroundColor = UIColor("A").withAlphaComponent(0.2)
                displayImdnStatus(message: event.chatMessage!, state: event.chatMessage!.state)
			}
			
			if event.chatMessage!.isEphemeral {
				ephemeralTimerLabel.isHidden = false
				ephemeralIcon.isHidden = false
				contentViewBubble.minWidth(44).done()
				updateEphemeralTimes()
			}else{
				ephemeralTimerLabel.isHidden = true
				ephemeralIcon.isHidden = true
			}
			
			if event.chatMessage!.isForward {
				NSLayoutConstraint.activate(preContentViewBubbleConstraints)
				NSLayoutConstraint.activate(forwardConstraints)
				NSLayoutConstraint.deactivate(replyConstraints)
				contentViewBubble.minWidth(90).done()
				forwardView.isHidden = false
				replyView.isHidden = true
			}else if event.chatMessage!.isReply{
				NSLayoutConstraint.activate(preContentViewBubbleConstraints)
				NSLayoutConstraint.deactivate(forwardConstraints)
				NSLayoutConstraint.activate(replyConstraints)
				contentViewBubble.minWidth(216).done()
				forwardView.isHidden = true
				replyView.isHidden = false
				
				if(event.chatMessage!.replyMessage != nil){
					replyColorContent.backgroundColor = event.chatMessage!.replyMessage!.isOutgoing ? UIColor("A") : UIColor("D")
					
					let isIcal = ICSBubbleView.isConferenceInvitationMessage(cmessage: (event.chatMessage!.replyMessage?.getCobject)!)
					let content : String? = (isIcal ? ICSBubbleView.getSubjectFromContent(cmessage: (event.chatMessage!.replyMessage?.getCobject)!) : ChatMessage.getSwiftObject(cObject: (event.chatMessage!.replyMessage?.getCobject)!).utf8Text)
					let contentList = linphone_chat_message_get_contents(event.chatMessage!.replyMessage?.getCobject)
					let fromAddress = FastAddressBook.displayName(for: event.chatMessage!.replyMessage!.fromAddress?.getCobject)
					replyLabelTextView.text = String.localizedStringWithFormat(NSLocalizedString("%@", comment: ""), fromAddress!)
					
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
							ChatMessage.getSwiftObject(cObject: (event.chatMessage!.replyMessage?.getCobject)!).contents.forEach({ content in
								if(content.isFile){
									let indexPath = IndexPath(row: replyCollectionView.count, section: 0)
                                    replyURLCollection.append(content)
									replyCollectionView.append(getImageFrom(content, forReplyBubble: false)!)
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
					replyContentTextView.text = event.chatMessage!.replyMessage!.contents.first?.utf8Text
				}else{
					replyLabelTextView.isHidden = true
					replyContentTextSpacing.isHidden = false
					replyContentTextView.text = VoipTexts.bubble_chat_reply_message_does_not_exist + "  "
				}
			}else{
				NSLayoutConstraint.activate(preContentViewBubbleConstraintsHidden)
				NSLayoutConstraint.deactivate(forwardConstraints)
				NSLayoutConstraint.deactivate(replyConstraints)
				contentViewBubble.minWidth(0).done()
				forwardView.isHidden = true
				replyView.isHidden = true
			}
			let isIcal = ICSBubbleView.isConferenceInvitationMessage(cmessage: (event.chatMessage!.getCobject)!)
			if(isIcal){
				
				let icsBubbleView = ICSBubbleView.init()
				icsBubbleView.setFromChatMessage(cmessage: event.chatMessage!.getCobject!)
				
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
				NSLayoutConstraint.deactivate(labelTopConstraints)
				NSLayoutConstraint.activate(labelHiddenConstraints)
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
				NSLayoutConstraint.deactivate(labelConstraints)
				NSLayoutConstraint.deactivate(labelTopConstraints)
				NSLayoutConstraint.activate(labelHiddenConstraints)
				NSLayoutConstraint.deactivate(imagesGridConstraints)
				NSLayoutConstraint.deactivate(imageConstraints)
				NSLayoutConstraint.deactivate(videoConstraints)
				NSLayoutConstraint.deactivate(playButtonConstraints)
				NSLayoutConstraint.deactivate(recordingConstraints)
				NSLayoutConstraint.deactivate(recordingWaveConstraints)
				NSLayoutConstraint.deactivate(meetingConstraints)
				label.isHidden = true
				collectionViewImagesGrid.isHidden = true
				imageViewBubble.isHidden = true
				imageVideoViewBubble.isHidden = true
				recordingView.isHidden = true
				
				imageViewBubble.image = nil
				imageVideoViewBubble.image = nil
				
				meetingView.isHidden = true
                
				event.chatMessage!.contents.forEach { content in
					
                    if content.isFileTransfer {
                    
                        let indexPath = IndexPath(row: imagesGridCollectionView.count, section: 0)
                        imagesGridCollectionView.append(nil)
                        collectionViewImagesGrid.insertItems(at: [indexPath])
                        
                        collectionViewImagesGrid.isHidden = false
                        NSLayoutConstraint.activate(imagesGridConstraints)
                         
                    }
                    
					if content.type == "text"{
						label.text = content.utf8Text.trimmingCharacters(in: .whitespacesAndNewlines)
						if event.chatMessage!.contents.count > 1 {
							NSLayoutConstraint.deactivate(labelConstraints)
							NSLayoutConstraint.activate(labelTopConstraints)
						}else{
							NSLayoutConstraint.activate(labelConstraints)
							NSLayoutConstraint.deactivate(labelTopConstraints)
						}
						
						NSLayoutConstraint.deactivate(labelHiddenConstraints)
						label.isHidden = false
						
					}else if content.type == "image"{
						
						if imagesGridCollectionView.count > 1 {
							if(content.isFile){
								let indexPath = IndexPath(row: imagesGridCollectionView.count, section: 0)
								imagesGridCollectionView.append(getImageFrom(content, forReplyBubble: false)!)
								collectionViewImagesGrid.insertItems(at: [indexPath])
							}
							
							collectionViewImagesGrid.isHidden = false
							NSLayoutConstraint.activate(imagesGridConstraints)
							imageViewBubble.image = nil
							NSLayoutConstraint.deactivate(imageConstraints)
							imageViewBubble.isHidden = true
							
						}else{
							if VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) {
								var plainFile = content.exportPlainFile()
								if let imageMessage = UIImage(named: plainFile){
									imageViewBubble.image = resizeImage(image: imageMessage, targetSize: CGSizeMake(UIScreen.main.bounds.size.width*3/4, 300.0))
								}
								
								ChatConversationViewModel.sharedModel.removeTmpFile(filePath: plainFile)
								plainFile = ""
							}else{
								if let imageMessage = UIImage(named: content.filePath){
									imageViewBubble.image = resizeImage(image: imageMessage, targetSize: CGSizeMake(UIScreen.main.bounds.size.width*3/4, 300.0))
								}
							}
                            
                            if(content.isFile){
                                let indexPath = IndexPath(row: imagesGridCollectionView.count, section: 0)
                                imagesGridCollectionView.append(getImageFrom(content, forReplyBubble: false)!)
                                collectionViewImagesGrid.insertItems(at: [indexPath])
                            }
						}

					}else if content.type == "video"{
						if imagesGridCollectionView.count > 1 {
							if(content.isFile){
								let indexPath = IndexPath(row: imagesGridCollectionView.count, section: 0)
								imagesGridCollectionView.append(getImageFrom(content, forReplyBubble: false)!)
								collectionViewImagesGrid.insertItems(at: [indexPath])
							}
							
							collectionViewImagesGrid.isHidden = false
							NSLayoutConstraint.activate(imagesGridConstraints)
							imageVideoViewBubble.image = nil
							NSLayoutConstraint.deactivate(imageConstraints)
							imageVideoViewBubble.isHidden = true
							
						}else{
							if VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) {
								var plainFile = content.exportPlainFile()
								if let imageMessage = createThumbnailOfVideoFromFileURL(videoURL: plainFile){
									imageVideoViewBubble.image = resizeImage(image: imageMessage, targetSize: CGSizeMake(UIScreen.main.bounds.size.width*3/4, 300.0))
								}
								
								ChatConversationViewModel.sharedModel.removeTmpFile(filePath: plainFile)
								plainFile = ""
							}else{
								if let imageMessage = createThumbnailOfVideoFromFileURL(videoURL: content.filePath){
									imageVideoViewBubble.image = resizeImage(image: imageMessage, targetSize: CGSizeMake(UIScreen.main.bounds.size.width*3/4, 300.0))
								}
							}
							
							if(content.isFile){
								let indexPath = IndexPath(row: imagesGridCollectionView.count, section: 0)
								imagesGridCollectionView.append(getImageFrom(content, forReplyBubble: false)!)
								collectionViewImagesGrid.insertItems(at: [indexPath])
							}
						}
						
					}else if content.type == "audio"{
						
						recordingView.subviews.forEach({ view in
							view.removeFromSuperview()
						})
						initPlayerAudio(message: event.chatMessage!)
						
					}else{
						if(content.isFile){
							let indexPath = IndexPath(row: imagesGridCollectionView.count, section: 0)
							imagesGridCollectionView.append(getImageFrom(content, forReplyBubble: false)!)
							collectionViewImagesGrid.insertItems(at: [indexPath])
						}
						
						collectionViewImagesGrid.isHidden = false
						NSLayoutConstraint.activate(imagesGridConstraints)
					}}
				if imagesGridCollectionView.count > 0 {
					self.collectionViewImagesGrid.layoutIfNeeded()
				}
                if imagesGridCollectionView.count == 1 {
                    collectionViewImagesGrid.width(138).done()
                }
				
				if imagesGridCollectionView.count == 2 {
					collectionViewImagesGrid.isHidden = false
					NSLayoutConstraint.activate(imagesGridConstraints)
					imageVideoViewBubble.image = nil
					NSLayoutConstraint.deactivate(imageConstraints)
					imageVideoViewBubble.isHidden = true
				}
                
                if (imageViewBubble.image != nil && imagesGridCollectionView.count <= 1){
                    NSLayoutConstraint.activate(imageConstraints)
                    imageViewBubble.isHidden = false
                }
				
				if (imageVideoViewBubble.image != nil && imagesGridCollectionView.count <= 1){
					NSLayoutConstraint.activate(videoConstraints)
					NSLayoutConstraint.activate(playButtonConstraints)
					imageVideoViewBubble.isHidden = false
				}
			}
		}else{
			contentBubble.isHidden = true
			NSLayoutConstraint.deactivate(constraintBubble)
			constraintLeadingBubble?.isActive = false
			constraintTrailingBubble?.isActive = false
			imageUser.isHidden = true
			
			eventMessageView.isHidden = false
			NSLayoutConstraint.activate(constraintEventMesssage)
			NSLayoutConstraint.activate(constraintEventMesssageLabel)
			
			eventMessageLabel.text = setEvent(event: event)
			
			if (eventMessageLabel.text == VoipTexts.bubble_chat_event_message_left_group || eventMessageLabel.text!.hasPrefix(VoipTexts.bubble_chat_event_message_max_participant) || eventMessageLabel.text!.hasPrefix(VoipTexts.bubble_chat_event_message_lime_changed) || eventMessageLabel.text!.hasPrefix(VoipTexts.bubble_chat_event_message_attack_detected)) {
				eventMessageLineView.backgroundColor = .red
				eventMessageLabel.textColor = .red
			} else {
				eventMessageLineView.backgroundColor = UIColor("D").withAlphaComponent(0.6)
				eventMessageLabel.textColor = UIColor("D").withAlphaComponent(0.6)
			}
		}
		
		if (editMode) {
			deleteItemCheckBox.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -18).isActive = true
			deleteItemCheckBox.isSelected = selected
			if (event.chatMessage != nil){
				deleteItemCheckBox.matchCenterYOf(view: contentBubble).done()
			}else{
				deleteItemCheckBox.matchCenterYOf(view: contentView).done()
			}
			imageUser.isHidden = true
			contentView.onClick {
				self.deleteItemCheckBox.isSelected = !self.deleteItemCheckBox.isSelected
				ChatConversationTableViewModel.sharedModel.messageListSelected.value![self.selfIndexMessage] = self.deleteItemCheckBox.isSelected
				
				if ChatConversationTableViewModel.sharedModel.messageListSelected.value![self.selfIndexMessage] == true {
					ChatConversationTableViewModel.sharedModel.messageSelected.value! += 1
				}else{
					ChatConversationTableViewModel.sharedModel.messageSelected.value! -= 1
				}
			}
			deleteItemCheckBox.onClick {
				self.deleteItemCheckBox.isSelected = !self.deleteItemCheckBox.isSelected
				ChatConversationTableViewModel.sharedModel.messageListSelected.value![self.selfIndexMessage] = self.deleteItemCheckBox.isSelected
				
				if ChatConversationTableViewModel.sharedModel.messageListSelected.value![self.selfIndexMessage] == true {
					ChatConversationTableViewModel.sharedModel.messageSelected.value! += 1
				}else{
					ChatConversationTableViewModel.sharedModel.messageSelected.value! -= 1
				}
			}
		}else{
			deleteItemCheckBox.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: 0).isActive = true
			deleteItemCheckBox.isHidden = true
			deleteItemCheckBox.width(0).done()
		}
	}
	
	
	func addMessageDelegate(){
		chatMessageDelegate = ChatMessageDelegateStub(
            onMsgStateChanged: { (message: ChatMessage, state: ChatMessage.State) -> Void in
                self.displayImdnStatus(message: message, state: state)
            },
			onFileTransferProgressIndication: { (message: ChatMessage, content: Content, offset: Int, total: Int) -> Void in
				self.file_transfer_progress_indication_recv(message: message, content: content, offset: offset, total: total)
			},
            onParticipantImdnStateChanged: { (message: ChatMessage, state: ParticipantImdnState) -> Void in
                //self.displayImdnStatus(message: message, state: state)
            }
		)
		chatMessage?.addDelegate(delegate: chatMessageDelegate!)
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
		let asset = AVAsset(url: URL(string: "file://" + videoURL.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed)!)!)
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
		if(collectionView == collectionViewReply){
			return replyCollectionView.count
		}else{
			return imagesGridCollectionView.count
		}
	}

	@objc(collectionView:cellForItemAtIndexPath:) func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
		if(collectionView == collectionViewReply){
			let cell = collectionView.dequeueReusableCell(withReuseIdentifier: "cellReplyMessage", for: indexPath)
			let viewCell: UIView = UIView(frame: cell.contentView.frame)
			cell.addSubview(viewCell)
			let imageCell = replyCollectionView[indexPath.row]
			var myImageView = UIImageView()
			
			if(replyURLCollection[indexPath.row].type == "image" || replyURLCollection[indexPath.row].type == "video"){
				myImageView = UIImageView(image: imageCell)
			}else{
				let fileNameText = replyURLCollection[indexPath.row].name
				let fileName = SwiftUtil.textToImage(drawText:fileNameText, inImage:imageCell, forReplyBubble:true)
				myImageView = UIImageView(image: fileName)
			}
			
			myImageView.size(w: (viewCell.frame.width), h: (viewCell.frame.height)).done()
			viewCell.addSubview(myImageView)
			
			if(replyURLCollection[indexPath.row].type == "video"){
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
		}else{
			let cell = collectionView.dequeueReusableCell(withReuseIdentifier: "cellImagesGridMessage", for: indexPath)
			let viewCell: UIView = UIView(frame: cell.contentView.frame)
			cell.addSubview(viewCell)
			if chatMessage?.contents[indexPath.row].filePath == "" {
				let  downloadView = DownloadMessageCell()
				downloadContentCollection.append(downloadView)
				downloadView.content = chatMessage?.contents[indexPath.row]
				downloadView.size(w: 138, h: 138).done()
				viewCell.addSubview(downloadView)

				downloadView.downloadNameLabel.text = chatMessage?.contents[indexPath.row].name.replacingOccurrences(of: (chatMessage?.contents[indexPath.row].name.dropFirst(6).dropLast(8))!, with: "...")
				downloadView.setFileType(fileName: (chatMessage?.contents[indexPath.row].name)!)
                
                let underlineAttribute = [NSAttributedString.Key.underlineStyle: NSUnderlineStyle.thick.rawValue]
				let underlineAttributedString = NSAttributedString(string: "\(VoipTexts.bubble_chat_download_file) (\(String(format: "%.1f", Float((chatMessage?.contents[indexPath.row].fileSize)!) / 1000000)) Mo)", attributes: underlineAttribute)
				downloadView.downloadButtonLabel.attributedText = underlineAttributedString
				downloadView.downloadButtonLabel.onClick {
					self.chatMessage?.contents[indexPath.row].filePath = LinphoneManager.imagesDirectory() + ((self.chatMessage?.contents[indexPath.row].name)!)
					let _ = self.chatMessage!.downloadContent(content: (self.chatMessage?.contents[indexPath.row])!)
                }
				downloadView.downloadButtonLabel.isUserInteractionEnabled = true
				
				if((linphone_core_get_max_size_for_auto_download_incoming_files(LinphoneManager.getLc()) > -1 && self.chatMessage!.isFileTransferInProgress) || self.chatMessage!.isOutgoing){
					downloadView.downloadButtonLabel.isHidden = true
                }
            } else {
				downloadContentCollection.append(nil)
				
                
                let myImageView = UIImageView()
                
				if(self.chatMessage?.contents[indexPath.row].type == "image" || self.chatMessage?.contents[indexPath.row].type == "video"){
					if #available(iOS 15.0, *) {
						myImageView.image = UIImage(named: "file_picture_default")
						let imageAsync: UIImage = getImageFrom(self.chatMessage?.contents[indexPath.row], forReplyBubble: false)!
						imageAsync.prepareForDisplay(completionHandler: { imageAsyncResult in
							DispatchQueue.main.async {
								myImageView.image = imageAsyncResult
							}
						})
					} else {
						DispatchQueue.global().async { [weak self] in
							if let image = self!.getImageFrom(self!.chatMessage?.contents[indexPath.row], forReplyBubble: false) {
								DispatchQueue.main.async {
									myImageView.image = image
								}
							}
						}
					}
					
                }else{
					myImageView.image = self.getImageFrom(self.chatMessage?.contents[indexPath.row], forReplyBubble: false)!
                }
                
                myImageView.size(w: (viewCell.frame.width), h: (viewCell.frame.height)).done()
                viewCell.addSubview(myImageView)
                
                myImageView.contentMode = .scaleAspectFill
                myImageView.clipsToBounds = true
				
				
				let  uploadView = UploadMessageCell()
				uploadContentCollection.append(uploadView)
				uploadView.content = chatMessage?.contents[indexPath.row]
				uploadView.size(w: 138, h: 138).done()
				
				if(self.chatMessage?.contents[indexPath.row].type == "video"){
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
				
				viewCell.onClick {
					ChatConversationTableViewModel.sharedModel.onGridClick(indexMessage: self.selfIndexMessage, index: indexPath.row)
				}
				
				viewCell.addSubview(uploadView)
            }
			return cell
		}
	}
	
	func getImageFrom(_ content: Content?, forReplyBubble: Bool) -> UIImage? {
		var filePath = ""
		if VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) {
			filePath = content!.exportPlainFile()
		}else {
			filePath = content!.filePath
		}
		let type = content?.type
		let name = content?.name
		if filePath == "" {
			filePath = LinphoneManager.validFilePath(name)
		}
		
		var image: UIImage? = nil
		if type == "video" {
			image = createThumbnailOfVideoFromFileURL(videoURL: filePath)
		} else if type == "image" {
			image = UIImage(named: filePath)
		}
		
		if VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) {
			ChatConversationViewModel.sharedModel.removeTmpFile(filePath: filePath)
			filePath = ""
		}
		
		if let image {
			return image
		} else {
			return getImageFromFileName(name, forReplyBubble: forReplyBubble)
		}
	}
	
	func getImageFromFileName(_ fileName: String?, forReplyBubble forReplyBubbble: Bool) -> UIImage? {
		let extensionFile = fileName?.lowercased().components(separatedBy: ".").last
		var image: UIImage?
		var text = fileName
		if fileName?.contains("voice-recording") ?? false {
			image = UIImage(named: "file_voice_default")
			text = recordingDuration(LinphoneManager.validFilePath(fileName))
		} else {
			if extensionFile == "pdf" {
				image = UIImage(named: "file_pdf_default")
			} else if ["png", "jpg", "jpeg", "bmp", "heic"].contains(extensionFile ?? "") {
				image = UIImage(named: "file_picture_default")
			} else if ["mkv", "avi", "mov", "mp4"].contains(extensionFile ?? "") {
				image = UIImage(named: "file_video_default")
			} else if ["wav", "au", "m4a"].contains(extensionFile ?? "") {
				image = UIImage(named: "file_audio_default")
			} else {
				image = UIImage(named: "file_default")
			}
		}

		return SwiftUtil.textToImage(drawText: text!, inImage: image!, forReplyBubble: forReplyBubbble)
	}
	
	func setEvent(event: EventLog) -> String {
		var subject = ""
		var participant = ""
		switch (event.type.rawValue) {
		case Int(LinphoneEventLogTypeConferenceSubjectChanged.rawValue):
			subject = event.subject
			return VoipTexts.bubble_chat_event_message_new_subject + subject
		case Int(LinphoneEventLogTypeConferenceParticipantAdded.rawValue):
			participant = event.participantAddress!.displayName != "" ? event.participantAddress!.displayName : event.participantAddress!.username
			return participant + VoipTexts.bubble_chat_event_message_has_joined
		case Int(LinphoneEventLogTypeConferenceParticipantRemoved.rawValue):
			participant = event.participantAddress!.displayName != "" ? event.participantAddress!.displayName : event.participantAddress!.username
			return participant + VoipTexts.bubble_chat_event_message_has_left
		case Int(LinphoneEventLogTypeConferenceParticipantSetAdmin.rawValue):
			participant = event.participantAddress!.displayName != "" ? event.participantAddress!.displayName : event.participantAddress!.username
			return participant + VoipTexts.bubble_chat_event_message_now_admin
		case Int(LinphoneEventLogTypeConferenceParticipantUnsetAdmin.rawValue):
			participant = event.participantAddress!.displayName != "" ? event.participantAddress!.displayName : event.participantAddress!.username
			return participant + VoipTexts.bubble_chat_event_message_no_longer_admin
		case Int(LinphoneEventLogTypeConferenceTerminated.rawValue):
			return VoipTexts.bubble_chat_event_message_left_group
		case Int(LinphoneEventLogTypeConferenceCreated.rawValue):
			return VoipTexts.bubble_chat_event_message_joined_group
		case Int(LinphoneEventLogTypeConferenceSecurityEvent.rawValue):
			let type = event.securityEventType
			let participant = event.securityEventFaultyDeviceAddress!.displayName != "" ? event.securityEventFaultyDeviceAddress!.displayName : event.securityEventFaultyDeviceAddress!.username
			switch (type.rawValue) {
			case Int(LinphoneSecurityEventTypeSecurityLevelDowngraded.rawValue):
				if (participant.isEmpty){
					return VoipTexts.bubble_chat_event_message_security_level_decreased
				}else{
					return VoipTexts.bubble_chat_event_message_security_level_decreased_because + participant
				}
			case Int(LinphoneSecurityEventTypeParticipantMaxDeviceCountExceeded.rawValue):
				if (participant.isEmpty){
					return VoipTexts.bubble_chat_event_message_max_participant
				}else{
					return VoipTexts.bubble_chat_event_message_max_participant_by + participant
				}
			case Int(LinphoneSecurityEventTypeEncryptionIdentityKeyChanged.rawValue):
				if (participant.isEmpty){
					return VoipTexts.bubble_chat_event_message_lime_changed
				}else{
					return VoipTexts.bubble_chat_event_message_lime_changed_for + participant
				}
			case Int(LinphoneSecurityEventTypeManInTheMiddleDetected.rawValue):
				if (participant.isEmpty){
					return VoipTexts.bubble_chat_event_message_attack_detected
				}else{
					return VoipTexts.bubble_chat_event_message_attack_detected_for + participant
				}
			default:
				return ""
			}
		case Int(LinphoneEventLogTypeConferenceEphemeralMessageDisabled.rawValue):
			return VoipTexts.bubble_chat_event_message_disabled_ephemeral
		case Int(LinphoneEventLogTypeConferenceEphemeralMessageEnabled.rawValue):
			return VoipTexts.bubble_chat_event_message_enabled_ephemeral
		case Int(LinphoneEventLogTypeConferenceEphemeralMessageLifetimeChanged.rawValue):
			return VoipTexts.bubble_chat_event_message_expiry_ephemeral + formatEphemeralExpiration(duration: event.ephemeralMessageLifetime)
		default:
			return ""
		}
	}
		
	func formatEphemeralExpiration(duration: CLong) -> String{
		switch (duration) {
		case 0:
			return VoipTexts.bubble_chat_event_message_ephemeral_disable
		case 60:
			return VoipTexts.bubble_chat_event_message_ephemeral_one_minute
		case 3600:
			return VoipTexts.bubble_chat_event_message_ephemeral_one_hour
		case 86400:
			return VoipTexts.bubble_chat_event_message_ephemeral_one_day
		case 259200:
			return VoipTexts.bubble_chat_event_message_ephemeral_three_days
		case 604800:
			return VoipTexts.bubble_chat_event_message_ephemeral_one_week
		default:
			return VoipTexts.bubble_chat_event_message_ephemeral_unexpected_duration
		}
	}
	
	func file_transfer_progress_indication_recv(message: ChatMessage, content: Content, offset: Int, total: Int) {
		let p =  Float(offset) / Float(total)
		
		if ((chatMessage?.contents.count)! > 0){
			if  !message.isOutgoing {
				if (indexTransferProgress == -1) {
					for indexItem in 0...(chatMessage?.contents.count)! - 1 {
						if chatMessage?.contents[indexItem].name == content.name {
							indexTransferProgress = indexItem
							break
						}
					}
					
					if downloadContentCollection[indexTransferProgress] != nil {
						downloadContentCollection[indexTransferProgress]!.downloadButtonLabel.isHidden = true
						downloadContentCollection[indexTransferProgress]!.circularProgressBarView.isHidden = false
					}
				}
				DispatchQueue.main.async(execute: { [self] in
					if (offset == total) {
						downloadContentCollection[indexTransferProgress] = nil
						imagesGridCollectionView[indexTransferProgress] = getImageFrom(content, forReplyBubble: false)!
						
						
						if (imagesGridCollectionView.count <= 1){
                            if content.type == "video" {
								if VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) {
									var plainFile = content.exportPlainFile()
									if let imageMessage = createThumbnailOfVideoFromFileURL(videoURL: plainFile){
										imageVideoViewBubble.image = resizeImage(image: imageMessage, targetSize: CGSizeMake(UIScreen.main.bounds.size.width*3/4, 300.0))
										if (imageVideoViewBubble.image != nil && imagesGridCollectionView.count <= 1){
											ChatConversationTableViewModel.sharedModel.reloadCollectionViewCell()
										}
									}
									
									ChatConversationViewModel.sharedModel.removeTmpFile(filePath: plainFile)
									plainFile = ""
								}else{
									if let imageMessage = createThumbnailOfVideoFromFileURL(videoURL: content.filePath){
										imageVideoViewBubble.image = resizeImage(image: imageMessage, targetSize: CGSizeMake(UIScreen.main.bounds.size.width*3/4, 300.0))
										if (imageVideoViewBubble.image != nil && imagesGridCollectionView.count <= 1){
											ChatConversationTableViewModel.sharedModel.reloadCollectionViewCell()
										}
									}
								}
							} else if content.type == "image" {
								if VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) {
									var plainFile = content.exportPlainFile()
									if let imageMessage = UIImage(named: plainFile){
										imageViewBubble.image = resizeImage(image: imageMessage, targetSize: CGSizeMake(UIScreen.main.bounds.size.width*3/4, 300.0))
										if (imageViewBubble.image != nil && imagesGridCollectionView.count <= 1){
											ChatConversationTableViewModel.sharedModel.reloadCollectionViewCell()
										}
									}
									
									ChatConversationViewModel.sharedModel.removeTmpFile(filePath: plainFile)
									plainFile = ""
								}else{
									if let imageMessage = UIImage(named: content.filePath){
										imageViewBubble.image = resizeImage(image: imageMessage, targetSize: CGSizeMake(UIScreen.main.bounds.size.width*3/4, 300.0))
										if (imageViewBubble.image != nil && imagesGridCollectionView.count <= 1){
											ChatConversationTableViewModel.sharedModel.reloadCollectionViewCell()
										}
									}
								}
							} else {
								collectionViewImagesGrid.reloadItems(at: [IndexPath(row: indexTransferProgress, section: 0)])
								indexTransferProgress = -1
							}
						}else{
							collectionViewImagesGrid.reloadItems(at: [IndexPath(row: indexTransferProgress, section: 0)])
							indexTransferProgress = -1
						}
					} else {
						if downloadContentCollection[indexTransferProgress] != nil {
							downloadContentCollection[indexTransferProgress]!.setUpCircularProgressBarView(toValue: p)
						}
					}
				})
			} else {
				if((chatMessage?.contents.count)! > 1){
                    DispatchQueue.main.async(execute: { [self] in
                        if (offset == total) {
							if(indexUploadTransferProgress >= 0){
								uploadContentCollection[indexUploadTransferProgress]!.circularProgressBarView.isHidden = true
							}
							if indexUploadTransferProgress <= (chatMessage?.contents.count)! {
								indexUploadTransferProgress += 1
							}else{
								indexUploadTransferProgress = 0
							}
                        } else {
							if((chatMessage?.contents.count)! > 1){
						  		uploadContentCollection[indexUploadTransferProgress]!.circularProgressBarView.isHidden = false
								uploadContentCollection[indexUploadTransferProgress]!.setUpCircularProgressBarView(toValue: p)
					  		}
                        }
                    })
                }
			}
		}
	}
    
    func displayImdnStatus(message: ChatMessage, state: ChatMessage.State) {
        if message.isOutgoing {
            if (state == ChatMessage.State.DeliveredToUser) {
                chatRead.image = UIImage(named: "chat_delivered.png")
                chatRead.isHidden = false
            } else if (state == ChatMessage.State.Displayed) {
                chatRead.image = UIImage(named: "chat_read.png")
                chatRead.isHidden = false
            } else if (state == ChatMessage.State.NotDelivered || state == ChatMessage.State.FileTransferError) {
                chatRead.image = UIImage(named: "chat_error")
                chatRead.isHidden = false
            } else {
                chatRead.isHidden = true
            }
        }
    }
	
	func contactDateForChat(message: ChatMessage) -> String {
		let address: Address? = message.fromAddress != nil ? message.fromAddress : message.chatRoom?.peerAddress
		return LinphoneUtils.time(toString: message.time, with: LinphoneDateChatBubble) + " - " + FastAddressBook.displayName(for: address?.getCobject)
	}
	
	func isFirstIndexInTableView(indexPath: IndexPath, chat: ChatMessage) -> Bool{
		let MAX_AGGLOMERATED_TIME=300
		var previousEvent : EventLog?  = nil
		let indexOfPreviousEvent = indexPath.row + 1
		previousEvent = ChatConversationTableViewModel.sharedModel.getMessage(index: indexPath.row+1)
		if (indexOfPreviousEvent > -1 && indexOfPreviousEvent < ChatConversationTableViewModel.sharedModel.getNBMessages()) {
			if ((previousEvent?.type.rawValue)! != LinphoneEventLogTypeConferenceChatMessage.rawValue) {
				return true
			}
		}
		if (previousEvent == nil){
			return true
		}

		let previousChat = previousEvent?.chatMessage
		
		if previousChat != nil {
			if (previousChat?.fromAddress!.equal(address2: chat.fromAddress!) == false) {
				return true;
			}
			// the maximum interval between 2 agglomerated chats at 5mn
			if (chat.time - previousChat!.time > MAX_AGGLOMERATED_TIME) {
				return true;
			}
		}
		
		return false;
	}
	
	func updateEphemeralTimes() {
		let f = DateComponentsFormatter()
		f.unitsStyle = .positional
		f.zeroFormattingBehavior = [.pad]
		
		if ((chatMessage != nil) && chatMessage!.isEphemeral) {
			let duration = self.chatMessage?.ephemeralExpireTime == 0 ? self.chatMessage?.ephemeralLifetime : self.chatMessage!.ephemeralExpireTime - Int(Date().timeIntervalSince1970)
			if(duration! > 86400){
				f.allowedUnits = [.day]
			}else if(duration! > 3600){
				f.allowedUnits = [.hour]
			}else{
				f.allowedUnits = [.minute, .second]
			}
			
			let textDuration = f.string(for: DateComponents(second: duration))?.capitalized ?? ""
			self.ephemeralTimerLabel.text = textDuration
			self.ephemeralTimerLabel.isHidden = false
			self.ephemeralIcon.isHidden = false
			ephemeralTimer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { timer in
				let duration = self.chatMessage?.ephemeralExpireTime == 0 ? self.chatMessage?.ephemeralLifetime : self.chatMessage!.ephemeralExpireTime - Int(Date().timeIntervalSince1970)
				if(duration! > 86400){
					f.allowedUnits = [.day]
				}else if(duration! > 3600){
					f.allowedUnits = [.hour]
				}else{
					f.allowedUnits = [.minute, .second]
				}
				
				let textDuration = f.string(for: DateComponents(second: duration))?.capitalized ?? ""
				self.ephemeralTimerLabel.text = textDuration
				self.ephemeralTimerLabel.isHidden = false
				self.ephemeralIcon.isHidden = false
				if(duration! <= 0){
					self.ephemeralTimer!.invalidate()
					ChatConversationTableViewModel.sharedModel.reloadCollectionViewCell()
				}
			}
		}
	}
}

class DynamicHeightCollectionView: UICollectionView {
	override func layoutSubviews() {
		super.layoutSubviews()
		if !__CGSizeEqualToSize(bounds.size, self.intrinsicContentSize) {
			self.invalidateIntrinsicContentSize()
		}
	}
	
	override var intrinsicContentSize: CGSize {
		return contentSize
	}
}
