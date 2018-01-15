/*
 * paths-apple.mm
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

#import "linphone/utils/utils.h"

#import "core/platform-helpers/platform-helpers.h"
#import "logger/logger.h"
#import "paths-apple.h"

#import <Foundation/Foundation.h>

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

std::string SysPaths::getDataPath (PlatformHelpers *platformHelper) {
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
	NSString *writablePath = [paths objectAtIndex:0];
	NSString *fullPath = [writablePath stringByAppendingString:@"/linphone/"];
	if(![[NSFileManager defaultManager] fileExistsAtPath:fullPath]) {
		NSError *error;
		lInfo() << "Data path " << fullPath.UTF8String << " does not exist, creating it.";
		if (![[NSFileManager defaultManager] createDirectoryAtPath:fullPath
	                                 withIntermediateDirectories:YES
	                                                  attributes:nil
	                                                       error:&error]) {
			lError() << "Create data path directory error: " << error.description;
		}
	}

	const char *ret = fullPath.UTF8String;
	return ret;
}

std::string SysPaths::getConfigPath (PlatformHelpers *platformHelper) {
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
	NSString *configPath = [paths objectAtIndex:0];
	NSString *fullPath = [configPath stringByAppendingString:@"/Preferences/linphone/"];
	if(![[NSFileManager defaultManager] fileExistsAtPath:fullPath]) {
		NSError *error;
		lInfo() << "Config path " << fullPath.UTF8String << " does not exist, creating it.";
		if (![[NSFileManager defaultManager] createDirectoryAtPath:fullPath
	                                 withIntermediateDirectories:YES
	                                                  attributes:nil
	                                                       error:&error]) {
			lError() << "Create config path directory error: " << error.description;
		}
	}

	const char *ret = fullPath.UTF8String;
	return ret;
}

LINPHONE_END_NAMESPACE
