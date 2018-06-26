/*
 * file-transfer-content.h
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

#ifndef _L_FILE_TRANSFER_CONTENT_H_
#define _L_FILE_TRANSFER_CONTENT_H_

#include <vector>

#include "content.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class FileContent;
class FileTransferContentPrivate;

class LINPHONE_PUBLIC FileTransferContent : public Content {
public:
	FileTransferContent ();
	FileTransferContent (const FileTransferContent &other);
	FileTransferContent (FileTransferContent &&other);

	FileTransferContent &operator= (const FileTransferContent &other);
	FileTransferContent &operator= (FileTransferContent &&other);

	bool operator== (const FileTransferContent &other) const;

	void setFileName (const std::string &name);
	const std::string &getFileName () const;

	void setFileUrl (const std::string &url);
	const std::string &getFileUrl () const;

	void setFilePath (const std::string &path);
	const std::string &getFilePath () const;

	void setFileContent (FileContent *content);
	FileContent *getFileContent () const;

	void setFileSize (size_t size);
	size_t getFileSize () const;

	void setFileKey (const char *key, size_t size);
	const std::vector<char> &getFileKey () const;
	size_t getFileKeySize() const;

	bool isFile () const override;
	bool isFileTransfer () const override;

private:
	L_DECLARE_PRIVATE(FileTransferContent);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_FILE_TRANSFER_CONTENT_H_
