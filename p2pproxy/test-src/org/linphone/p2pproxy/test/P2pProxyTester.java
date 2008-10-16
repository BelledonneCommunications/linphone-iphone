/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

P2pProxyTester.java - junit test for p2pproxy

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
package org.linphone.p2pproxy.test;

import java.net.DatagramSocket;
import java.util.Enumeration;
import java.util.List;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;



import net.jxta.discovery.DiscoveryEvent;
import net.jxta.discovery.DiscoveryListener;
import net.jxta.discovery.DiscoveryService;
import net.jxta.document.Advertisement;
import net.jxta.protocol.DiscoveryResponseMsg;

import org.apache.log4j.Logger;
import org.apache.log4j.PropertyConfigurator;
import org.linphone.p2pproxy.api.P2pProxyInstance;
import org.linphone.p2pproxy.api.P2pProxyInstance.Mode;
import org.linphone.p2pproxy.core.JxtaNetworkManager;
import org.linphone.p2pproxy.core.P2pProxyAccountManagement;
import org.linphone.p2pproxy.core.P2pProxyAccountManagementMBean;
import org.linphone.p2pproxy.core.P2pProxyInstanceImpl;
import org.linphone.p2pproxy.core.P2pProxyMain;
import org.linphone.p2pproxy.core.sipproxy.NetworkResourceAdvertisement;
import org.linphone.p2pproxy.core.sipproxy.SipProxyRegistrar;
import org.linphone.p2pproxy.core.stun.AddressInfo;
import org.linphone.p2pproxy.core.stun.StunClient;
import org.linphone.p2pproxy.test.utils.DefaultCallListener;
import org.linphone.p2pproxy.test.utils.SipClient;
import org.zoolu.sip.address.NameAddress;
import org.zoolu.sip.call.Call;
import org.zoolu.sip.call.CallListener;
import org.zoolu.sip.message.Message;
import org.zoolu.sip.provider.SipProvider;

import junit.framework.Assert;
import junit.framework.TestCase;

public class P2pProxyTester extends TestCase {


   private final static Logger mLog = Logger.getLogger(P2pProxyTester.class);

	static private  SipProvider mProvider;
    static final String mDefaultSipIdentity = "sip:p2pTester@linphone.org";
    static private  SipClient mSipClient;
    static private P2pProxyInstance mP2pProxyInstance;
    final private String mCallerUri = "sip:caller@linphone.org";
    final private String mCalleeUri = "sip:callee@linphone.org";
    final private int RDV_DISCOVERY_TIMEOUT = 5000;
    static private P2pProxyAccountManagementMBean mP2pProxyAccountManagement;

    public P2pProxyTester() {
       
       }   
	protected void setUp() throws Exception {
        
		if (mP2pProxyInstance == null) {
	          // setup logging
	       System.setProperty("org.linphone.p2pproxy.home", ".");
	       P2pProxyMain.staticLoadTraceConfigFile(); 
	       //PropertyConfigurator.configure("log4j.properties");
           setupJxta(); 
           mProvider = mP2pProxyInstance.getSipClientProvider() ;
           mSipClient = new SipClient(mProvider, mDefaultSipIdentity,RDV_DISCOVERY_TIMEOUT*2);
           // create account for user mDefaultSipIdentity, mCallerUri, mCalleeUri if not exist
           mP2pProxyAccountManagement = new P2pProxyAccountManagement((JxtaNetworkManager)mP2pProxyInstance.getOpaqueNetworkManager());

           try {
              mP2pProxyAccountManagement.createAccount(mDefaultSipIdentity);
            } catch (Exception e) {
              mLog.warn(e);
           }           
            try {
               mP2pProxyAccountManagement.createAccount(mCallerUri);
             } catch (Exception e) {
               mLog.warn(e);
            }

             try {
                mP2pProxyAccountManagement.createAccount(mCalleeUri);
              } catch (Exception e) {
                mLog.warn(e);
             }
		}
		
	}

	protected void tearDown() throws Exception {
	}

