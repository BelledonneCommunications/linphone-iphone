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
};

// -----------------------------------------------------------------------------

FileContent::FileContent () : Content(*new FileContentPrivate) {}

FileContent::FileContent (const FileContent &src) : Content(*new FileContentPrivate) {
	L_D();
	d->fileName = src.getFileName();
	d->filePath = src.getFilePath();
	d->fileSize = src.getFileSize();
}

FileContent::FileContent (FileContent &&src) : Content(*new FileContentPrivate) {
	L_D();
	d->fileName = move(src.getPrivate()->fileName);
	d->filePath = move(src.getPrivate()->filePath);
	d->fileSize = move(src.getPrivate()->fileSize);
}

FileContent &FileContent::operator= (const FileContent &src) {
	L_D();
	if (this != &src) {
		Content::operator=(src);
		d->fileName = src.getFileName();
		d->filePath = src.getFilePath();
		d->fileSize = src.getFileSize();
	}

	return *this;
}

FileContent &FileContent::operator= (FileContent &&src) {
	L_D();
	Content::operator=(move(src));
	d->fileName = move(src.getPrivate()->fileName);
	d->filePath = move(src.getPrivate()->filePath);
	d->fileSize = move(src.getPrivate()->fileSize);
	return *this;
}

bool FileContent::operator== (const FileContent &content) const {
	L_D();
	return Content::operator==(content) &&
		d->fileName == content.getFileName() &&
		d->filePath == content.getFilePath() &&
		d->fileSize == content.getFileSize();
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

LinphoneContent *FileContent::toLinphoneContent () const {
	LinphoneContent *content = linphone_core_create_content(nullptr);
	linphone_content_set_type(content, getContentType().getType().c_str());
	linphone_content_set_subtype(content, getContentType().getSubType().c_str());
	linphone_content_set_name(content, getFileName().c_str());
	linphone_content_set_size(content, getFileSize());
	return content;
}

LINPHONE_END_NAMESPACE
