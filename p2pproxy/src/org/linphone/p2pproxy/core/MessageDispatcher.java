/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

MessageDispatcher.java 

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
package org.linphone.p2pproxy.core;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.apache.log4j.Logger;

import net.jxta.pipe.PipeMsgEvent;
import net.jxta.pipe.PipeMsgListener;

/**
 * 
 * @author jehan
 *
 */
public class MessageDispatcher implements PipeMsgListener {
	private final static Logger mLog = Logger.getLogger(MessageDispatcher.class);

	private List<PipeMsgListener> mPipeMsgListenerList = Collections.synchronizedList(new ArrayList<PipeMsgListener>());
	
	public void pipeMsgEvent(PipeMsgEvent event) {
		synchronized (this) {
			mLog.debug("receiving event with message ["+event.getMessage()+"] propagating to ["+mPipeMsgListenerList.size()+"]");
			for (PipeMsgListener lPipeMsgListener:mPipeMsgListenerList) {
				lPipeMsgListener.pipeMsgEvent(event);
			}
		}
	}
	synchronized void addPipeMsgListener(PipeMsgListener aPipeMsgListener) {
		mPipeMsgListenerList.add(aPipeMsgListener);
	}

}