	public void testStunClient() {
	   try {
//	      if (mP2pProxyInstance2 == null) {
//	         try {
//	            setupJxta2();
//	         } catch (Exception e) {
//	            mLog.error("cannot start peer2");
//	         }
//	      }
	      DatagramSocket lDatagramSocket = new DatagramSocket();
	      StunClient lStunClient = new StunClient((JxtaNetworkManager)mP2pProxyInstance.getOpaqueNetworkManager());
	      AddressInfo lAddressInfo = lStunClient.computeAddressInfo(lDatagramSocket);
	      mLog.info("AddressInfo ["+lAddressInfo+"]");
	   }catch (Exception e) {
	      mLog.error("testStunClient ko",e);
	      Assert.fail(e.getMessage());
	   }

	}
	public void testGetRegistrarAdress() {
	   try {
	      NetworkResourceAdvertisement lSipProxyRegistrarAdvertisement = (NetworkResourceAdvertisement) (((JxtaNetworkManager)mP2pProxyInstance.getOpaqueNetworkManager()).getAdvertisement(null,SipProxyRegistrar.ADV_NAME, true));
	      mLog.info("testGetRegistrarAdress ok ["+lSipProxyRegistrarAdvertisement.getAddress()+"]");
	   } catch (Exception e) {
	      mLog.error("testGetRegistrarAdress ko",e);
	      Assert.fail(e.getMessage());
	   }

	}
	public void testSipRegisterUnregister() {
		try {
		 	//register
           mSipClient.register();
			//unregister
           mSipClient.unRegister();
			mLog.info("testSipRegisterUnregister ok");
		} catch (Exception e) {
			mLog.error("testSipRegisterUnregister ko",e);
			Assert.fail(e.getMessage());
		}
		
	}
    public void testRegisterUnknownUser() {
        try {
            //register
          Assert.assertFalse(mSipClient.register(900,"sip:toto@linphone.org",404));
           mLog.info("testRegisterUnknownUser ok");
        } catch (Exception e) {
            mLog.error("testRegisterUnknownUser ko",e);
            Assert.fail(e.getMessage());
        }
        
    }    
//	public void testPipeDiscovery() {
//		try {
//           long lDiscoveryTimout = 10000;
//			//register
//           mSipClient.register(900,mDefaultSipIdentity);
//			DiscoveryService lDiscoveryService = ((JxtaNetworkManager)mP2pProxyInstance.getOpaqueNetworkManager()).getPeerGroup().getDiscoveryService();
//			final Semaphore lSemaphore = new Semaphore(0);
//			DiscoveryListener lDiscoveryListener = new DiscoveryListener() {
//
//				public void discoveryEvent(DiscoveryEvent event) {
//					DiscoveryResponseMsg LRes = event.getResponse();
//					Enumeration lAdvertisementLists = LRes.getAdvertisements();
//					Advertisement lAdvertisement = (Advertisement) lAdvertisementLists.nextElement();
//					//Assert.assertEquals("not the good adv name", lContact, lAdvertisement.)
//					mLog.info(lAdvertisement.toString());
//					lSemaphore.release();
//				}
//				
//			};
//			lDiscoveryService.getRemoteAdvertisements(null, DiscoveryService.ADV, "Name",mDefaultSipIdentity, 1,lDiscoveryListener);
//			Assert.assertTrue("pipe not found until ["+lDiscoveryTimout+"]", lSemaphore.tryAcquire(lDiscoveryTimout,TimeUnit.MILLISECONDS));
//			lSemaphore.release();
//			
//			//unregister
//            mSipClient.register(0,mDefaultSipIdentity);
//			mLog.info("testPipeDiscovery ok");
//		} catch (Exception e) {
//			mLog.error("testPipeDiscovery ko",e);
//			Assert.fail(e.getMessage());
//		}
//		
//	}
    public void testCall() {
       try {
          Call(false);
       } catch (Exception e) {
          mLog.error("testCall ko",e);
          Assert.fail(e.getMessage());
      }       
       
    }
    public void testCallWithSdp() {
       try {
          Call(true);
       } catch (Exception e) {
          mLog.error("testCallWithSdp ko",e);
          Assert.fail(e.getMessage());
      }       
       
    }

