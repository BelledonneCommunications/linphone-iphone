package org.linphone.core;

public enum PublishState {
	/**
	 * Initial state, should not be used.
	 */
	None(0), 
	/**
	 * Publish is in progress.
	 */
	Progress(1),
	/**
	 * Publish succeeded.
	 */
	Ok(2), 
	/**
	 * Publish encountered an error. {@link LinphoneEvent.getReason()} gives more information about failure.
	 */
	Error(3),
	/**
	 * Publish is about to expire. Application can trigger a refresh by calling {@link LinphoneCore.updatePublish()}
	 * [sip]->refresh_generic_subscribe property is set to 0.
	 */
	Expiring(4),
	/**
	 * Publish is terminated cleared.
	 */
	Cleared(5);
	
	protected final int mValue;
	private PublishState(int value){
		mValue=value;
	}
	static protected PublishState fromInt(int value) throws LinphoneCoreException{
		switch(value){
		case 0: return None;
		case 1: return Progress;
		case 2: return Ok;
		case 3: return Error;
		case 4: return Expiring;
		case 5: return Cleared;
		default:
			throw new LinphoneCoreException("Unhandled enum value "+value+" for PublishState");
		}
	}
}
