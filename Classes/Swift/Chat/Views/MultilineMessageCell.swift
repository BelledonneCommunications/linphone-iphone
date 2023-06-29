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
import Foundation
import linphonesw
import SwipeCellKit

class MultilineMessageCell: SwipeCollectionViewCell, UICollectionViewDataSource, UICollectionViewDelegate {
	static let reuseId = "MultilineMessageCellReuseId"
	
	var label: UILabel = UILabel(frame: .zero)
	var eventMessageView: UIView = UIView(frame: .zero)
	var preContentViewBubble: UIView = UIView(frame: .zero)
	var contentViewBubble: UIView = UIView(frame: .zero)
	var contentMediaViewBubble: UIView = UIView(frame: .zero)
	var contentBubble: UIView = UIView(frame: .zero)
	var bubble: UIView = UIView(frame: .zero)
	var imageUser = UIImageView()
	var contactDateLabel = StyledLabel(VoipTheme.chat_conversation_forward_label)
	var chatRead = UIImageView(image: UIImage(named: "chat_delivered.png"))

	var labelInset = UIEdgeInsets(top: 10, left: 10, bottom: -10, right: -10)
	
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
	var imagesGridConstraintsWithRecording : [NSLayoutConstraint] = []
	var imageConstraints: [NSLayoutConstraint] = []
	var videoConstraints: [NSLayoutConstraint] = []
	var playButtonConstraints: [NSLayoutConstraint] = []
	var progressBarVideoConstraints: [NSLayoutConstraint] = []
	var progressBarImageConstraints: [NSLayoutConstraint] = []
	var recordingConstraints: [NSLayoutConstraint] = []
	var recordingConstraintsWithMediaGrid: [NSLayoutConstraint] = []
	var recordingWaveConstraints: [NSLayoutConstraint] = []
	var recordingWaveConstraintsWithMediaGrid: [NSLayoutConstraint] = []
	var meetingConstraints: [NSLayoutConstraint] = []
	
	var eventMessageLineView: UIView = UIView(frame: .zero)
	var eventMessageLabelView: UIView = UIView(frame: .zero)
	var eventMessageLabel = StyledLabel(VoipTheme.chat_conversation_forward_label)
	
	var forwardView = UIView()
	var forwardIcon = UIImageView(image: UIImage(named: "menu_forward_default"))
	var forwardLabel = StyledLabel(VoipTheme.chat_conversation_black_text)
	
	var replyView = UIView()
	var replyIcon = UIImageView(image: UIImage(named: "menu_reply_default"))
	var replyLabel = StyledLabel(VoipTheme.chat_conversation_black_text)
	var replyContent = UIView()
	var replyColorContent = UIView()
	var replyLabelContent = StyledLabel(VoipTheme.chat_conversation_forward_label)
	var stackViewReply = UIStackView()
	var replyLabelTextView = StyledLabel(VoipTheme.chat_conversation_reply_label)
	var replyLabelContentTextSpacing = UIView()
	var replyContentTextView = StyledLabel(VoipTheme.chat_conversation_reply_content)
	var replyContentTextSpacing = UIView()
	var replyContentForMeetingTextView = StyledLabel(VoipTheme.chat_conversation_reply_content)
	var replyContentForMeetingSpacing = UIView()
	var replyMeetingSchedule = UIImageView()
	var mediaSelectorReply  = UIView()
	
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
	var replyContentCollection : [Content] = []
	
	var imagesGridCollectionView : [UIImage?] = []
    var imagesGridCollectionViewNil = 0
	var downloadContentCollection: [DownloadMessageCell?] = []
	var uploadContentCollection: [UploadMessageCell?] = []
	
	var imageViewBubble = UIImageView(image: UIImage(named: "chat_error"))
	var imageVideoViewBubble = UIImageView(image: UIImage(named: "file_video_default"))
	var imagePlayViewBubble = UIImageView(image: UIImage(named: "vr_play"))
	
	var meetingView = UIView()
	
	var recordingView = UIView()
	
	var ephemeralIcon = UIImageView(image: UIImage(named: "ephemeral_messages_color_A.png"))
	var ephemeralTimerLabel = StyledLabel(VoipTheme.chat_conversation_ephemeral_timer)
	var ephemeralTimer : Timer? = nil
	
	
	var isPlayingVoiceRecording = false
	
	var eventMessage: EventLog?
    var chatMessage: ChatMessage?
	var chatMessageDelegate: ChatMessageDelegate? = nil
	
	var indexTransferProgress: Int = -1
	var indexUploadTransferProgress: Int = -1
	
	var selfIndexMessage: Int = -1
	
	var deleteItemCheckBox = StyledCheckBox()
	
	var matches : [NSTextCheckingResult] = []
	
	var circularProgressBarVideoView = CircularProgressBarView()
	var circularProgressBarImageView = CircularProgressBarView()
	var circularProgressBarVideoLabel = StyledLabel(VoipTheme.chat_conversation_download_progress_text)
	var circularProgressBarImageLabel = StyledLabel(VoipTheme.chat_conversation_download_progress_text)
	var fromValue : Float = 0.0
	
	var messageWithRecording = false
	
	override init(frame: CGRect) {
		super.init(frame: frame)
		initCell()
	}
	
