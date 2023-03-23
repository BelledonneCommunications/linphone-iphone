//
//  DownloadMessageCell.swift
//  linphone
//
//  Created by Benoît Martins on 21/03/2023.
//

import Foundation
import linphonesw

class DownloadMessageCell: UIView {
	let downloadStackView = UIStackView()
	let downloadView = UIView()
	var downloadImageView = UIImageView(image: UIImage(named: "file_default"))
	let downloadNameLabel = StyledLabel(VoipTheme.chat_conversation_download_button)
	let downloadButtonLabel = StyledLabel(VoipTheme.chat_conversation_download_button)
	var circularProgressBarView = CircularProgressBarView()
	let circularProgressBarLabel = StyledLabel(VoipTheme.chat_conversation_download_progress_text)
	let downloadSpacing = UIView()
	
	var content: Content? = nil
	var fromValue : Float = 0.0
	
	override init(frame: CGRect) {
		super.init(frame: frame)
		downloadStackView.axis = .vertical
		downloadStackView.distribution = .fill
		downloadStackView.alignment = .center
		
		addSubview(downloadStackView)
		downloadStackView.addArrangedSubview(downloadView)
		downloadView.addSubview(downloadImageView)
		downloadStackView.addArrangedSubview(downloadNameLabel)
		downloadStackView.addArrangedSubview(downloadButtonLabel)
		downloadStackView.addArrangedSubview(downloadSpacing)
		
		downloadStackView.backgroundColor = VoipTheme.backgroundWhiteBlack.get()
		
		downloadStackView.size(w: 138, h: 138).done()
		downloadView.size(w: 138, h: 80).done()
		downloadNameLabel.size(w: 130, h: 22).done()
		downloadButtonLabel.size(w: 130, h: 22).done()
		downloadSpacing.size(w: 138, h: 14).done()
		downloadImageView.center = CGPoint(x: 69, y: 40)
		
		addSubview(circularProgressBarView)
		circularProgressBarView.isHidden = true
		circularProgressBarLabel.text = "0%"
		circularProgressBarLabel.size(w: 30, h: 30).done()
		circularProgressBarView.addSubview(circularProgressBarLabel)
	}
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
    
    func setFileType(fileName: String) {
        let extensionFile = fileName.lowercased().components(separatedBy: ".").last

        if extensionFile == "pdf" {
            downloadImageView.image = UIImage(named: "file_pdf_default")
        } else if ["png", "jpg", "jpeg", "bmp", "heic"].contains(extensionFile ?? "") {
            downloadImageView.image = UIImage(named: "file_picture_default")
        } else if ["mkv", "avi", "mov", "mp4"].contains(extensionFile ?? "") {
            downloadImageView.image = UIImage(named: "file_video_default")
            downloadImageView.frame = CGRect(x: 0, y: 0, width: 50, height: 40)
            downloadImageView.center = CGPoint(x: 69, y: 40)
        } else if ["wav", "au", "m4a"].contains(extensionFile ?? "") {
            downloadImageView.image = UIImage(named: "file_audio_default")
        } else {
            downloadImageView.image = UIImage(named: "file_default")
        }
    }
	
	func setUpCircularProgressBarView(toValue: Float) {
		
		circularProgressBarLabel.text = "\(Int(toValue*100))%"
		circularProgressBarLabel.center = CGPoint(x: 69, y: 69)
		

		circularProgressBarView.progressAnimation(fromValue: fromValue, toValue: toValue)
		fromValue = toValue
	}
}
