package org.linphone.core;

import java.util.Vector;


public interface LinphoneChatMessage {
	@Deprecated
	interface StateListener {
		void onLinphoneChatMessageStateChanged(LinphoneChatMessage msg, State state);
	}
	
	interface LinphoneChatMessageListener {
		void onLinphoneChatMessageStateChanged(LinphoneChatMessage msg, State state);
		
		/**
		 * This function is called by the core upon an incoming File transfer is started. This function may be call several time for the same file in case of large file.
		 * @param content incoming content information
		 * @param buffer holding the received data. Empty buffer means end of file.
		 */
		void onLinphoneChatMessageFileTransferReceived(LinphoneChatMessage msg, LinphoneContent content, LinphoneBuffer buffer);
		
		/**
		 * This function is called by the core when an outgoing file transfer is started. This function is called until size is set to 0.
		 * @param content incoming content information
		 * @param offset the offset in the file from where to get the data to be sent
		 * @param size the number of bytes expected by the framework
		 * @param bufferToFill A LinphoneBuffer object holding the data written by the application. An empty buffer means end of file.
		 */
		void onLinphoneChatMessageFileTransferSent(LinphoneChatMessage msg, LinphoneContent content, int offset, int size, LinphoneBuffer bufferToFill);
		
		/**
		 * File transfer progress indication callback prototype.
		 * @param content incoming content information
		 * @param offset The number of bytes sent/received since the beginning of the transfer.
		 * @param total The total number of bytes to be sent/received.
		 */
		void onLinphoneChatMessageFileTransferProgressChanged(LinphoneChatMessage msg, LinphoneContent content, int offset, int total);
	}
	public static class State {
		static private Vector<State> values = new Vector<State>();
		private final int mValue;
		public final int value() {return mValue;}
		
		private final String mStringValue;
		/**
		 * Initial state
		 */
		public final static State Idle = new State(0,"Idle");
		/**
		 * Delivery in progress
		 */
		public final static State InProgress = new State(1,"InProgress");
		/**
		 * Message succesffully delivered an acknoleged by remote end point
		 */
		public final static State Delivered = new State(2,"Delivered");
		/**
		 * Message was not delivered
		 */
		public final static State NotDelivered = new State(3,"NotDelivered");
		/**
		 * Message was received(and acknowledged) but cannot get file from server
		 */
		public final static State FileTransferError = new State(4,"FileTransferError");
		/**
		 * File transfer has been completed successfully.
		 */
		public final static State FileTransferDone = new State(5,"FileTransferDone");
		
		private State(int value,String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue=stringValue;
		}
		
		public static State fromInt(int value) {

			for (int i=0; i<values.size();i++) {
				State state = (State) values.elementAt(i);
				if (state.mValue == value) return state;
			}
			throw new RuntimeException("state not found ["+value+"]");
		}
		public String toString() {
			return mStringValue;
		}
		public int toInt() {
			return mValue;
		}
	}
	
	
	/**
	 * get text associated to this LinphoneChatMessage
	 * 
	 * @return text sent along with the message
	 */
	String getText();
	
	/**
	 * get peer address associated to this LinphoneChatMessage
	 *
	 * @return LinphoneAddress peer address
	 */
	LinphoneAddress getPeerAddress();
	
	/**
	 * get from address associated to this LinphoneChatMessage
	 *
	 * @return LinphoneAddress from address
	 */
	LinphoneAddress getFrom();
	
	/**
	 * Get destination address of the LinphoneChatMessage.
	 * @return the LinphoneAddress in the To field of the message.
	 */
	LinphoneAddress getTo();
	
	/**
	 * Linphone message can carry external body as defined by rfc2017
	 * @param message #LinphoneChatMessage
	 * @return return external body url null if not present.
	 */
	String getExternalBodyUrl();
	
	/**
	 * Linphone message can carry external body as defined by rfc2017
	 * @param  #LinphoneChatMessage  
	 * @param url ex: access-type=URL; URL="http://www.foo.com/file"
	 */
	void setExternalBodyUrl(String url);
	
	/**
	 * Add a custom header into the message.
	 * @param name
	 * @param value
	 */
	void addCustomHeader(String name, String value);
	
	/**
	 * Obtain a header value.
	 * @param name
	 * @return the value of the header, or null if not found.
	 */
	String  getCustomHeader(String name);
	
	/**
	 * Gets the time at which the message was sent
	 * @return the time in milliseconds
	 */
	long getTime();
	
	/**
	 * Gets the status of the message
	 * @return the status of the message
	 */
	LinphoneChatMessage.State getStatus();
	
	/**
	 * Returns wether or not the message has been read
	 * @return true if it has been read, flase otherwise
	 */
	boolean isRead();
	
	/**
	 * Returns wether the message has been sent or received
	 * @return true if the message has been sent, false if it has been received
	 */
	boolean isOutgoing();
	
	/**
	 * THIS METHOD IS ONLY USED TO IMPORT OLD MESSAGES, DON'T USE IT FOR ANY OTHER USAGE!
	 */
	void store();
	
	/**
	 * Returns the id used to id this message in the database
	 * @return the id used to id this message in the database
	 */
	int getStorageId();

	/**
     * @return the reason if response received
     */
	Reason getReason();
	
	/**
	 * Returns full error in case of failure when sending message.
	 * @return an ErrorInfo.
	 */
	ErrorInfo getErrorInfo();
	
	/**
	 * 	Cancel an ongoing file transfer attached to this message.(upload or download).
	 */
	void cancelFileTransfer();
	/**
	 * Get the file_transfer_information (used by call backs to recover informations during a rcs file transfer)
	 * @return a pointer to the LinphoneContent structure or NULL if not present.
	 */
	LinphoneContent getFileTransferInformation();
	
	/**
	 * Sets data in the chat message
	 * @param data to store in the message
	 */
	void setAppData(String data);
	
	/**
	 * @return the data stored in the chat message if any, else null
	 */
	String getAppData();

	/**
	 * Set the path to the file to read from or write to during the file transfer.
	 * @param path The path to the file to use for the file transfer.
	 */
	void setFileTransferFilepath(String path);
	
	/**
	 * Start the download of the file referenced in a LinphoneChatMessage from remote server.
	 */
	int downloadFile();
	
	/**
	 * Set the callbacks associated with the LinphoneChatMessage.
	 */
	void setListener(LinphoneChatMessage.LinphoneChatMessageListener listener);
	/**
	 * Fulfill a chat message char by char. Message linked to a Real Time Text Call send char in realtime following RFC 4103/T.140
	 * To commit a message, use #linphone_chat_room_send_message
	 * @param[in] character T.140 char
	 * @throw LinphoneCoreExeption .
	 */
	void putChar(long character) throws LinphoneCoreException;
	
}
