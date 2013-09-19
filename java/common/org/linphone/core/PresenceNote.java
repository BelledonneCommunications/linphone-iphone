/*
PresenceNote.java
Copyright (C) 2010-2013  Belledonne Communications, Grenoble, France

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

package org.linphone.core;

public interface PresenceNote {

	/**
	 * @brief Gets the content of a presence note.
	 * @return A String with the content of the presence note.
	 */
	String getContent();

	/**
	 * @brief Sets the content of a presence note.
	 * @param[in] content The content of the note.
	 * @return 0 if successful, a value < 0 in case of error.
	 */
	int setContent(String content);

	/**
	 * @brief Gets the language of a presence note.
	 * @return A String containing the language of the presence note, or null if no language is specified.
	 */
	String getLang();

	/**
	 * @brief Sets the language of a presence note.
	 * @param[in] lang The language of the note.
	 * @return 0 if successful, a value < 0 in case of error.
	 */
	int setLang(String lang);

	/**
	 * @brief Gets the native pointer for this object.
	 */
	long getNativePtr();

}
