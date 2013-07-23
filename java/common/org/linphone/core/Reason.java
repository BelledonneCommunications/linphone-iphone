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
