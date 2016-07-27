/*
LinphoneContent.java
Copyright (C) 2015  Belledonne Communications, Grenoble, France

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

/**
 * LinphoneContent interface describes a SIP message content (body).
 * It can be used together with the LinphoneInfoMessage, in order to add content attachment to the INFO message.
 * @author smorlat
 *
 */
public interface LinphoneContent {
	/**
	 * Get the type of the content, for example "application"
	 * @return the type
	 */
	String getType();
	/**
	 * Get the subtype of the content, for example "html"
	 * @return the subtype
	 */
	String getSubtype();
	/**
	 * Get the encoding applied to the data, can be null if no encoding.
	**/
	String getEncoding();
	/**
	 * Get the data as a string.
	 * @return the data
	 */
	String getDataAsString();
	/**
	 * Get the data as a byte array.
	**/
	byte [] getData();
	/**
	 * Get the expected data size.
	 * @return the expected data size
	 */
	int getExpectedSize();
	
	/**
	 * Sets the expected data size
	 */
	void setExpectedSize(int size);
	
	/**
	 * Return the size of the data field
	 * @return the size of the data field
	 */
	int getRealSize();
	
	/**
	 * Set the content type, for example "application"
	 * @param type the content's primary type
	 */
	void setType(String type);
	/**
	 * Set the subtype, for example "text"
	 * @param subtype the subtype
	 */
	void setSubtype(String subtype);
	/**
	 * Set the encoding applied to the data, can be null if no encoding.
	**/
	void setEncoding(String encoding);
	/**
	 * Set the data, supplied as String.
	 * @param data the data
	 */
	void setStringData(String data);
	/**
	 * Set the data, as a byte buffer.
	**/
	void setData(byte data[]);
	
	void setName(String name);
	
	String getName();
}
