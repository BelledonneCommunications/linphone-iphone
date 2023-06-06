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
