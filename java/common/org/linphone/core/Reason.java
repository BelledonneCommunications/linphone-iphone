package org.linphone.core;

import java.util.Vector;

public class Reason {
	static private Vector<Reason> values = new Vector<Reason>();
	/**
	 * None (no failure)
	 */
	static public Reason None = new Reason(0,"None");
	/**
	 * No response 
	 */
	static public Reason NoResponse = new Reason(1,"NoResponse");
	/**
	 * Bad credentials
	 */
	static public Reason BadCredentials = new Reason(2,"BadCredentials");
	/**
	 * Call declined
	 */
	static public Reason Declined = new Reason(3,"Declined");
	/**
	 * Not found
	 */
	static public Reason NotFound = new Reason(4,"NotFound");
	/**
	 * Call not answered (in time).
	 */
	static public Reason NotAnswered = new Reason(5,"NotAnswered");
	/**
	 * Call not answered (in time).
	 */
	static public Reason Busy = new Reason(6,"Busy");
	/**
	 * Incompatible media
	 * */
	static public Reason Media = new Reason(7,"Media");
	/**
	 * Transport error: connection failures, disconnections etc...
	 * */
	static public Reason IOError = new Reason(8,"IOError");
	/**
	 * Transport error: connection failures, disconnections etc...
	 * */
	static public Reason DoNotDisturb = new Reason(9,"DoNotDisturb");
	/**
	 * Operation not authorized because no credentials found
	 * */
	static public Reason Unauthorized = new Reason(10,"Unauthorized");
	/**
	 * Operation was rejected by remote, for example a LinphoneCore.updateCall()
	 */
	static public Reason NotAcceptable = new Reason(11,"NotAcceptable");
	/**
	 * Operation was rejected by remote due to request unmatched to any context.
	 */
	static public Reason NoMatch = new Reason(12,"NoMatch");
	/**
	 * Resource moved permanently
	 */
	static public Reason MovedPermanently = new Reason(13,"MovedPermanently");
	/**
	 * Resource no longer exists
	 */
	static public Reason Gone = new Reason(14,"Gone");
	/**
	 * Temporarily unavailable
	 */
	static public Reason TemporarilyUnavailable = new Reason(15,"TemporarilyUnavailable");
	/**
	 * Address incomplete
	 */
	static public Reason AddressIncomplete = new Reason(16,"AddressIncomplete");
	/**
	 * Not implemented
	 */
	static public Reason NotImplemented = new Reason(17,"NotImplemented");
	/**
	 * Bad gateway
	 */
	static public Reason BadGateway = new Reason(18,"BadGateway");
	/**
	 * Server timeout
	 */
	static public Reason ServerTimeout = new Reason(19,"ServerTimeout");
	/**
	 * Unknown
	 */
	static public Reason Unknown = new Reason(20,"Unknown");
	
	protected final int mValue;
	private final String mStringValue;

	
	private Reason(int value,String stringValue) {
		mValue = value;
		values.addElement(this);
		mStringValue=stringValue;
	}
	public static Reason fromInt(int value) {
		for (int i=0; i<values.size();i++) {
			Reason state = (Reason) values.elementAt(i);
			if (state.mValue == value) return state;
		}
		throw new RuntimeException("Reason not found ["+value+"]");
	}

	public String toString() {
		return mStringValue;
	}
}
