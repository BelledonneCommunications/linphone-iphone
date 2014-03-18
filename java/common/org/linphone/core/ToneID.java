package org.linphone.core;

public enum ToneID {
	Undefined(0),
	Busy(1),
	CallWaiting(2),
	CallOnHold(3),
	CallLost(4);
	protected final int mValue;
	private ToneID(int value){
		mValue=value;
	}
	static protected ToneID fromInt(int value) throws LinphoneCoreException{
		switch(value){
		case 0: return Undefined;
		case 1: return Busy;
		case 2: return CallWaiting;
		case 3: return CallOnHold;
		case 4: return CallLost;
		default:
			throw new LinphoneCoreException("Unhandled enum value "+value+" for LinphoneToneID");
		}
	}
}