	func initCell(){
	//CheckBox for select item to delete
		contentView.addSubview(deleteItemCheckBox)
		deleteItemCheckBox.isHidden = true
		
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
		
		imagesGridConstraintsWithRecording = [
			collectionViewImagesGrid.topAnchor.constraint(equalTo: contentMediaViewBubble.topAnchor, constant: labelInset.top + 50),
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
		
		imageViewBubble.contentMode = .scaleAspectFit
		
		imageViewBubble.addSubview(circularProgressBarImageView)
		progressBarImageConstraints = [
			circularProgressBarImageView.centerXAnchor.constraint(equalTo: imageViewBubble.centerXAnchor),
			circularProgressBarImageView.centerYAnchor.constraint(equalTo: imageViewBubble.centerYAnchor)
		]
		
		circularProgressBarImageView.size(w: 138, h: 138).done()
		circularProgressBarImageLabel.size(w: 30, h: 30).done()
		circularProgressBarImageLabel.text = "0%"
		circularProgressBarImageView.addSubview(circularProgressBarImageLabel)
		circularProgressBarImageView.isHidden = true
		
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
		
		imageVideoViewBubble.contentMode = .scaleAspectFit
		
		if #available(iOS 13.0, *) {
			imagePlayViewBubble.image = (UIImage(named: "vr_play")!.withTintColor(.white))
		}
		
		imageVideoViewBubble.addSubview(imagePlayViewBubble)
		playButtonConstraints = [
			imagePlayViewBubble.centerXAnchor.constraint(equalTo: imageVideoViewBubble.centerXAnchor),
			imagePlayViewBubble.centerYAnchor.constraint(equalTo: imageVideoViewBubble.centerYAnchor)
		]
		imagePlayViewBubble.size(w: 40, h: 40).done()
		
		imageVideoViewBubble.addSubview(circularProgressBarVideoView)
		progressBarVideoConstraints = [
			circularProgressBarVideoView.centerXAnchor.constraint(equalTo: imageVideoViewBubble.centerXAnchor),
			circularProgressBarVideoView.centerYAnchor.constraint(equalTo: imageVideoViewBubble.centerYAnchor)
		]
		
		circularProgressBarVideoView.size(w: 138, h: 138).done()
		circularProgressBarVideoLabel.size(w: 30, h: 30).done()
		circularProgressBarVideoLabel.text = "0%"
		circularProgressBarVideoView.addSubview(circularProgressBarVideoLabel)
		circularProgressBarVideoView.isHidden = true
		
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

		recordingConstraintsWithMediaGrid = [
			recordingView.topAnchor.constraint(equalTo: contentMediaViewBubble.topAnchor, constant: labelInset.top),
			recordingView.leadingAnchor.constraint(equalTo: contentMediaViewBubble.leadingAnchor, constant: labelInset.left),
			recordingView.trailingAnchor.constraint(equalTo: contentMediaViewBubble.trailingAnchor, constant: labelInset.right)
		]
		 
		recordingView.height(50.0).width(280).done()
		recordingView.isHidden = true
		
	//Text
		label.numberOfLines = 0
		label.lineBreakMode = .byWordWrapping
		label.textColor = .black
		
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
			recordingWaveView.trailingAnchor.constraint(equalTo: contentMediaViewBubble.trailingAnchor, constant: labelInset.right),
			recordingWaveImage.centerYAnchor.constraint(equalTo: recordingWaveView.centerYAnchor)
		]
		
