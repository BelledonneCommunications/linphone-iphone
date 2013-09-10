/*
PresenceService.java
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

public interface PresenceService {

	/**
	 * @brief Gets the basic status of a presence service.
	 * @return The #PresenceBasicStatus of the #PresenceService object.
	 */
	PresenceBasicStatus getBasicStatus();

	/**
	 * @brief Sets the basic status of a presence service.
	 * @param[in] status The #PresenceBasicStatus to set for the #PresenceService object.
	 * @return 0 if successful, a value < 0 in case of error.
	 */
	int setBasicStatus(PresenceBasicStatus status);

	/**
	 * @brief Gets the contact of a presence service.
	 * @return A string containing the contact, or null if no contact is found.
	 */
	String getContact();

	 /**
	 * @brief Sets the contact of a presence model.
	 * @param[in] contact The contact string to set.
	 * @return 0 if successful, a value < 0 in case of error.
	 */
	int setContact(String contact);

	/**
	 * @brief Gets the native pointer for this object.
	 */
	long getNativePtr();

}
