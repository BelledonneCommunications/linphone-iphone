package org.linphone.p2pproxy.core.media;

import org.linphone.p2pproxy.api.P2pProxyException;

public class MediaResoureUnreachableException extends P2pProxyException {

	private String mRourceAddress;
	public MediaResoureUnreachableException() {
		super();
		// TODO Auto-generated constructor stub
	}

	public MediaResoureUnreachableException(String arg0, Throwable arg1) {
		super(arg0, arg1);
		// TODO Auto-generated constructor stub
	}

	public MediaResoureUnreachableException(String arg0) {
		super(arg0);
		// TODO Auto-generated constructor stub
	}

	public MediaResoureUnreachableException(Throwable arg0) {
		super(arg0);
		// TODO Auto-generated constructor stub
	}
	public void setRourceAddress(String anAddress) {
		mRourceAddress = anAddress;
	}
	public String getResourceAddress() {
		return mRourceAddress;
	}

}
