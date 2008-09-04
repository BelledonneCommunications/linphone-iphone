/*
p2pproxy
Copyright (C) 2007  Jehan Monnier ()

SipClient.java - .

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

import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import org.apache.log4j.Logger;
import org.zoolu.sip.address.NameAddress;
import org.zoolu.sip.call.Call;
import org.zoolu.sip.call.CallListener;
import org.zoolu.sip.header.ExpiresHeader;
import org.zoolu.sip.message.Message;
import org.zoolu.sip.message.MessageFactory;
import org.zoolu.sip.provider.SipProvider;
import org.zoolu.sip.transaction.TransactionClient;
import org.zoolu.sip.transaction.TransactionClientListener;

import junit.framework.Assert;


public class SipClient {
   private final static Logger mLog = Logger.getLogger(SipClient.class);
   private  final SipProvider mProvider;
   private final String mSipIdentity;
   private final long mTransactionTimeout;
   public final static String sdp_offer = "v=0\n"
                                          +"o=jdoe 2890844526 2890842807 IN IP4 10.0.1.1\n"
                                          +"s=\n"
                                          +"c=IN IP4 192.0.2.3\n"
                                          +"t=0 0\n"
                                          +"a=ice-pwd:asd88fgpdd777uzjYhagZg\n"
                                          +"a=ice-ufrag:8hhY\n"
                                          +"m=audio 45664 RTP/AVP 0\n"
                                          +"a=rtpmap:0 PCMU/8000\n"
                                          +"a=candidate:1 1 UDP 2130706431 127.0.0.1 8998 typ host";
   
   
   
   public SipClient(SipProvider aProvider,String aSipIdentity,long aTransactionTimeout) {
      mProvider=aProvider;
      mSipIdentity = aSipIdentity;
      mTransactionTimeout = aTransactionTimeout;
   }
   public void register() throws InterruptedException {
        register(900,mSipIdentity);
    }
   
   public void unRegister() throws InterruptedException {
        register(0,mSipIdentity);
    }
   public void register(int expiration,String aTo) throws InterruptedException {
      register(expiration,aTo, 200);
   }
   public boolean register(int expiration,String aTo,final int aReturnCode) throws InterruptedException {
            final long TryingTimout = 1000;
            
            NameAddress lContact = new NameAddress(getContact(mProvider));
            NameAddress lT0 = new NameAddress(aTo);
            Message req=MessageFactory.createRegisterRequest(mProvider,lT0,lT0,lContact);
            req.setExpiresHeader(new ExpiresHeader(expiration));
            final Semaphore l100Semaphore = new Semaphore(0);
            final Semaphore l200Semaphore = new Semaphore(0);
            final boolean[] lstatus = {false};
            TransactionClientListener lTransactionClient = new TransactionClientListener() {

                public void onTransFailureResponse(TransactionClient tc, Message resp) {
                    if (aReturnCode < 400) {
                       Assert.fail("transaction failure ["+resp.getStatusLine().toString()+"]");
                    } else {
                       Assert.assertEquals("unexpected return code" ,aReturnCode, resp.getStatusLine().getCode());
                       l100Semaphore.release();
                       l200Semaphore.release();
                    }
                }
                public void onTransProvisionalResponse(TransactionClient tc, Message resp) {
                    int lStatusCode = resp.getStatusLine().getCode();
                    Assert.assertEquals("bad status code" ,100, lStatusCode);
                    l100Semaphore.release();
                }
                public void onTransSuccessResponse(TransactionClient tc, Message resp) {
                   lstatus[0] = true; 
                   l200Semaphore.release();
                }
                public void onTransTimeout(TransactionClient tc) {
                    Assert.fail("transaction timeout");
                }

            };
            TransactionClient t=new TransactionClient(mProvider,req,lTransactionClient);
            t.request();
            Assert.assertTrue("100 trying received too late["+TryingTimout+"]", l100Semaphore.tryAcquire(TryingTimout,TimeUnit.MILLISECONDS));
            l100Semaphore.release();
            Assert.assertTrue("200 received too late["+mTransactionTimeout+"]", l200Semaphore.tryAcquire(mTransactionTimeout,TimeUnit.MILLISECONDS));
            l200Semaphore.release();
            return lstatus[0];
    }
   public String getContact(SipProvider aProvider) {
        return "sip:"+aProvider.getViaAddress()+":"+aProvider.getPort();
    }
   
   /**
    * @param aTo uri to call
    * @param aProvider sip provider of the To party
    */
   public void call(String aTo,SipProvider aCalleeProvider) {
      call(aTo,aCalleeProvider,false);
   }
   /**
    * @param aTo uri to call
    * @param aProvider sip provider of the To party
    * @param should I put an SDP ?
    */
   public void call(String aTo,SipProvider aCalleeProvider,boolean enableSdp) {

        try {
            String lCallerUri = mSipIdentity;
            String lCalleeUri = aTo;
            mLog.info("Calling  ["+aTo+"] from ["+mSipIdentity+"]");
            long lTimout = 75000;

            final Semaphore lCallerSemaphoreAccepted = new Semaphore(0);
            final Semaphore lCalleeSemaphoreClosed = new Semaphore(0);
            final Semaphore lCallerSemaphoreRinging = new Semaphore(0);
            CallListener lCallerListener = new DefaultCallListener() {
                public void onCallAccepted(Call call, String sdp, Message resp) {
                    lCallerSemaphoreAccepted.release();
                  call.ackWithAnswer(sdp);
              }
              public void onCallClosing(Call call, Message bye) {
                  //nop
              }
                public void onCallRinging(Call call, Message resp) {
                    lCallerSemaphoreRinging.release();
                }
            };
            Call  lCaller = new Call(mProvider, lCallerUri, getContact(mProvider), lCallerListener);
            if (enableSdp) {
               lCaller.setLocalSessionDescriptor(sdp_offer);
            }
            final Semaphore lCalleeSemaphoreConfirmed = new Semaphore(0);
            final Semaphore lCalleeSemaphoreIncoming = new Semaphore(0);
            CallListener lCalleeListener = new DefaultCallListener() {
                public void onCallConfirmed(Call call, String sdp, Message ack) {
                    lCalleeSemaphoreConfirmed.release();
                    call.bye();
                }
                public void onCallIncoming(Call call, NameAddress callee, NameAddress caller, String sdp, Message invite) {
                    lCalleeSemaphoreIncoming.release();
                    call.accept(sdp);
                }
              public void onCallClosed(Call call, Message resp) {
                  lCalleeSemaphoreClosed.release();
              }
            };
            Call  lCallee = new Call(aCalleeProvider, lCalleeUri, getContact(aCalleeProvider), lCalleeListener);
            if (enableSdp) {
               lCallee.setLocalSessionDescriptor(sdp_offer);
            }

            lCallee.listen();
            lCaller.call(lCalleeUri);

            Assert.assertTrue("callee  not alerted until ["+lTimout+"]", lCalleeSemaphoreIncoming.tryAcquire(lTimout,TimeUnit.MILLISECONDS));
            Assert.assertTrue("caller  call not accepted until ["+lTimout+"]", lCallerSemaphoreAccepted.tryAcquire(lTimout,TimeUnit.MILLISECONDS));
            Assert.assertTrue("callee  call not confirmed until ["+lTimout+"]", lCalleeSemaphoreConfirmed.tryAcquire(lTimout,TimeUnit.MILLISECONDS));
            Assert.assertTrue("caller  call not closed until ["+lTimout+"]", lCalleeSemaphoreClosed.tryAcquire(lTimout,TimeUnit.MILLISECONDS));
            
 
            mLog.info("Call ok");
        } catch (Exception e) {
            mLog.error("Call ko",e);
            Assert.fail(e.getMessage());
        }       

          
   }
}
