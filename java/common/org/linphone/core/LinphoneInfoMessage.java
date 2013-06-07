package org.linphone.core;

/**
 * The LinphoneInfoMessage represents an informational message (INFO) to be transmitted or received by the LinphoneCore.
 * It can be created with {@link LinphoneCore.createInfoMessage() }.
 * @author smorlat
 *
 */
public interface LinphoneInfoMessage {
	/**
	 * Assign a content to the info message. This is optional.
	 * @param content
	 */
	void setContent(LinphoneContent content);
	/**
	 * Get the actual content of the info message. It may be null.
	 * @return a LinphoneContent object or null
	 */
	LinphoneContent getContent();
	/**
	 * Add a specific header to the info message
	 * @param name the header's name
	 * @param value the header's value
	 */
	void addHeader(String name, String value);
	/**
	 * Retrieve a header's value based on its name.
	 * @param name the header's name
	 * @return the header's value
	 */
	String getHeader(String name);
}
