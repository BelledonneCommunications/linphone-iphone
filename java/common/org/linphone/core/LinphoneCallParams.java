/*
LinphoneCallParameters.java
Copyright (C) 2010  Belledonne Communications, Grenoble, France

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
package org.linphone.core;
import org.linphone.core.LinphoneCore.MediaEncryption;
/**
 * The LinphoneCallParams is an object containing various call related parameters.
 * It can be used to retrieve parameters from a currently running call or modify the call's characteristics
 * dynamically.
 * @author Guillaume Beraudo
 *
 */
public interface LinphoneCallParams {
	void setVideoEnabled(boolean b);
	boolean getVideoEnabled();
	
	/**
	 * set audio bandwidth in kbits/s
	 * @param value 0 to disable limitation
	 */
	void setAudioBandwidth(int value);

	/**
	 * return selected media encryption
	 * @return MediaEncryption.None MediaEncryption.SRTP or MediaEncryption.ZRTP
	 */
	MediaEncryption getMediaEncryption();
	/**
	 * set media encryption (rtp) to use
	 * @params menc: MediaEncryption.None, MediaEncryption.SRTP or MediaEncryption.ZRTP
	 */
	void setMediaEnctyption(MediaEncryption menc);

	/**
	 * Get the currently used audio codec
	 * @return PayloadType or null
	 */
	PayloadType getUsedAudioCodec();

	/**
	 * Get the currently used video codec
	 * @return PayloadType or null
	 */
	PayloadType getUsedVideoCodec();
	
	/**
	 * Indicate low bandwith mode. 
	 * Configuring a call to low bandwidth mode will result in the core to activate several settings for the call in order to ensure that bitrate usage
	 * is lowered to the minimum possible. Typically, ptime (packetization time) will be increased, audio codec's output bitrate will be targetted to 20kbit/s provided
	 * that it is achievable by the codec selected after SDP handshake. Video is automatically disabled.
	**/
	void enableLowBandwidth(boolean enable);
	
	/**
	 * Use to know if this call has been configured in low bandwidth mode.
	 * <br> When enabled, this param may transform a call request with video in audio only mode.
	 * @return true if low bandwidth has been configured/detected
	 */
	boolean isLowBandwidthEnabled();
	
	/**
	 * Set a path to file where the call will be recorded.
	 * Actual start of the recording is controlled by LinphoneCall.startRecording().
	 * @param path Path to the file where the call will be recorded. If it is a WAV
	 * file, only audio will be written whereas if it is a MKV file, audio and video
	 * will be written. 
	**/
	void setRecordFile(String path);
	
	/**
	 * Add a custom header to be used for the call for which these call params are used.
	 * @param name header name
	 * @param value header value
	 */
	void addCustomHeader(String name, String value);
	
	/**
	 * Returns the value of a custom header given its name.
	 * If no header with that name exists, then null is returned.
	 * @param name
	 * @return value for the header, or null if it doesn't exist.
	 */
	String getCustomHeader(String name);
	
	/**
	 * Set the privacy for the call.
	 * @param privacy_mask a or'd int of values defined in interface {@link org.linphone.core.Privacy}
	 */
	void setPrivacy(int privacy_mask);
	
	/**
	 * Get the privacy mask requested for this call.
	 * @return the privacy mask as defined in interface {@link org.linphone.core.Privacy}
	 */
	int getPrivacy();
	
	/**
	 * Set the session name of the media session (ie in SDP). Subject from the SIP message can be retrieved using linphone_call_params_get_custom_header().
	 * @param name the session name
	**/
	void setSessionName(String name);
	/**
	 * Get the session name of the media session (ie in SDP). Subject from the SIP message can be retrieved using linphone_call_params_get_custom_header().
	 * @return the session name
	**/
	String getSessionName();
	
	/**
	 * Gets the size of the video that is sent.
	 * @return The sent video size.
	 */	
	VideoSize getSentVideoSize();
	
	/**
	 * Gets the size of the video that is received.
	 * @return The received video size.
	 */
	VideoSize getReceivedVideoSize();
}
