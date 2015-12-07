/*
LinPhoneCallStats.java
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

import java.util.Vector;


public interface LinphoneCallStats {
	static public class MediaType {
		static private Vector<MediaType> values = new Vector<MediaType>();
		/**
		 * Audio
		 */
		static public MediaType Audio = new MediaType(0, "Audio");
		/**
		 * Video
		 */
		static public MediaType Video = new MediaType(1, "Video");
		/**
		 * Text
		*/
		static public MediaType Text = new MediaType(2, "Text");
		protected final int mValue;
		private final String mStringValue;

		private MediaType(int value, String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue = stringValue;
		}
		public static MediaType fromInt(int value) {
			for (int i = 0; i < values.size(); i++) {
				MediaType mtype = (MediaType) values.elementAt(i);
				if (mtype.mValue == value) return mtype;
			}
			throw new RuntimeException("MediaType not found [" + value + "]");
		}
		public String toString() {
			return mStringValue;
		}
	}
	static public class IceState {
		static private Vector<IceState> values = new Vector<IceState>();
		/**
		 * Not activated
		 */
		static public IceState NotActivated = new IceState(0, "Not activated");
		/**
		 * Failed
		 */
		static public IceState Failed = new IceState(1, "Failed");
		/**
		 * In progress
		 */
		static public IceState InProgress = new IceState(2, "In progress");
		/**
		 * Host connection
		 */
		static public IceState HostConnection = new IceState(3, "Host connection");
		/**
		 * Reflexive connection
		 */
		static public IceState ReflexiveConnection = new IceState(4, "Reflexive connection");
		/**
		 * Relay connection
		 */
		static public IceState RelayConnection = new IceState(5, "Relay connection");
		protected final int mValue;
		private final String mStringValue;

		private IceState(int value, String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue = stringValue;
		}
		public static IceState fromInt(int value) {
			for (int i = 0; i < values.size(); i++) {
				IceState mstate = (IceState) values.elementAt(i);
				if (mstate.mValue == value) return mstate;
			}
			throw new RuntimeException("IceState not found [" + value + "]");
		}
		public String toString() {
			return mStringValue;
		}
	}

	/**
	 * Get the stats media type
	 * @return MediaType
	 */
	public MediaType getMediaType();

	/**
	 * Get the ICE state
	 */
	public IceState getIceState();

	/**
	 * Get the download bandwidth in kbit/s
	 * @return The download bandwidth
	 */
	public float getDownloadBandwidth();

	/**
	 * Get the upload bandwidth in kbit/s
	 * @return The upload bandwidth
	 */
	public float getUploadBandwidth();

	/**
	 * Get the local loss rate since last report
	 * @return The sender loss rate
	 */
	public float getSenderLossRate();

	/**
	 * Get the remote reported loss rate since last report
	 * @return The receiver loss rate
	 */
	public float getReceiverLossRate();

	/**
	 * Get the local interarrival jitter
	 * @return The interarrival jitter at last emitted sender report
	 */
	public float getSenderInterarrivalJitter();

	/**
	 * Get the remote reported interarrival jitter
	 * @return The interarrival jitter at last received receiver report
	 */
	public float getReceiverInterarrivalJitter();

	/**
	 * Get the round trip delay
	 * @return The round trip delay in seconds, -1 if the information is not available
	 */
	public float getRoundTripDelay();

	/**
	 * Get the cumulative number of late packets
	 * @return The cumulative number of late packets
	 */
	public long getLatePacketsCumulativeNumber();

	/**
	 * Get the jitter buffer size
	 * @return The jitter buffer size in milliseconds
	 */
	public float getJitterBufferSize();

	/**
	 * Get the local loss rate. Unlike getSenderLossRate() that returns this loss rate "since last emitted RTCP report", the value returned here is updated every second.
	 * @return The local loss rate percentage.
	**/
	public float getLocalLossRate();

	/**
	 * Get the local late packets rate. The value returned here is updated every second.
	 * @return The local late rate percentage.
	**/
	public float getLocalLateRate();
}
