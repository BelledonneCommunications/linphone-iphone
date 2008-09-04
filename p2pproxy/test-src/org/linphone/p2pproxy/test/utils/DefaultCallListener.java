/*
p2pproxy
Copyright (C) 2007  Jehan Monnier ()

DefaultCallListener.java - .

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
package org.linphone.p2pproxy.test.utils;

import java.util.Vector;

import junit.framework.Assert;

import org.zoolu.sip.address.NameAddress;
import org.zoolu.sip.call.Call;
import org.zoolu.sip.call.CallListener;
import org.zoolu.sip.message.Message;

public class   DefaultCallListener implements CallListener {
    public void onCallAccepted(Call call, String sdp, Message resp) {
        Assert.fail("call accepted");
    }
    public void onCallCanceling(Call call, Message cancel) {
        Assert.fail("call canceled");
    }
    public void onCallClosed(Call call, Message resp) {
        Assert.fail("call closed");         }

    public void onCallClosing(Call call, Message bye) {
        Assert.fail("call closing");
    }
    public void onCallConfirmed(Call call, String sdp, Message ack) {
        Assert.fail("call Confirmed");
    }
    public void onCallIncoming(Call call, NameAddress callee, NameAddress caller, String sdp, Message invite) {
        Assert.fail("call Incoming");
    }
    public void onCallModifying(Call call, String sdp, Message invite) {
        Assert.fail("call Modifying");
    }
    public void onCallReInviteAccepted(Call call, String sdp, Message resp) {
        Assert.fail("call ReInviteAccepted");
    }
    public void onCallReInviteRefused(Call call, String reason, Message resp) {
        Assert.fail("call ReInviteRefused");
    }
    public void onCallReInviteTimeout(Call call) {
        Assert.fail("call ReInviteTimeout");
    }
    public void onCallRedirection(Call call, String reason, Vector contact_list, Message resp) {
        Assert.fail("call Redirection");
    }
    public void onCallRefused(Call call, String reason, Message resp) {
        Assert.fail("call Refused reason ["+resp.getStatusLine()+"]");
    }
    public void onCallRinging(Call call, Message resp) {
        Assert.fail("call Ringing");
    }
    public void onCallTimeout(Call call) {
        Assert.fail("call Timeout");
    }
}
