/*
 * file-content.cpp
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
#include "file-content.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

class FileContentPrivate : public ContentPrivate {
public:
	string fileName;
	string filePath;
	size_t fileSize = 0;
	string fileKey;
};

// -----------------------------------------------------------------------------

FileContent::FileContent () : Content(*new FileContentPrivate) {}

FileContent::FileContent (const FileContent &other) : Content(*new FileContentPrivate) {
	L_D();
	d->fileName = other.getFileName();
	d->filePath = other.getFilePath();
	d->fileSize = other.getFileSize();
	d->fileKey = other.getFileKey();
}

FileContent::FileContent (FileContent &&other) : Content(*new FileContentPrivate) {
	L_D();
	d->fileName = move(other.getPrivate()->fileName);
	d->filePath = move(other.getPrivate()->filePath);
	d->fileSize = move(other.getPrivate()->fileSize);
	d->fileKey = move(other.getPrivate()->fileKey);
}

FileContent &FileContent::operator= (const FileContent &other) {
	L_D();
	if (this != &other) {
		Content::operator=(other);
		d->fileName = other.getFileName();
		d->filePath = other.getFilePath();
		d->fileSize = other.getFileSize();
		d->fileKey = other.getFileKey();
	}

	return *this;
}

FileContent &FileContent::operator= (FileContent &&other) {
	L_D();
	Content::operator=(move(other));
	d->fileName = move(other.getPrivate()->fileName);
	d->filePath = move(other.getPrivate()->filePath);
	d->fileSize = move(other.getPrivate()->fileSize);
	d->fileKey = move(other.getPrivate()->fileKey);
	return *this;
}

bool FileContent::operator== (const FileContent &other) const {
	L_D();
	return Content::operator==(other) &&
		d->fileName == other.getFileName() &&
		d->filePath == other.getFilePath() &&
		d->fileSize == other.getFileSize() &&
		d->fileKey == other.getFileKey();
}

void FileContent::setFileSize (size_t size) {
	L_D();
	d->fileSize = size;
}

size_t FileContent::getFileSize () const {
	L_D();
	return d->fileSize;
}

void FileContent::setFileName (const string &name) {
	L_D();
	d->fileName = name;
}

const string &FileContent::getFileName () const {
	L_D();
	return d->fileName;
}

void FileContent::setFilePath (const string &path) {
	L_D();
	d->filePath = path;
}

const string &FileContent::getFilePath () const {
	L_D();
	return d->filePath;
}

void FileContent::setFileKey (const string &key) {
	L_D();
	d->fileKey = key;
}

const string &FileContent::getFileKey () const {
	L_D();
	return d->fileKey;
}

bool FileContent::isFile () const {
	return true;
}

LinphoneContent *FileContent::toLinphoneContent () const {
	LinphoneContent *content = linphone_core_create_content(nullptr);
	linphone_content_set_type(content, getContentType().getType().c_str());
	linphone_content_set_subtype(content, getContentType().getSubType().c_str());
	linphone_content_set_name(content, getFileName().c_str());
	linphone_content_set_size(content, getFileSize());
	linphone_content_set_key(content, getFileKey().c_str(), getFileKey().size());
	return content;
}

LINPHONE_END_NAMESPACE
