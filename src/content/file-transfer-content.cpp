/*
 * file-transfer-content.cpp
 * Copyright (C) 2010-2017 Belledonne Communications SARL
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "content-p.h"
#include "file-transfer-content.h"
#include "linphone/core.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class FileTransferContentPrivate : public ContentPrivate {
public:
	string fileUrl;
	string filePath;
	FileContent *fileContent = nullptr;
};

// -----------------------------------------------------------------------------

FileTransferContent::FileTransferContent() : Content(*new FileTransferContentPrivate()) {

}

FileTransferContent::FileTransferContent (const FileTransferContent &src) : Content(*new FileTransferContentPrivate) {
	L_D();
	d->fileUrl = src.getFileUrl();
	d->filePath = src.getFilePath();
	d->fileContent = src.getFileContent();
}

FileTransferContent::FileTransferContent (FileTransferContent &&src) : Content(*new FileTransferContentPrivate) {
	L_D();
	d->fileUrl = move(src.getPrivate()->fileUrl);
	d->filePath = move(src.getPrivate()->filePath);
	d->fileContent = move(src.getPrivate()->fileContent);
}

FileTransferContent &FileTransferContent::operator= (const FileTransferContent &src) {
	L_D();
	if (this != &src) {
		Content::operator=(src);
		d->fileUrl = src.getFileUrl();
		d->filePath = src.getFilePath();
		d->fileContent = src.getFileContent();
	}

	return *this;
}

FileTransferContent &FileTransferContent::operator= (FileTransferContent &&src) {
	L_D();
	Content::operator=(move(src));
	d->fileUrl = move(src.getPrivate()->fileUrl);
	d->filePath = move(src.getPrivate()->filePath);
	d->fileContent = move(src.getPrivate()->fileContent);
	return *this;
}

bool FileTransferContent::operator== (const FileTransferContent &content) const {
	L_D();
	return Content::operator==(content) &&
		d->fileUrl == content.getFileUrl() &&
		d->filePath == content.getFilePath();
}

void FileTransferContent::setFileUrl(const string &url) {
	L_D();
	d->fileUrl = url;
}

const string& FileTransferContent::getFileUrl() const {
	L_D();
	return d->fileUrl;
}

void FileTransferContent::setFilePath(const string &path) {
	L_D();
	d->filePath = path;
}

const string& FileTransferContent::getFilePath() const {
	L_D();
	return d->filePath;
}

void FileTransferContent::setFileContent(FileContent *content) {
	L_D();
	d->fileContent = content;
}

FileContent* FileTransferContent::getFileContent() const {
	L_D();
	return d->fileContent;
}

LinphoneContent * FileTransferContent::toLinphoneContent() const {
	LinphoneContent* content;
	content = linphone_core_create_content(NULL);
	linphone_content_set_type(content, getContentType().getType().c_str());
	linphone_content_set_subtype(content, getContentType().getSubType().c_str());
	return content;
}

LINPHONE_END_NAMESPACE
