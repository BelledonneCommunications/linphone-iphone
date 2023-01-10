//
//  FileType.swift
//  linphone
//
//  Created by Benoît Martins on 10/01/2023.
//

import Foundation

enum FileType : String {
	case pdf = "pdf";
	case png = "png";
	case jpg = "jpg";
	case jpeg = "jpeg";
	case bmp = "bmp";
	case heic = "heic";
	case mkv = "mkv";
	case avi = "avi";
	case mov = "mov";
	case mp4 = "mp4";
	case wav = "wav";
	case au = "au";
	case m4a = "m4a";
	case other = "other";
	
	case file_pdf_default = "file_pdf_default";
	case file_picture_default = "file_picture_default";
	case file_video_default = "file_video_default";
	case file_audio_default = "file_audio_default";
	case file_default = "file_default";

	func getGroupTypeFromFile() -> String? {
		switch self {
		case .pdf, .file_pdf_default:
			return "file_pdf_default"

		case .png, .jpg, .jpeg, .bmp, .heic, .file_picture_default:
			return "file_picture_default"

		case .mkv, .avi, .mov, .mp4, .file_video_default:
			return "file_video_default"

		case .wav, .au, .m4a, .file_audio_default:
			return "file_audio_default"

		default:
			return "file_default"
		}
	}

	func getImageFromFile() -> UIImage? {
		switch self {
		case .pdf, .file_pdf_default:
			return UIImage(named:"file_pdf_default")

		case .png, .jpg, .jpeg, .bmp, .heic, .file_picture_default:
			return UIImage(named:"file_picture_default")

		case .mkv, .avi, .mov, .mp4, .file_video_default:
			return UIImage(named:"file_video_default")

		case .wav, .au, .m4a, .file_audio_default:
			return UIImage(named:"file_audio_default")

		default:
			return UIImage(named:"file_default")
		}
	}
}

extension FileType {
	init() {
	  self = .file_default
	}

	init?(_ value: String) {
		switch value.lowercased() {
		case "pdf", "file_pdf_default":
			self = .file_pdf_default

		case "png", "jpg", "jpeg", "bmp", "heic", "file_picture_default":
			self = .file_picture_default

		case "mkv", "avi", "mov", "mp4", "file_video_default":
			self = .file_video_default

		case "wav", "au", "m4a", "file_audio_default":
			self = .file_audio_default

		default:
			self = .file_default
		}
	}
}
