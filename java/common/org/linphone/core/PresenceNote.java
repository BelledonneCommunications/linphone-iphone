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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

package org.linphone.core;

public interface PresenceNote {

	/**
	 * Gets the content of a presence note.
	 * @return A String with the content of the presence note.
	 */
	String getContent();

	/**
	 * Sets the content of a presence note.
	 * @param content The content of the note.
	 * @return 0 if successful, a value &lt; 0 in case of error.
	 */
	int setContent(String content);

	/**
	 * Gets the language of a presence note.
	 * @return A String containing the language of the presence note, or null if no language is specified.
	 */
	String getLang();

	/**
	 * Sets the language of a presence note.
	 * @param lang The language of the note.
	 * @return 0 if successful, a value &lt; 0 in case of error.
	 */
	int setLang(String lang);

	/**
	 * Gets the native pointer for this object.
	 */
	long getNativePtr();

}
