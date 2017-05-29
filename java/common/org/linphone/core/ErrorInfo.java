package org.linphone.core;

public interface ErrorInfo {
	/**
	 * Return the Reason enum corresponding to the error type.
	 * @return the reason.
	 */
	Reason getReason();
	/**
	 * Get the protocol name for which the error code (returned by getProtocolCode()) is meaningful.
	 **/
	String getProtocol();
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
	 * Get content of warning header, if any.
	**/
	String getWarnings();
	/**
	 * Get additional error information, which is provided as a Reason header in SIP response.
	**/
	ErrorInfo getSubErrorInfo();

	/**
	 * Set Reason enum.
	 */
	void setReason(Reason r);
	/**
	 * Set the protocol name for which the error code (set by setProtocolCode()) is meaningful.
	 **/
	void setProtocol(String protocol);
	/**
	 * Set the protocol code corresponding to the error (typically a SIP status code).
	 */
	void setProtocolCode(int code);
	/**
	 * Set the reason-phrase provided by the protocol (typically a SIP reason-phrase).
	 */
	void setPhrase(String phrase);
	/**
	 * Set warnings.
	**/
	void setWarnings(String warnings);
	/**
	 * Set additional error information, which is provided as a Reason header in SIP response.
	**/
	void setSubErrorInfo(ErrorInfo sub_ei);

	/**
	 * Get details about the error, if provided by the protocol. For SIP it consists of the content of a Warning or Reason header.
	 * @return details about the error.
	 * @deprecated
	 */
	String getDetails();
}