    private void Call(boolean useSdp) throws Exception {
			//register
            mSipClient.register(900,mCallerUri);
            mSipClient.register(900,mCalleeUri);
            call(mCallerUri,mCalleeUri,useSdp);
			// unregister
            mSipClient.register(0,mCallerUri);
            mSipClient.register(0,mCalleeUri);

			mLog.info("testCall ok");

	}
    public void testUserNotFound() {
        try {
            long lTimout = (long) (RDV_DISCOVERY_TIMEOUT * 2);
            //register
            mSipClient.register(900,mCallerUri);
            final Semaphore lCallerSemaphoreRefused = new Semaphore(0);
            CallListener lCallerListener = new DefaultCallListener() {
            public void onCallRefused(Call call, String reason, Message resp) {
               lCallerSemaphoreRefused.release();
               Assert.assertEquals("bad reason, must be user not found", 404, resp.getStatusLine().getCode());
            }
            };
            Call  lCaller = new Call(mProvider, mCallerUri, mSipClient.getContact(mProvider), lCallerListener);
            lCaller.call(mCalleeUri);
            Assert.assertTrue("caller  call not refused until ["+lTimout+"]", lCallerSemaphoreRefused.tryAcquire(lTimout,TimeUnit.MILLISECONDS));
            // unregister
            mSipClient.register(0,mCallerUri);

            mLog.info("testUserNotFound ok");
        } catch (Exception e) {
            mLog.error("testUserNotFound ko",e);
            Assert.fail(e.getMessage());
        }       
    }
    public void testUserUnregisterred() {
       try {
           long lTimout = RDV_DISCOVERY_TIMEOUT * 2;
           //register
           mSipClient.register(900,mCallerUri);
           mSipClient.register(900,mCalleeUri);
           //unregister callee
           mSipClient.register(0,mCalleeUri);
           final Semaphore lCallerSemaphoreRefused = new Semaphore(0);
           CallListener lCallerListener = new DefaultCallListener() {
           public void onCallRefused(Call call, String reason, Message resp) {
              lCallerSemaphoreRefused.release();
              Assert.assertEquals("bad reason, must be user not found", 404, resp.getStatusLine().getCode());
           }
           };
           Call  lCaller = new Call(mProvider, mCallerUri, mSipClient.getContact(mProvider), lCallerListener);
           lCaller.call(mCalleeUri);
           Assert.assertTrue("caller  call not refused until ["+lTimout+"]", lCallerSemaphoreRefused.tryAcquire(lTimout,TimeUnit.MILLISECONDS));
           // unregister caller
           mSipClient.register(0,mCallerUri);

           mLog.info("testUserNotFound ok");
       } catch (Exception e) {
           mLog.error("testUserNotFound ko",e);
           Assert.fail(e.getMessage());
       }       
   }
    public void xxxCallCancelledBeforeDialogEstablishement() {
       try {
          //Assert.fail("not debugged yet");
           long lTimout = RDV_DISCOVERY_TIMEOUT * 2;
          //register
          mSipClient.register(900,mCallerUri);
          //mSipClient.register(900,mCalleeUri);
          final Semaphore lCallerSemaphoreCancel = new Semaphore(0);
          CallListener lCallerListener = new DefaultCallListener() {
             public void onCallRefused(Call call, String reason, Message resp) {
                Assert.assertEquals("bad reason, must be  Request Terminated", 487, resp.getStatusLine().getCode());
                lCallerSemaphoreCancel.release();         
                }                

          };
          Call  lCaller = new Call(mProvider, mCallerUri, mSipClient.getContact(mProvider), lCallerListener);
          lCaller.call(mCalleeUri);
          Thread.sleep(1000);
          lCaller.cancel();
          Assert.assertTrue("caller  call not canceled until ["+lTimout+"]", lCallerSemaphoreCancel.tryAcquire(lTimout,TimeUnit.MILLISECONDS));
          // unregister
          mSipClient.register(0,mCallerUri);
          
          mLog.info("testCallCancelledBeforeDialogEstablishement ok");
       } catch (Exception e) {
          mLog.error("testCallCancelledBeforeDialogEstablishement ko",e);
          Assert.fail(e.getMessage());
       }       
    }
    
