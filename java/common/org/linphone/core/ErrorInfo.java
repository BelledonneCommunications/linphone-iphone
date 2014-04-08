package org.linphone.core;

public interface ErrorInfo {
	/**
	 * Return the Reason enum corresponding to the error type.
	 * @return the reason.
	 */
	Reason getReason();
	/**
	 * Get the protocol code corresponding to the error (typically a SIP status code).
	 * @return the code.
	 */
	int getProtocolCode();
	/**
	 * Get the reason-phrase provided by the protocol (typically a SIP reason-phrase).
	 * @return the reason phrase.
	 */
	String getPhrase();
	/**
	 * Get details about the error, if provided by the protocol. For SIP it consists of the content of a Warning or Reason header.
	 * @return details about the error.
	 */
	String getDetails();
}
