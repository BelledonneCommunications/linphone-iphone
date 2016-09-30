package org.linphone.core;

public interface OpenH264DownloadHelperListener {
	/**
	 * Called at the beginning of download with current &lt; max Called
	 * at each iteration of download Called at the ending of download
	 * with current &gt; max
	 * @param current: Size of file already downloaded
	 * @param max: Size of file we want to download
	 */
	void OnProgress(int current, int max);
	
	/**
	 * Called when we failed to download codec
	 * @param error: Error message
	 */
	void OnError(String error);
}
