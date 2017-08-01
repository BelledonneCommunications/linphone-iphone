/**
 *
 */
package org.linphone.core;

/**
 * @author François Grisez
 *
 */
public class LinphonePlayerImpl implements LinphonePlayer {
	private long nativePtr = 0;

	private native void init(long nativePtr);
	LinphonePlayerImpl(long nativePtr) {
		this.nativePtr = nativePtr;
		init(nativePtr);
	}

	private native int open(long nativePtr, String filename);
	@Override
	public synchronized int open(String filename) {
		return open(nativePtr, filename);
	}

	private native int start(long nativePtr);
	@Override
	public synchronized int start() {
		return start(nativePtr);
	}

	private native int pause(long nativePtr);
	@Override
	public synchronized int pause() {
		return pause(nativePtr);
	}

	private native int seek(long nativePtr, int timeMs);
	@Override
	public synchronized int seek(int timeMs) {
		return seek(nativePtr, timeMs);
	}

	private native int getState(long nativePtr);
	@Override
	public synchronized State getState() {
		return LinphonePlayer.State.fromValue(getState(nativePtr));
	}

	private native int getDuration(long nativePtr);
	@Override
	public synchronized int getDuration() {
		return getDuration(nativePtr);
	}

	private native int getCurrentPosition(long nativePtr);
	@Override
	public synchronized int getCurrentPosition() {
		return getCurrentPosition(nativePtr);
	}

	private native void close(long nativePtr);
	@Override
	public synchronized void close() {
		close(nativePtr);
	}

	private native void destroy(long nativePtr);
	@Override
	protected void finalize() {
		destroy(nativePtr);
	}
}
