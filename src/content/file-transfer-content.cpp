/*
 * file-transfer-content.cpp
 * Copyright (C) 2010-2018 Belledonne Communications SARL
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

// TODO: Remove me later.
#include "linphone/core.h"

#include "content-p.h"
#include "file-transfer-content.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

class FileTransferContentPrivate : public ContentPrivate {
public:
	string fileName;
	string fileUrl;
	string filePath;
	FileContent *fileContent = nullptr;
	size_t fileSize = 0;
	std::vector<char> fileKey;
};

// -----------------------------------------------------------------------------

FileTransferContent::FileTransferContent () : Content(*new FileTransferContentPrivate) {}

FileTransferContent::FileTransferContent (const FileTransferContent &src) : Content(*new FileTransferContentPrivate) {
	L_D();
	d->fileName = src.getFileName();
	d->fileUrl = src.getFileUrl();
	d->filePath = src.getFilePath();
	d->fileContent = src.getFileContent();
	d->fileSize = src.getFileSize();
	d->fileKey = src.getFileKey();
}

FileTransferContent::FileTransferContent (FileTransferContent &&src) : Content(*new FileTransferContentPrivate) {
	L_D();
	d->fileName = move(src.getPrivate()->fileName);
	d->fileUrl = move(src.getPrivate()->fileUrl);
	d->filePath = move(src.getPrivate()->filePath);
	d->fileContent = move(src.getPrivate()->fileContent);
	d->fileSize = move(src.getPrivate()->fileSize);
	d->fileKey = move(src.getPrivate()->fileKey);
}

FileTransferContent &FileTransferContent::operator= (const FileTransferContent &src) {
	L_D();
	if (this != &src) {
		Content::operator=(src);
		d->fileName = src.getFileName();
		d->fileUrl = src.getFileUrl();
		d->filePath = src.getFilePath();
		d->fileContent = src.getFileContent();
		d->fileSize = src.getFileSize();
		d->fileKey = src.getFileKey();
	}

	return *this;
}

FileTransferContent &FileTransferContent::operator= (FileTransferContent &&src) {
	L_D();
	Content::operator=(move(src));
	d->fileName = move(src.getPrivate()->fileName);
	d->fileUrl = move(src.getPrivate()->fileUrl);
	d->filePath = move(src.getPrivate()->filePath);
	d->fileContent = move(src.getPrivate()->fileContent);
	d->fileSize = move(src.getPrivate()->fileSize);
	d->fileKey = move(src.getPrivate()->fileKey);
	return *this;
}

bool FileTransferContent::operator== (const FileTransferContent &content) const {
	L_D();
	return Content::operator==(content) &&
		d->fileName == content.getFileName() &&
		d->fileUrl == content.getFileUrl() &&
		d->filePath == content.getFilePath() &&
		d->fileSize == content.getFileSize();
}

void FileTransferContent::setFileName (const string &name) {
	L_D();
	d->fileName = name;
}

const string &FileTransferContent::getFileName () const {
	L_D();
	return d->fileName;
}

void FileTransferContent::setFileUrl (const string &url) {
	L_D();
	d->fileUrl = url;
}

const string &FileTransferContent::getFileUrl () const {
	L_D();
	return d->fileUrl;
}

void FileTransferContent::setFilePath (const string &path) {
	L_D();
	d->filePath = path;
}

const string &FileTransferContent::getFilePath () const {
	L_D();
	return d->filePath;
}

void FileTransferContent::setFileContent (FileContent *content) {
	L_D();
	d->fileContent = content;
}

FileContent *FileTransferContent::getFileContent () const {
	L_D();
	return d->fileContent;
}

void FileTransferContent::setFileSize (size_t size) {
	L_D();
	d->fileSize = size;
}

size_t FileTransferContent::getFileSize () const {
	L_D();
	return d->fileSize;
}

void FileTransferContent::setFileKey (const char *key, size_t size) {
	L_D();
	d->fileKey = vector<char>(key, key + size);
}

const vector<char> &FileTransferContent::getFileKey () const {
	L_D();
	return d->fileKey;
}

const char *FileTransferContent::getFileKeyAsString() const {
	L_D();
	return d->fileKey.data();
}

bool FileTransferContent::isFile () const {
	return false;
}

LINPHONE_END_NAMESPACE