		recordingWaveConstraintsWithMediaGrid = [
			recordingWaveView.topAnchor.constraint(equalTo: recordingView.topAnchor, constant: 0),
			recordingWaveView.bottomAnchor.constraint(equalTo: recordingView.bottomAnchor, constant: -10),
			recordingWaveView.leadingAnchor.constraint(equalTo: contentMediaViewBubble.leadingAnchor, constant: labelInset.left),
			recordingWaveView.trailingAnchor.constraint(equalTo: contentMediaViewBubble.trailingAnchor, constant: labelInset.right),
			recordingWaveImage.centerYAnchor.constraint(equalTo: recordingWaveView.centerYAnchor)
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
		recordingWaveImage.alignParentLeft(withMargin: 60).alignParentRight(withMargin: 60).height(26).done()
		
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
		
		if (recordingView.isHidden == false && imagesGridCollectionView.count > 0){
			collectionViewImagesGrid.isHidden = false
			NSLayoutConstraint.activate(recordingConstraintsWithMediaGrid)
			NSLayoutConstraint.activate(recordingWaveConstraintsWithMediaGrid)
		} else {
			NSLayoutConstraint.activate(recordingConstraints)
			NSLayoutConstraint.activate(recordingWaveConstraints)
		}
		
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
		
		deleteItemCheckBox.removeFromSuperview()
		eventMessageView.removeFromSuperview()
		contactDateLabel.removeFromSuperview()
		contentBubble.removeFromSuperview()
		
		if chatMessageDelegate != nil {
			chatMessage?.removeDelegate(delegate: chatMessageDelegate!)
		}
		
		label = UILabel(frame: .zero)
		eventMessageView = UIView(frame: .zero)
		preContentViewBubble = UIView(frame: .zero)
		contentViewBubble = UIView(frame: .zero)
		contentMediaViewBubble = UIView(frame: .zero)
		contentBubble = UIView(frame: .zero)
		bubble = UIView(frame: .zero)
		imageUser = UIImageView()
		contactDateLabel = StyledLabel(VoipTheme.chat_conversation_forward_label)
		chatRead = UIImageView(image: UIImage(named: "chat_delivered.png"))
		labelInset = UIEdgeInsets(top: 10, left: 10, bottom: -10, right: -10)
		constraintEventMesssage = []
		constraintEventMesssageLabel = []
		constraintBubble = []
		constraintLeadingBubble = nil
		constraintTrailingBubble = nil
		constraintDateLeadingBubble = nil
		constraintDateTrailingBubble = nil
		constraintDateBubble = nil
		constraintDateBubbleHidden = nil
		preContentViewBubbleConstraints = []
		preContentViewBubbleConstraintsHidden = []
		contentViewBubbleConstraints = []
		contentMediaViewBubbleConstraints = []
		forwardConstraints = []
		replyConstraints = []
		labelConstraints = []
		labelTopConstraints = []
		labelHiddenConstraints = []
		imagesGridConstraints = []
		imagesGridConstraintsWithRecording = []
		imageConstraints = []
		videoConstraints = []
		playButtonConstraints = []
		progressBarVideoConstraints = []
		progressBarImageConstraints = []
		recordingConstraints = []
		recordingConstraintsWithMediaGrid = []
		recordingWaveConstraints = []
		recordingWaveConstraintsWithMediaGrid = []
		meetingConstraints = []
		eventMessageLineView = UIView(frame: .zero)
		eventMessageLabelView = UIView(frame: .zero)
		eventMessageLabel = StyledLabel(VoipTheme.chat_conversation_forward_label)
		forwardView = UIView()
		forwardIcon = UIImageView(image: UIImage(named: "menu_forward_default"))
		replyView = UIView()
		replyIcon = UIImageView(image: UIImage(named: "menu_reply_default"))
		replyContent = UIView()
		replyColorContent = UIView()
		replyLabelContent = StyledLabel(VoipTheme.chat_conversation_forward_label)
		stackViewReply = UIStackView()
		replyLabelTextView = StyledLabel(VoipTheme.chat_conversation_reply_label)
		replyLabelContentTextSpacing = UIView()
		replyContentTextView = StyledLabel(VoipTheme.chat_conversation_reply_content)
		replyContentTextSpacing = UIView()
		replyContentForMeetingTextView = StyledLabel(VoipTheme.chat_conversation_reply_content)
		replyContentForMeetingSpacing = UIView()
		replyMeetingSchedule = UIImageView()
		mediaSelectorReply  = UIView()
		replyCollectionView = []
		replyContentCollection = []
		imagesGridCollectionView = []
		imagesGridCollectionViewNil = 0
		downloadContentCollection = []
		uploadContentCollection = []
		imageViewBubble = UIImageView(image: UIImage(named: "chat_error"))
		imageVideoViewBubble = UIImageView(image: UIImage(named: "file_video_default"))
		imagePlayViewBubble = UIImageView(image: UIImage(named: "vr_play"))
		meetingView = UIView()
		recordingView = UIView()
		ephemeralIcon = UIImageView(image: UIImage(named: "ephemeral_messages_color_A.png"))
		ephemeralTimerLabel = StyledLabel(VoipTheme.chat_conversation_ephemeral_timer)
		ephemeralTimer = nil
		isPlayingVoiceRecording = false
		chatMessage = nil
		chatMessageDelegate = nil
		indexTransferProgress = -1
		indexUploadTransferProgress = -1
		selfIndexMessage = -1
		deleteItemCheckBox = StyledCheckBox()
		matches = []
		circularProgressBarVideoView = CircularProgressBarView()
		circularProgressBarImageView = CircularProgressBarView()
		circularProgressBarVideoLabel = StyledLabel(VoipTheme.chat_conversation_download_progress_text)
		circularProgressBarImageLabel = StyledLabel(VoipTheme.chat_conversation_download_progress_text)
		fromValue = 0.0
		
		collectionViewReply = {
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
		collectionViewImagesGrid = {
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
		messageWithRecording = false
		initCell()
	}
	
	func configure(event: EventLog, selfIndexPathConfigure: IndexPath, editMode: Bool, selected: Bool) {
		selfIndexMessage = selfIndexPathConfigure.row
		eventMessage = event
        chatMessage = event.chatMessage
		addMessageDelegate()
        imagesGridCollectionView.removeAll()
        imagesGridCollectionViewNil = 0
		imageUser.isHidden = true
		deleteItemCheckBox.isHidden = true
		
		
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
				
				if isFirstIndexInTableView(indexPath: selfIndexPathConfigure, chat: event.chatMessage!) {
					imageUser.isHidden = false
					if event.chatMessage?.fromAddress?.contact() != nil {
						imageUser.image = FastAddressBook.image(for: event.chatMessage?.fromAddress?.contact())
					}else{
						imageUser.image = FastAddressBook.image(for: event.chatMessage?.fromAddress?.getCobject)
					}
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
					imageUser.isHidden = true
					contactDateLabel.size(w: 200, h: 0).done()
				}

				bubble.backgroundColor = VoipTheme.gray_light_color//.withAlphaComponent(0.2)
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
				
				bubble.backgroundColor = VoipTheme.primary_light_color//.withAlphaComponent(0.2)
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
                                    replyContentCollection.append(content)
									replyCollectionView.append(getImageFrom(content, forReplyBubble: false)!)
									collectionViewReply.reloadData()
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
				NSLayoutConstraint.deactivate(progressBarVideoConstraints)
				NSLayoutConstraint.deactivate(progressBarImageConstraints)
				NSLayoutConstraint.deactivate(recordingConstraints)
				NSLayoutConstraint.deactivate(recordingConstraintsWithMediaGrid)
				NSLayoutConstraint.deactivate(recordingWaveConstraints)
				NSLayoutConstraint.deactivate(recordingWaveConstraintsWithMediaGrid)
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
				NSLayoutConstraint.deactivate(imagesGridConstraintsWithRecording)
				NSLayoutConstraint.deactivate(imageConstraints)
				NSLayoutConstraint.deactivate(videoConstraints)
				NSLayoutConstraint.deactivate(playButtonConstraints)
				NSLayoutConstraint.deactivate(progressBarVideoConstraints)
				NSLayoutConstraint.deactivate(progressBarImageConstraints)
				NSLayoutConstraint.deactivate(recordingConstraints)
				NSLayoutConstraint.deactivate(recordingConstraintsWithMediaGrid)
				NSLayoutConstraint.deactivate(recordingWaveConstraints)
				NSLayoutConstraint.deactivate(recordingWaveConstraintsWithMediaGrid)
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
					if (content.isFileTransfer && content.name != "" && !content.isVoiceRecording) {
						imagesGridCollectionView.append(getImageFrom(content, forReplyBubble: false)!)
                        collectionViewImagesGrid.reloadData()
                        
                        collectionViewImagesGrid.isHidden = false
                        NSLayoutConstraint.activate(imagesGridConstraints)
						imageViewBubble.image = nil
						NSLayoutConstraint.deactivate(imageConstraints)
						imageViewBubble.isHidden = true
                    }
					
					if (event.chatMessage?.isOutgoing == true && content.isFileTransfer && event.chatMessage?.isFileTransferInProgress == true && !content.isVoiceRecording) {
						var filePath = ""
						if VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) {
							filePath = content.exportPlainFile()
						}else {
							filePath = content.filePath
						}
						let name = content.name
						if filePath == "" {
							filePath = LinphoneManager.validFilePath(name)
						}
						
						let extensionFile = filePath.lowercased().components(separatedBy: ".").last
						if (["png", "jpg", "jpeg", "bmp", "heic"].contains(extensionFile ?? "")){
							if imagesGridCollectionView.count > 1 {
								imagesGridCollectionView.append(getImageFrom(content, forReplyBubble: false)!)
								collectionViewImagesGrid.reloadData()
								
								collectionViewImagesGrid.isHidden = false
								NSLayoutConstraint.activate(imagesGridConstraints)
								imageViewBubble.image = nil
								NSLayoutConstraint.deactivate(imageConstraints)
								imageViewBubble.isHidden = true
								
							}else{
								if VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) {
									var plainFile = content.exportPlainFile()
									if let imageMessage = UIImage(named: plainFile){
										self.imageViewBubble.image = self.resizeImage(image: imageMessage, targetSize: CGSize(width: UIScreen.main.bounds.size.width*3/4, height: 300.0))
									}
									
									ChatConversationViewModel.sharedModel.removeTmpFile(filePath: plainFile)
									plainFile = ""
								}else{
									if let imageMessage = UIImage(named: content.filePath){
										self.imageViewBubble.image = self.resizeImage(image: imageMessage, targetSize: CGSize(width: UIScreen.main.bounds.size.width*3/4, height: 300.0))
									}
								}
								
								imagesGridCollectionView.append(getImageFrom(content, forReplyBubble: false)!)
								collectionViewImagesGrid.reloadData()
							}
						} else if (["mkv", "avi", "mov", "mp4"].contains(extensionFile ?? "")){
							if imagesGridCollectionView.count > 1 {
								imagesGridCollectionView.append(getImageFrom(content, forReplyBubble: false)!)
								collectionViewImagesGrid.reloadData()
								
								collectionViewImagesGrid.isHidden = false
								NSLayoutConstraint.activate(imagesGridConstraints)
								imageVideoViewBubble.image = nil
								NSLayoutConstraint.deactivate(imageConstraints)
								imageVideoViewBubble.isHidden = true
								
							}else{
								if VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) {
									var plainFile = content.exportPlainFile()
									if let imageMessage = createThumbnailOfVideoFromFileURL(videoURL: plainFile){
										imageVideoViewBubble.image = resizeImage(image: imageMessage, targetSize: CGSize(width: UIScreen.main.bounds.size.width*3/4, height: 300.0))
									}
									
									ChatConversationViewModel.sharedModel.removeTmpFile(filePath: plainFile)
									plainFile = ""
								}else{
									if let imageMessage = createThumbnailOfVideoFromFileURL(videoURL: content.filePath){
										imageVideoViewBubble.image = resizeImage(image: imageMessage, targetSize: CGSize(width: UIScreen.main.bounds.size.width*3/4, height: 300.0))
									}
								}
								imagesGridCollectionView.append(getImageFrom(content, forReplyBubble: false)!)
								collectionViewImagesGrid.reloadData()
							}
							
							if VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) {
								ChatConversationViewModel.sharedModel.removeTmpFile(filePath: filePath)
								filePath = ""
							}
						} else {
							imagesGridCollectionView.append(getImageFrom(content, forReplyBubble: false)!)
						   	collectionViewImagesGrid.reloadData()
						   	
						   	collectionViewImagesGrid.isHidden = false
						   	NSLayoutConstraint.activate(imagesGridConstraints)
						   	imageViewBubble.image = nil
						   	NSLayoutConstraint.deactivate(imageConstraints)
						   	imageViewBubble.isHidden = true
						}
					}
                    
					if content.type == "text" && !content.isFile{
						if event.chatMessage!.contents.count > 1 {
							NSLayoutConstraint.deactivate(labelConstraints)
							NSLayoutConstraint.activate(labelTopConstraints)
						}else{
							NSLayoutConstraint.activate(labelConstraints)
							NSLayoutConstraint.deactivate(labelTopConstraints)
						}
						
						if imagesGridCollectionView.count == 0 {
							//imagesGridCollectionView.append(nil)
							imagesGridCollectionViewNil += 1
						}
						
						label.font = label.font.withSize(17)
						
						if (content.utf8Text.trimmingCharacters(in: .whitespacesAndNewlines).unicodeScalars.first?.properties.isEmojiPresentation == true){
							var onlyEmojis = true
							content.utf8Text.trimmingCharacters(in: .whitespacesAndNewlines).unicodeScalars.forEach { emoji in
								if !emoji.properties.isEmojiPresentation && !emoji.properties.isWhitespace{
									onlyEmojis = false
								}
							}
							if onlyEmojis {
								label.font = label.font.withSize(51)
							}
						}
						
						checkIfIsLinkOrPhoneNumber(content: content.utf8Text)
						
						NSLayoutConstraint.deactivate(labelHiddenConstraints)
						label.isHidden = false
					}else if content.type == "image"{
						if imagesGridCollectionView.count > 1 {
							if(content.isFile){
								imagesGridCollectionView.append(getImageFrom(content, forReplyBubble: false)!)
								collectionViewImagesGrid.reloadData()
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
									self.imageViewBubble.image = self.resizeImage(image: imageMessage, targetSize: CGSize(width: UIScreen.main.bounds.size.width*3/4, height: 300.0))
								}
								
								ChatConversationViewModel.sharedModel.removeTmpFile(filePath: plainFile)
								plainFile = ""
							}else{
								if let imageMessage = UIImage(named: content.filePath){
									self.imageViewBubble.image = self.resizeImage(image: imageMessage, targetSize: CGSize(width: UIScreen.main.bounds.size.width*3/4, height: 300.0))
								}
							}
                            
                            if(content.isFile){
                                imagesGridCollectionView.append(getImageFrom(content, forReplyBubble: false)!)
								collectionViewImagesGrid.reloadData()
                            }
						}

					}else if content.type == "video"{
						if imagesGridCollectionView.count > 1 {
							if(content.isFile){
								imagesGridCollectionView.append(getImageFrom(content, forReplyBubble: false)!)
								collectionViewImagesGrid.reloadData()
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
									imageVideoViewBubble.image = resizeImage(image: imageMessage, targetSize: CGSize(width: UIScreen.main.bounds.size.width*3/4, height: 300.0))
								}
								
								ChatConversationViewModel.sharedModel.removeTmpFile(filePath: plainFile)
								plainFile = ""
							}else{
								if let imageMessage = createThumbnailOfVideoFromFileURL(videoURL: content.filePath){
									imageVideoViewBubble.image = resizeImage(image: imageMessage, targetSize: CGSize(width: UIScreen.main.bounds.size.width*3/4, height: 300.0))
								}
							}
							
							if(content.isFile){
								imagesGridCollectionView.append(getImageFrom(content, forReplyBubble: false)!)
								collectionViewImagesGrid.reloadData()
							}
						}
						
					}else if content.isVoiceRecording {
						recordingView.subviews.forEach({ view in
							view.removeFromSuperview()
						})
						initPlayerAudio(message: event.chatMessage!)
						if imagesGridCollectionView.count == 0 {
							messageWithRecording = true
						}
					}else{
						if(content.isFile && !content.isText){
							var filePath = ""
							if VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) {
								filePath = content.exportPlainFile()
							}else {
								filePath = content.filePath
							}
							let name = content.name
							if filePath == "" {
								filePath = LinphoneManager.validFilePath(name)
							}
							
							let extensionFile = filePath.lowercased().components(separatedBy: ".").last
							if (["png", "jpg", "jpeg", "bmp", "heic"].contains(extensionFile ?? "")){
								if imagesGridCollectionView.count > 1 {
									imagesGridCollectionView.append(getImageFrom(content, forReplyBubble: false)!)
									collectionViewImagesGrid.reloadData()
									
									collectionViewImagesGrid.isHidden = false
									NSLayoutConstraint.activate(imagesGridConstraints)
									imageViewBubble.image = nil
									NSLayoutConstraint.deactivate(imageConstraints)
									imageViewBubble.isHidden = true
									
								}else{
									if VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) {
										var plainFile = content.exportPlainFile()
										if let imageMessage = UIImage(named: plainFile){
											self.imageViewBubble.image = self.resizeImage(image: imageMessage, targetSize: CGSize(width: UIScreen.main.bounds.size.width*3/4, height: 300.0))
										}
										
										ChatConversationViewModel.sharedModel.removeTmpFile(filePath: plainFile)
										plainFile = ""
									}else{
										if let imageMessage = UIImage(named: content.filePath){
											self.imageViewBubble.image = self.resizeImage(image: imageMessage, targetSize: CGSize(width: UIScreen.main.bounds.size.width*3/4, height: 300.0))
										}
									}
									
									imagesGridCollectionView.append(getImageFrom(content, forReplyBubble: false)!)
									collectionViewImagesGrid.reloadData()
								}
							} else if (["mkv", "avi", "mov", "mp4"].contains(extensionFile ?? "")){
								if imagesGridCollectionView.count > 1 {
									imagesGridCollectionView.append(getImageFrom(content, forReplyBubble: false)!)
									collectionViewImagesGrid.reloadData()
									
									collectionViewImagesGrid.isHidden = false
									NSLayoutConstraint.activate(imagesGridConstraints)
									imageVideoViewBubble.image = nil
									NSLayoutConstraint.deactivate(imageConstraints)
									imageVideoViewBubble.isHidden = true
									
								}else{
									if VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) {
										var plainFile = content.exportPlainFile()
										if let imageMessage = createThumbnailOfVideoFromFileURL(videoURL: plainFile){
											imageVideoViewBubble.image = resizeImage(image: imageMessage, targetSize: CGSize(width: UIScreen.main.bounds.size.width*3/4, height: 300.0))
										}
										
										ChatConversationViewModel.sharedModel.removeTmpFile(filePath: plainFile)
										plainFile = ""
									}else{
										if let imageMessage = createThumbnailOfVideoFromFileURL(videoURL: content.filePath){
											imageVideoViewBubble.image = resizeImage(image: imageMessage, targetSize: CGSize(width: UIScreen.main.bounds.size.width*3/4, height: 300.0))
										}
									}
									imagesGridCollectionView.append(getImageFrom(content, forReplyBubble: false)!)
									collectionViewImagesGrid.reloadData()
								}
								
								if VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) {
									ChatConversationViewModel.sharedModel.removeTmpFile(filePath: filePath)
									filePath = ""
								}
							} else {
								imagesGridCollectionView.append(getImageFrom(content, forReplyBubble: false)!)
								collectionViewImagesGrid.reloadData()
								
								collectionViewImagesGrid.isHidden = false
								NSLayoutConstraint.activate(imagesGridConstraints)
								imageViewBubble.image = nil
								NSLayoutConstraint.deactivate(imageConstraints)
								imageViewBubble.isHidden = true
						 	}
						} else {
							if content.filePath == "" && content.isFileTransfer == false {
								imagesGridCollectionView.append(SwiftUtil.textToImage(drawText: "Error", inImage: UIImage(named: "file_default")!, forReplyBubble: true))
								collectionViewImagesGrid.reloadData()
								
								collectionViewImagesGrid.isHidden = false
								NSLayoutConstraint.activate(imagesGridConstraints)
								imageViewBubble.image = nil
								NSLayoutConstraint.deactivate(imageConstraints)
								imageViewBubble.isHidden = true
							} else {
								var filePathString = VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) ? content.exportPlainFile() : content.filePath
								if let urlEncoded = filePathString.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed){
									if !urlEncoded.isEmpty {
										if let urlFile = URL(string: "file://" + urlEncoded){
											do {
												let text = try String(contentsOf: urlFile, encoding: .utf8)
												imagesGridCollectionView.append(SwiftUtil.textToImage(drawText: "Error", inImage: UIImage(named: "file_default")!, forReplyBubble: true))
												collectionViewImagesGrid.reloadData()
												
												collectionViewImagesGrid.isHidden = false
												NSLayoutConstraint.activate(imagesGridConstraints)
												imageViewBubble.image = nil
												NSLayoutConstraint.deactivate(imageConstraints)
												imageViewBubble.isHidden = true
											} catch {}
										}
									}
								}
								if VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) {
									ChatConversationViewModel.sharedModel.removeTmpFile(filePath: filePathString)
									filePathString = ""
								}
							}
						}
					}
				}
				if imagesGridCollectionView.count > 0 {
					self.collectionViewImagesGrid.layoutIfNeeded()
				}
                if imagesGridCollectionView.count == 1 && recordingView.isHidden == true {
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
				
				if (recordingView.isHidden == false && imagesGridCollectionView.count > 0){
					collectionViewImagesGrid.isHidden = false
					NSLayoutConstraint.deactivate(imagesGridConstraints)
					NSLayoutConstraint.activate(imagesGridConstraintsWithRecording)
					NSLayoutConstraint.deactivate(recordingConstraints)
					NSLayoutConstraint.activate(recordingConstraintsWithMediaGrid)
					NSLayoutConstraint.deactivate(recordingWaveConstraints)
					NSLayoutConstraint.activate(recordingWaveConstraintsWithMediaGrid)
					imageViewBubble.image = nil
					imageVideoViewBubble.image = nil
					NSLayoutConstraint.deactivate(imageConstraints)
					imageVideoViewBubble.isHidden = true
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
			
			deleteItemCheckBox.isHidden = false
			deleteItemCheckBox.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -18).isActive = true
			deleteItemCheckBox.isSelected = selected
			if (event.chatMessage != nil){
				deleteItemCheckBox.matchCenterYOf(view: contentBubble).done()
			}else{
				deleteItemCheckBox.matchCenterYOf(view: contentView).done()
			}
			imageUser.isHidden = true
			contentView.onClick {
				if ChatConversationTableViewModel.sharedModel.editModeOn.value! {
					self.deleteItemCheckBox.isSelected = !self.deleteItemCheckBox.isSelected
					ChatConversationTableViewModel.sharedModel.messageListSelected.value![self.selfIndexMessage] = self.deleteItemCheckBox.isSelected
					
					if ChatConversationTableViewModel.sharedModel.messageListSelected.value![self.selfIndexMessage] == true {
						ChatConversationTableViewModel.sharedModel.messageSelected.value! += 1
					}else{
						ChatConversationTableViewModel.sharedModel.messageSelected.value! -= 1
					}
				}
			}
			deleteItemCheckBox.onClick {
				if ChatConversationTableViewModel.sharedModel.editModeOn.value! {
					self.deleteItemCheckBox.isSelected = !self.deleteItemCheckBox.isSelected
				 	ChatConversationTableViewModel.sharedModel.messageListSelected.value![self.selfIndexMessage] = self.deleteItemCheckBox.isSelected
				 
				 	if ChatConversationTableViewModel.sharedModel.messageListSelected.value![self.selfIndexMessage] == true {
					 	ChatConversationTableViewModel.sharedModel.messageSelected.value! += 1
				 	}else{
						ChatConversationTableViewModel.sharedModel.messageSelected.value! -= 1
				 	}
				}
			}
		}else{
			deleteItemCheckBox.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: 0).isActive = true
			deleteItemCheckBox.isHidden = true
			deleteItemCheckBox.width(0).done()
		}
	}
	
	func checkIfIsLinkOrPhoneNumber(content: String){
		let input = content
		
		let detector = try! NSDataDetector(types: NSTextCheckingResult.CheckingType.phoneNumber.rawValue | NSTextCheckingResult.CheckingType.link.rawValue)
		
		let regex = try! NSRegularExpression(pattern: "sips:(\\S+)")
		
		let matchesSips = detector.matches(in: input, options: [], range: NSRange(location: 0, length: input.utf16.count))
		matches = regex.matches(in: input, options: [], range: NSRange(location: 0, length: input.utf16.count))
		
		for matcheSips in matchesSips {
			matches.append(matcheSips)
		}
		
		let paragraphStyle = NSMutableParagraphStyle()
		paragraphStyle.lineSpacing = 1
		
		let attributedString = NSMutableAttributedString.init(string: content, attributes: [
			NSAttributedString.Key.font: label.font as Any
		 ])

		for match in matches {
			let linkRange = match.range
			let linkAttributes = [NSAttributedString.Key.foregroundColor: UIColor.blue, NSAttributedString.Key.underlineStyle: NSUnderlineStyle.single.rawValue] as [NSAttributedString.Key : Any]
			attributedString.setAttributes(linkAttributes, range: linkRange)
		}
		
		if matches.count > 0 {
			label.isUserInteractionEnabled = true
			let tap = UITapGestureRecognizer(target: self, action: #selector(self.handleTapOnLabel(_:)))
			label.addGestureRecognizer(tap)
		}
		
		label.attributedText = attributedString
	}
									   
	@objc func handleTapOnLabel(_ sender: UITapGestureRecognizer) {
		matches.forEach { match in
			if sender.didTapAttributedTextInLabel(label: label, inRange: match.range) {
				
				let emailRegEx = "[A-Z0-9a-z._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,64}"
				let emailTest = NSPredicate(format:"SELF MATCHES %@", emailRegEx)
				
				if let url = match.url {
					
					if url.absoluteString.hasPrefix("sip:") || url.absoluteString.hasPrefix("sips:"){
						let view: DialerView = self.VIEW(DialerView.compositeViewDescription())
						CallManager.instance().nextCallIsTransfer = true
						PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
						view.addressField.text = url.absoluteString
					}else if emailTest.evaluate(with: url.absoluteString){
						if let urlWithMailTo = URL(string: "mailto:\(url.absoluteString)") {
						  if #available(iOS 10.0, *) {
							UIApplication.shared.open(urlWithMailTo, options: [:], completionHandler: nil)
						  } else {
							UIApplication.shared.openURL(urlWithMailTo)
						  }
						}
					}else{
						if #available(iOS 10.0, *) {
							UIApplication.shared.open(url, options: [:], completionHandler: nil)
						} else {
							UIApplication.shared.openURL(url)
						}
					}
				}else if let phoneNumber = match.phoneNumber {
					let view: DialerView = self.VIEW(DialerView.compositeViewDescription())
					CallManager.instance().nextCallIsTransfer = true
					PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
					view.addressField.text = phoneNumber
				}else if let sips = label.attributedText {
					let view: DialerView = self.VIEW(DialerView.compositeViewDescription())
					CallManager.instance().nextCallIsTransfer = true
					PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
					view.addressField.text = (sips.string as NSString).substring(with: match.range)
				}
			}
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
		if window != nil {
			let marginsAndInsets = window!.safeAreaInsets.left + window!.safeAreaInsets.right + minimumInterItemSpacing * CGFloat(cellsPerRow - 1)
			layoutAttributes.bounds.size.width = ((window!.bounds.size.width - marginsAndInsets) / CGFloat(cellsPerRow)).rounded(.down)
		} else {
			layoutAttributes.bounds.size.width = (UIScreen.main.bounds.size.width / CGFloat(cellsPerRow)).rounded(.down)
		}
		return layoutAttributes
	}
	
	func createThumbnailOfVideoFromFileURL(videoURL: String) -> UIImage? {
		if let urlEncoded = videoURL.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed){
			if !urlEncoded.isEmpty {
				if let urlVideo = URL(string: "file://" + urlEncoded){
					do {
						let asset = AVAsset(url: urlVideo)
						let assetImgGenerate = AVAssetImageGenerator(asset: asset)
						assetImgGenerate.appliesPreferredTrackTransform = true
						let img = try assetImgGenerate.copyCGImage(at: CMTimeMake(value: 1, timescale: 10), actualTime: nil)
						let thumbnail = UIImage(cgImage: img)
						return thumbnail
					} catch _{
						return nil
					}
				} else {
					return nil
				}
			} else {
				return nil
			}
		} else {
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
			Log.e(error.localizedDescription)
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
			
			if(replyContentCollection[indexPath.row].type == "image" || replyContentCollection[indexPath.row].type == "video"){
				myImageView = UIImageView(image: imageCell)
			}else{
				let fileNameText = replyContentCollection[indexPath.row].name
				let fileName = SwiftUtil.textToImage(drawText:fileNameText, inImage:imageCell, forReplyBubble:true)
				myImageView = UIImageView(image: fileName)
			}
			
			myImageView.size(w: (viewCell.frame.width), h: (viewCell.frame.height)).done()
			viewCell.addSubview(myImageView)
			
			if(replyContentCollection[indexPath.row].type == "video"){
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
			let indexPathWithoutNil = indexPath.row + imagesGridCollectionViewNil
			let indexPathWithoutNilWithRecording = indexPathWithoutNil + (messageWithRecording ? 1 : 0)
			if (imagesGridCollectionView.indices.contains(indexPathWithoutNil) == true && chatMessage?.contents.indices.contains(indexPathWithoutNilWithRecording) == true){
				if ((indexPathWithoutNil <= imagesGridCollectionView.count - 1) && (imagesGridCollectionView[indexPathWithoutNil] != nil) && (chatMessage?.contents[indexPathWithoutNilWithRecording].isFile == true || chatMessage?.contents[indexPathWithoutNilWithRecording].isFileTransfer == true)) {
					let viewCell: UIView = UIView(frame: cell.contentView.frame)
					cell.addSubview(viewCell)
					if (chatMessage?.isOutgoing == false && (chatMessage?.contents[indexPathWithoutNilWithRecording].filePath == "" || chatMessage?.contents[indexPathWithoutNilWithRecording].isFileTransfer == true)) {
						let  downloadView = DownloadMessageCell()
						downloadContentCollection.append(downloadView)
						downloadView.content = chatMessage?.contents[indexPathWithoutNilWithRecording]
						downloadView.size(w: 138, h: 138).done()
						viewCell.addSubview(downloadView)

						downloadView.downloadNameLabel.text = chatMessage?.contents[indexPathWithoutNilWithRecording].name.replacingOccurrences(of: ((chatMessage?.contents[indexPathWithoutNilWithRecording].name.dropFirst(6).dropLast(8))!), with: "...")
						downloadView.setFileType(fileName: (chatMessage?.contents[indexPathWithoutNilWithRecording].name)!)
						
						let underlineAttribute = [NSAttributedString.Key.underlineStyle: NSUnderlineStyle.thick.rawValue]
						let underlineAttributedString = NSAttributedString(string: "\(VoipTexts.bubble_chat_download_file) (\(String(format: "%.1f", Float(((chatMessage?.contents[indexPathWithoutNilWithRecording].fileSize)!)) / 1000000)) Mo)", attributes: underlineAttribute)
						downloadView.downloadButtonLabel.attributedText = underlineAttributedString
						downloadView.downloadButtonLabel.onClick {
							self.chatMessage?.contents[indexPathWithoutNilWithRecording].filePath = LinphoneManager.imagesDirectory() + (((self.chatMessage?.contents[indexPathWithoutNilWithRecording].name)!))
							let _ = self.chatMessage!.downloadContent(content: (self.chatMessage?.contents[indexPathWithoutNilWithRecording])!)
						}
						downloadView.downloadButtonLabel.isUserInteractionEnabled = true
						
						if((linphone_core_get_max_size_for_auto_download_incoming_files(LinphoneManager.getLc()) > -1 && self.chatMessage!.isFileTransferInProgress) || self.chatMessage!.isOutgoing){
							downloadView.downloadButtonLabel.isHidden = true
						}
					} else if imagesGridCollectionView[indexPathWithoutNil] != nil {
						downloadContentCollection.append(nil)
						
						let myImageView = UIImageView()
						
						
						myImageView.image = getImageFrom(chatMessage?.contents[indexPathWithoutNilWithRecording], forReplyBubble: false)
						
						myImageView.size(w: (viewCell.frame.width), h: (viewCell.frame.height)).done()
						viewCell.addSubview(myImageView)
						
						myImageView.contentMode = .scaleAspectFill
						myImageView.clipsToBounds = true
						
						if (chatMessage?.isOutgoing == true && (chatMessage?.contents[indexPathWithoutNilWithRecording].filePath == "" || chatMessage?.isFileTransferInProgress == true)){
							let  uploadView = UploadMessageCell()
							uploadContentCollection.append(uploadView)
							uploadView.content = chatMessage?.contents[indexPathWithoutNilWithRecording]
							uploadView.size(w: 138, h: 138).done()
							
							if(chatMessage?.contents[indexPathWithoutNilWithRecording].type == "video"){
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
							
							viewCell.addSubview(uploadView)

						}
					}
					if(imagesGridCollectionView[indexPathWithoutNil] != nil){
						if(chatMessage?.contents[indexPathWithoutNilWithRecording].type == "video"){
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
						if chatMessage?.contents[indexPathWithoutNilWithRecording].filePath != "" {
							viewCell.onClick {
								ChatConversationTableViewModel.sharedModel.onGridClick(indexMessage: self.selfIndexMessage, index: indexPathWithoutNil)
							}
						}
					}
				}
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
		} else {
			let extensionFile = filePath.lowercased().components(separatedBy: ".").last
			if (["png", "jpg", "jpeg", "bmp", "heic"].contains(extensionFile ?? "")){
				image = UIImage(named: filePath)
			} else if (["mkv", "avi", "mov", "mp4"].contains(extensionFile ?? "")){
				image = createThumbnailOfVideoFromFileURL(videoURL: filePath)
			}
		}
		
		if VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) {
			ChatConversationViewModel.sharedModel.removeTmpFile(filePath: filePath)
			filePath = ""
		}
		
		if let img = image {
			return img
		} else {
			if (name == ""){
				return getImageFromFileName(filePath, forReplyBubble: forReplyBubble)
			} else {
				return getImageFromFileName(name, forReplyBubble: forReplyBubble)
			}
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
		if ((imagesGridCollectionView.count) > 0 && !content.isVoiceRecording){
			if  !message.isOutgoing {
				if (indexTransferProgress == -1) {
					for indexItem in 0...(imagesGridCollectionView.count) - 1 {
						if chatMessage?.contents[indexItem].name == content.name {
							indexTransferProgress = indexItem - imagesGridCollectionViewNil - (messageWithRecording ? 1 : 0)
							break
						}
					}
					
					if (indexTransferProgress > -1 && downloadContentCollection[indexTransferProgress] != nil) {
						downloadContentCollection[indexTransferProgress]!.downloadButtonLabel.isHidden = true
						downloadContentCollection[indexTransferProgress]!.circularProgressBarView.isHidden = false
					}
				}
				DispatchQueue.main.async(execute: { [self] in
					if (indexTransferProgress > -1 && offset == total) {
						downloadContentCollection[indexTransferProgress] = nil
						imagesGridCollectionView[indexTransferProgress] = getImageFrom(content, forReplyBubble: false)!
						
						
						if (imagesGridCollectionView.count <= 1){
                            if content.type == "video" {
								if VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) {
									var plainFile = content.exportPlainFile()
									if let imageMessage = createThumbnailOfVideoFromFileURL(videoURL: plainFile){
										imageVideoViewBubble.image = resizeImage(image: imageMessage, targetSize: CGSize(width: UIScreen.main.bounds.size.width*3/4, height: 300.0))
										if (imageVideoViewBubble.image != nil && imagesGridCollectionView.count <= 1){
											ChatConversationTableViewModel.sharedModel.reloadCollectionViewCell()
										}
									}
									
									ChatConversationViewModel.sharedModel.removeTmpFile(filePath: plainFile)
									plainFile = ""
								}else{
									if let imageMessage = createThumbnailOfVideoFromFileURL(videoURL: content.filePath){
										imageVideoViewBubble.image = resizeImage(image: imageMessage, targetSize: CGSize(width: UIScreen.main.bounds.size.width*3/4, height: 300.0))
										if (imageVideoViewBubble.image != nil && imagesGridCollectionView.count <= 1){
											ChatConversationTableViewModel.sharedModel.reloadCollectionViewCell()
										}
									}
								}
							} else if content.type == "image" {
								if VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) {
									var plainFile = content.exportPlainFile()
									if let imageMessage = UIImage(named: plainFile){
										imageViewBubble.image = resizeImage(image: imageMessage, targetSize: CGSize(width: UIScreen.main.bounds.size.width*3/4, height: 300.0))
										if (imageViewBubble.image != nil && imagesGridCollectionView.count <= 1 && !(linphone_core_get_max_size_for_auto_download_incoming_files(LinphoneManager.getLc()) > -1)){
											ChatConversationTableViewModel.sharedModel.reloadCollectionViewCell()
										}
									}
									
									ChatConversationViewModel.sharedModel.removeTmpFile(filePath: plainFile)
									plainFile = ""
								}else{
									if let imageMessage = UIImage(named: content.filePath){
										imageViewBubble.image = resizeImage(image: imageMessage, targetSize: CGSize(width: UIScreen.main.bounds.size.width*3/4, height: 300.0))
										if (imageViewBubble.image != nil && imagesGridCollectionView.count <= 1 && !(linphone_core_get_max_size_for_auto_download_incoming_files(LinphoneManager.getLc()) > -1)){
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
						if (indexTransferProgress > -1 && downloadContentCollection[indexTransferProgress] != nil && indexTransferProgress > -1) {
							downloadContentCollection[indexTransferProgress]!.setUpCircularProgressBarView(toValue: p)
						}
						if (indexTransferProgress == -1 && imagesGridCollectionView.count == 1 && messageWithRecording){
							indexTransferProgress = 0
							downloadContentCollection[indexTransferProgress]!.circularProgressBarView.isHidden = false
							downloadContentCollection[indexTransferProgress]!.setUpCircularProgressBarView(toValue: p)
						}
					}
				})
			} else {
				if((imagesGridCollectionView.count) > 0){
					if (indexUploadTransferProgress == -1) {
						for indexItem in 0...(imagesGridCollectionView.count) - 1 {
							if chatMessage?.contents[indexItem].filePath == content.filePath {
								indexUploadTransferProgress = indexItem - imagesGridCollectionViewNil - (messageWithRecording ? 1 : 0)
								break
							}
						}
					}
                    DispatchQueue.main.async(execute: { [self] in
						if uploadContentCollection.indices.contains(indexUploadTransferProgress){
							if (offset == total) {
								if(indexUploadTransferProgress >= 0){
									uploadContentCollection[indexUploadTransferProgress]!.circularProgressBarView.isHidden = true
								}
								if indexUploadTransferProgress <= (imagesGridCollectionView.count) + (messageWithRecording ? 1 : 0) {
									indexUploadTransferProgress += 1
								}else{
									indexUploadTransferProgress = -1
									DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
										ChatConversationTableViewModel.sharedModel.reloadCollectionViewCell()
									}
								}
							} else {
								if((imagesGridCollectionView.count) > 0 && indexUploadTransferProgress > -1){
									uploadContentCollection[indexUploadTransferProgress]!.circularProgressBarView.isHidden = false
									uploadContentCollection[indexUploadTransferProgress]!.setUpCircularProgressBarView(toValue: p)
								}
								if (indexUploadTransferProgress == -1 && imagesGridCollectionView.count == 1 && messageWithRecording){
									indexUploadTransferProgress = 0
									uploadContentCollection[indexUploadTransferProgress]!.circularProgressBarView.isHidden = false
									uploadContentCollection[indexUploadTransferProgress]!.setUpCircularProgressBarView(toValue: p)
								}
							}
						}
                    })
                }
				if((imagesGridCollectionView.count) == 1){
					var filePath = ""
					if VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) {
						filePath = content.exportPlainFile()
					}else {
						filePath = content.filePath
					}
					
					let extensionFile = filePath.lowercased().components(separatedBy: ".").last
					if (offset == total) {
						if (["png", "jpg", "jpeg", "bmp", "heic"].contains(extensionFile ?? "")){
							NSLayoutConstraint.deactivate(progressBarImageConstraints)
							circularProgressBarImageView.isHidden = true
						} else{
							NSLayoutConstraint.deactivate(progressBarVideoConstraints)
							circularProgressBarVideoView.isHidden = true
						}
						
					} else {
						if (["png", "jpg", "jpeg", "bmp", "heic"].contains(extensionFile ?? "")){
							NSLayoutConstraint.activate(progressBarImageConstraints)
							circularProgressBarImageView.isHidden = false
							circularProgressBarImageLabel.text = "\(Int(p*100))%"
							circularProgressBarImageLabel.center = CGPoint(x: 69, y: 69)
							circularProgressBarImageView.progressAnimation(fromValue: fromValue, toValue: p)
						} else{
							NSLayoutConstraint.activate(progressBarVideoConstraints)
							circularProgressBarVideoView.isHidden = false
							circularProgressBarVideoLabel.text = "\(Int(p*100))%"
							circularProgressBarVideoLabel.center = CGPoint(x: 69, y: 69)
							circularProgressBarVideoView.progressAnimation(fromValue: fromValue, toValue: p)
						}
						
						fromValue = p
					}
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
			if (previousEvent?.type != nil && (previousEvent?.type.rawValue)! != LinphoneEventLogTypeConferenceChatMessage.rawValue) {
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

extension UITapGestureRecognizer {

	func didTapAttributedTextInLabel(label: UILabel, inRange targetRange: NSRange) -> Bool {
		// Create instances of NSLayoutManager, NSTextContainer and NSTextStorage
		let layoutManager = NSLayoutManager()
		let textContainer = NSTextContainer(size: CGSize.zero)
		let textStorage = NSTextStorage(attributedString: label.attributedText!)

		// Configure layoutManager and textStorage
		layoutManager.addTextContainer(textContainer)
		textStorage.addLayoutManager(layoutManager)

		// Configure textContainer
		textContainer.lineFragmentPadding = 0.0
		textContainer.lineBreakMode = label.lineBreakMode
		textContainer.maximumNumberOfLines = label.numberOfLines
		let labelSize = label.bounds.size
		textContainer.size = labelSize

		// Find the tapped character location and compare it to the specified range
		let locationOfTouchInLabel = self.location(in: label)
		let textBoundingBox = layoutManager.usedRect(for: textContainer)
		let textContainerOffset = CGPoint(
			x: (labelSize.width - textBoundingBox.size.width) * 0.5 - textBoundingBox.origin.x,
			y: (labelSize.height - textBoundingBox.size.height) * 0.5 - textBoundingBox.origin.y
		)
		let locationOfTouchInTextContainer = CGPoint(
			x: locationOfTouchInLabel.x - textContainerOffset.x,
			y: locationOfTouchInLabel.y - textContainerOffset.y
		)
		let indexOfCharacter = layoutManager.characterIndex(for: locationOfTouchInTextContainer, in: textContainer, fractionOfDistanceBetweenInsertionPoints: nil)

		return NSLocationInRange(indexOfCharacter, targetRange)
	}

}