    public void testCallCancelledAfterRinging() {
        try {
            long lTimout = RDV_DISCOVERY_TIMEOUT * 2;
            //register
           mSipClient.register(900,mCallerUri);
           mSipClient.register(900,mCalleeUri);
            final Semaphore lCalleeSemaphoreCanceling = new Semaphore(0);
            final Semaphore lCallerSemaphoreRinging = new Semaphore(0);
            final Semaphore lCallerSemaphoreCanceled = new Semaphore(0);
            CallListener lCallerListener = new DefaultCallListener() {

                public void onCallRinging(Call call, Message resp) {
                    lCallerSemaphoreRinging.release();
                }
                public void onCallRefused(Call call, String reason, Message resp) {
                   Assert.assertEquals("bad reason, must be  Request Terminated", 487, resp.getStatusLine().getCode());
                   lCallerSemaphoreCanceled.release();         
                   }                
 
            };
            Call  lCaller = new Call(mProvider, mCallerUri, mSipClient.getContact(mProvider), lCallerListener);
            final Semaphore lCalleeSemaphoreIncoming = new Semaphore(0);
            CallListener lCalleeListener = new DefaultCallListener() {
               public void onCallIncoming(Call call, NameAddress callee, NameAddress caller, String sdp, Message invite) {
                    lCalleeSemaphoreIncoming.release();
                    call.ring();
                }
                public void onCallCanceling(Call call, Message cancel) {
                   lCalleeSemaphoreCanceling.release();
               }

            };
            Call  lCallee = new Call(mProvider, mCalleeUri, mSipClient.getContact(mProvider), lCalleeListener);
            lCallee.listen();
            lCaller.call(mCalleeUri);

            Assert.assertTrue("callee  not alerted until ["+lTimout+"]", lCalleeSemaphoreIncoming.tryAcquire(lTimout,TimeUnit.MILLISECONDS));
            Assert.assertTrue("callee  call not ringing until ["+lTimout+"]", lCallerSemaphoreRinging.tryAcquire(lTimout,TimeUnit.MILLISECONDS));
            lCaller.cancel();
            Assert.assertTrue("caller  call not canceling until ["+lTimout+"]", lCalleeSemaphoreCanceling.tryAcquire(lTimout,TimeUnit.MILLISECONDS));
            Assert.assertTrue("caller  call not refused until ["+lTimout+"]", lCallerSemaphoreCanceled.tryAcquire(lTimout,TimeUnit.MILLISECONDS));
            
            // unregister
           mSipClient.register(0,mCallerUri);
           mSipClient.register(0,mCalleeUri);

            mLog.info("testCallCancelledAfterRinging ok");
        } catch (Exception e) {
            mLog.error("testCallCancelledAfterRinging ko",e);
            Assert.fail(e.getMessage());
        }       

   }
    public void testAlreadyRegister() {
    	Assert.fail("not implemented yet");
    }
    public void testReRegisterAfterExpired() {
        try {
             //register
           mSipClient.register(10,mCallerUri);
           mSipClient.register(10,mCalleeUri);
           call(mCallerUri,mCalleeUri);
           // wait
           Thread.sleep(10000);
           mSipClient.register(20,mCallerUri);
           mSipClient.register(20,mCalleeUri);
           
           call(mCallerUri,mCalleeUri);

           // unregister
           mSipClient.register(0,mCallerUri);
           mSipClient.register(0,mCalleeUri);

            mLog.info("testReRegisterAfterExpired ok");
        } catch (Exception e) {
            mLog.error("testtestReRegisterAfterExpired ko",e);
            Assert.fail(e.getMessage());
        }       

    
    }
    public void testReRegisterBeforeExpired() {
       try {
           //register
          mSipClient.register(900,mCallerUri);
          mSipClient.register(900,mCalleeUri);
          
          call(mCallerUri,mCalleeUri);
          //re-register
          mSipClient.register(900,mCallerUri);
          mSipClient.register(900,mCalleeUri);
          
          call(mCallerUri,mCalleeUri);

          // unregister
          mSipClient.register(0,mCallerUri);
          mSipClient.register(0,mCalleeUri);

           mLog.info("testReRegisterBeforeExpired ok");
       } catch (Exception e) {
           mLog.error("testReRegisterBeforeExpired ko",e);
           Assert.fail(e.getMessage());
       }       

   
   }

	private void setupJxta() throws Exception {
       mP2pProxyInstance = new P2pProxyInstanceImpl();
       mP2pProxyInstance.setMode(Mode.seeding_server);
       mP2pProxyInstance.setIndex(1);
       mP2pProxyInstance.setProperty(JxtaNetworkManager.ADV_DISCOVERY_TIMEOUT, String.valueOf(RDV_DISCOVERY_TIMEOUT));
       mP2pProxyInstance.start();
       while (mP2pProxyInstance.isStarted() == false) Thread.sleep(500);
	}

	private void call(String aCaller,String aCallee) throws Exception {
	   call(aCaller,aCallee,false);
	}
	
	private void call(String aCaller,String aCallee,boolean useSdp) throws Exception {
	   long lTimout = 1000;
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
	   Call  lCaller = new Call(mProvider, aCaller, mSipClient.getContact(mProvider), lCallerListener);
       if (useSdp) {
          lCaller.setLocalSessionDescriptor(SipClient.sdp_offer);
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
	   Call  lCallee = new Call(mProvider, aCallee, mSipClient.getContact(mProvider), lCalleeListener);
       if (useSdp) {
          lCallee.setLocalSessionDescriptor(SipClient.sdp_offer);
       }
	   lCallee.listen();
	   lCaller.call(aCallee);
	   
	   Assert.assertTrue("callee  not alerted until ["+lTimout+"]", lCalleeSemaphoreIncoming.tryAcquire(lTimout,TimeUnit.MILLISECONDS));
	   Assert.assertTrue("caller  call not accepted until ["+lTimout+"]", lCallerSemaphoreAccepted.tryAcquire(lTimout,TimeUnit.MILLISECONDS));
	   Assert.assertTrue("callee  call not confirmed until ["+lTimout+"]", lCalleeSemaphoreConfirmed.tryAcquire(lTimout,TimeUnit.MILLISECONDS));
	   Assert.assertTrue("caller  call not closed until ["+lTimout+"]", lCalleeSemaphoreClosed.tryAcquire(lTimout,TimeUnit.MILLISECONDS));
	   
	}
	public void testBunchOfCall() {
		for (int i=0;i<20;i++) {
			//testCallCancelledBeforeDialogEstablishement();
			testCall();
			testCallCancelledAfterRinging();
		}
	}

}
