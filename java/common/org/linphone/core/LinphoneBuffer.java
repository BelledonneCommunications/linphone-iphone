package org.linphone.core;

/**
 * The LinphoneContent object representing a data buffer.
**/
public interface LinphoneBuffer {

	byte[] getContent();
	
	void setContent(byte[] data);
	
	int getSize();
	
	void setSize(int size);
}
