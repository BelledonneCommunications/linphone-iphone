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
