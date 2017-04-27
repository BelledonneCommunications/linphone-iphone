/**
 * Interface to manipulate different media players of Linphone
 */
package org.linphone.core;

/**
 * @author François Grisez
 *
 */
public interface LinphonePlayer {
	/**
	 * States that the player can be
	 * @author François Grisez
	 *
	 */
	public enum State {
		closed, /*< No file is open */
		paused, /*< A file is open and playback is not running */
		playing; /*< A file is open and playback is running */

		public static State fromValue(int value) {
			if(value == 0) {
				return closed;
			} else if(value == 1) {
				return paused;
			} else if(value == 2) {
				return playing;
			} else {
				return null;
			}
		}
	};

	/**
	 * Open a file
	 * @param filename Name of the file to open
	 * @return 0 on success, -1 on failure
	 */
	public int open(String filename);

	/**
	 * Start playback
	 * @return 0 on success, -1 on failure
	 */
	public int start();

	/**
	 * Get playback paused
	 * @return 0 on success, -1 on failure
	 */
	public int pause();

	/**
	 * Go to a specific position in the timeline
	 * @param timeMs Time in milliseconds
	 * @return 0 on success, -1 on failure
	 */
	public int seek(int timeMs);

	/**
	 * Get the state of the player
	 * @return See State enumeration
	 */
	public State getState();

	/**
	 * Get the duration of the media
	 * @return The duration in milliseconds
	 */
	public int getDuration();

	/**
	 * Get the position of the playback
	 * @return The position in milliseconds
	 */
	public int getCurrentPosition();

	/**
	 * Close a file
	 */
	public void close();
}
