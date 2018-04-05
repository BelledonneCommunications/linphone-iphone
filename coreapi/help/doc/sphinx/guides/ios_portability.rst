iOS portability
===============
Multitasking
------------

Liblinphone for IOS natively supports multitasking assuming application follows multitasking guides provided by Apple.

First step is to declare application as multitasked. It means adding background mode for both audio and voip to Info.plist file.

.. code-block:: xml

	<key>UIBackgroundModes</key>
	<array>
		<string>voip</string>
		<string>audio</string>
	</array>


SIP socket
^^^^^^^^^^

Recommended mode is SIP over TCP, because UDP usually requires frequent keep alives for maintaining NAT association at the IP router level. This can
be as frequent as one UDP packet every 15 seconds to maintain the NAT association accross NAT routers. Doing such drains the battery very fast, and
furthermore the iOS keep-alive designed by Apple to handle this task can only be called with a minimum of 10 minutes interval.

For TCP, liblinphone automatically configures SIP socket for voip  (I.E kCFStreamNetworkServiceType set to kCFStreamNetworkServiceTypeVoIP).

.. note::

	Since IOS > 4.1 Apple disabled voip mode for UDP sockets.


Entering background mode
^^^^^^^^^^^^^^^^^^^^^^^^

Before entering in background mode (through ``- (void)applicationDidEnterBackground:(UIApplication *)application``), the application must first refresh
sip registration using function :cpp:func:`linphone_core_refresh_registers` and register a keep-alive handler for periodically refreshing the registration.
The speudo code below shows how to register a keep alive handler:

.. code-block:: objective-c

	//First refresh registration
	linphone_core_refresh_registers(theLinphoneCore);
	//wait for registration answer
	int i=0;
	while (!linphone_proxy_config_is_registered(proxyCfg) && i++<40 ) {
		linphone_core_iterate(theLinphoneCore);
		usleep(100000);
	}
	//register keepalive handler
	[[UIApplication sharedApplication] setKeepAliveTimeout:600/*minimal interval is 600 s*/
	                                   handler:^{
	                                       //refresh sip registration
	                                       linphone_core_refresh_registers(theLinphoneCore);
	                                       //make sure sip REGISTER is sent
	                                       linphone_core_iterate(theLinphoneCore);
	                                   }];


Incoming call notification while in background mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Assuming application using liblinphone is well configured for multitasking, incoming calls arriving while liblinphone is in background mode will simply wakeup
liblinphone thread but not resume GUI. To wakeup GUI, it is recommended to send a Local Notification to the user from the #LinphoneCoreCallStateChangedCb. Here
under a speudo code for this operation:

.. code-block:: objective-c

	if ([UIApplication sharedApplication].applicationState !=  UIApplicationStateActive) {
		// Create a new notification
		UILocalNotification* notif = [[[UILocalNotification alloc] init] autorelease];
		if (notif) {
			notif.repeatInterval = 0;
			notif.alertBody =@"New incoming call";
			notif.alertAction = @"Answer";
			notif.soundName = @"oldphone-mono-30s.caf";

			[[UIApplication sharedApplication]  presentLocalNotificationNow:notif];
		}


Networking
----------
WWAN connection
^^^^^^^^^^^^^^^

Liblinphone relies on iOS's standard BSD socket layer for sip/rtp networking. On IOS, WWAN connection is supposed to automatically bring up on any networking
resquest issued by an application. At least on iPhone OS 3.x, BSD sockets do not implement this behavior. So it is recomended to add a special code to make sure
the WWAN connection is properly setup. Pseudo code below describes a way to force WWAN connection by setting up a dummy TCP connection.

.. code-block:: objective-c

	/*start a new thread to avoid blocking the main ui in case of peer host failure*/
	[NSThread detachNewThreadSelector:@selector(runNetworkConnection) toTarget:self withObject:nil];
	-(void) runNetworkConnection {
		CFWriteStreamRef writeStream;
		//create a dummy socket
		CFStreamCreatePairWithSocketToHost(NULL, (CFStringRef)@"192.168.0.200", 15000, nil, &writeStream);
		CFWriteStreamOpen (writeStream);
		const char* buff="hello";
		//try to write on this socket
		CFWriteStreamWrite (writeStream,(const UInt8*)buff,strlen(buff));
		CFWriteStreamClose (writeStream);
	}

It is recommanded to perform this task each time the application is woken up, including keep alive handler.


Managing IP connection state
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Liblinphone for IOS relies on the application to be informed of network connectivity changes. Network state changes when the IP connection moves from DOWN to UP,
or from WIFI to WWAN. Applications using liblinphone must inform libliblinphone of this changes using function :cpp:func:`linphone_core_set_network_reachable`.
Usually this method is called from the IOS NetworkReachability callback. Here under a sample code:

.. code-block:: c

	//typical reachability callback
	void networkReachabilityCallBack(SCNetworkReachabilityRef target, SCNetworkReachabilityFlags flags, void * info) {
		if ((flags == 0) | (flags & (kSCNetworkReachabilityFlagsConnectionRequired |kSCNetworkReachabilityFlagsConnectionOnTraffic))) {
			//network state is off
			linphone_core_set_network_reachable(lc,false);
			((LinphoneManager*)info).connectivity = none;
		} else {
			Connectivity  newConnectivity = flags & kSCNetworkReachabilityFlagsIsWWAN ? wwan:wifi;
			if (lLinphoneMgr.connectivity == none) {
				//notify new network state
				linphone_core_set_network_reachable(lc,true);
			} else if (lLinphoneMgr.connectivity != newConnectivity) {
				// connectivity has changed
				linphone_core_set_network_reachable(lc,false);
				linphone_core_set_network_reachable(lc,true);
			}
			//store new connectivity status
			lLinphoneMgr.connectivity=newConnectivity;
		}
	}


Sound cards
-----------

Since IOS 5.0, liblinphone supports 2 sound cards. *AU: Audio Unit Receiver* based on IO units for voice calls plus *AQ: Audio Queue Device* dedicated to rings.
Here under the recommended settings (I.E default one)

.. code-block:: c

	linphone_core_set_playback_device(lc, "AU: Audio Unit Receiver");
	linphone_core_set_ringer_device(lc, "AQ: Audio Queue Device");
	linphone_core_set_capture_device(lc, "AU: Audio Unit Receiver");


GSM call interaction
--------------------

To ensure gentle interaction with GSM calls, it is recommended to register an AudioSession delegate. This allows the application to be notified when its audio
session is interrupted/resumed (presumably by a GSM call).

.. code-block:: objective-c

    // declare a class handling the AVAudioSessionDelegate protocol
    @interface MyClass : NSObject <AVAudioSessionDelegate> { [...] }
    // implement 2 methods : here's an example implementation
    -(void) beginInterruption {
        LinphoneCall* c = linphone_core_get_current_call(theLinphoneCore);
        ms_message("Sound interruption detected!");
        if (c) {
            linphone_core_pause_call(theLinphoneCore, c);
        }
    }

    -(void) endInterruption {
        ms_message("Sound interruption ended!");
        const MSList* c = linphone_core_get_calls(theLinphoneCore);

        if (c) {
            ms_message("Auto resuming call");
            linphone_core_resume_call(theLinphoneCore, (LinphoneCall*) c->data);
        }
    }

.. seealso:: http://developer.apple.com/library/ios/#documentation/AVFoundation/Reference/AVAudioSessionDelegate_ProtocolReference/Reference/Reference.html

Declare an instance of your class as AudioSession's delegate :

.. code-block:: objective-c

    [audioSession setDelegate:myClassInstance];

.. seealso:: http://developer.apple.com/library/ios/#documentation/AVFoundation/Reference/AVAudioSession_ClassReference/Reference/Reference.html


Video
-----

Since 3.5 video support has been added to liblinphone for IOS. It requires the application to provide liblinphone with pointers to IOS's views hosting video
display and video previous. These two UIView objects must be passed to the core using functions :cpp:func:`linphone_core_set_native_video_window_id` and
:cpp:func:`linphone_core_set_native_preview_window_id`. Here under pseudo code:

.. code-block:: objective-c

	UIView* display = [[UIView alloc] init];
	UIView* preview = [[UIView alloc] init];
	linphone_core_set_native_video_window_id(lc,(unsigned long)display);
	linphone_core_set_native_preview_window_id(lc,(unsigned long)preview);

Screen rotations are also handled by liblinphone. Two positions are currently supported, namely *UIInterfaceOrientationPortrait* and *UIInterfaceOrientationLandscapeRight*.
Applications may invoke :cpp:func:`linphone_core_set_device_rotation` followed by :cpp:func:`linphone_core_update_call` to notify liblinphone of an orientation
change. Here under a speudo code to handle orientation changes

.. code-block:: c

	-(void) configureOrientation:(UIInterfaceOrientation) oritentation  {
		int oldLinphoneOrientation = linphone_core_get_device_rotation(lc);
		if (oritentation == UIInterfaceOrientationPortrait ) {
			linphone_core_set_native_video_window_id(lc,(unsigned long)display-portrait);
			linphone_core_set_native_preview_window_id(lc,(unsigned long)preview-portrait);
			linphone_core_set_device_rotation(lc, 0);

		} else if (oritentation == UIInterfaceOrientationLandscapeRight ) {
			linphone_core_set_native_video_window_id(lc,(unsigned long)display-landscape);
			linphone_core_set_native_preview_window_id(lc,(unsigned long)preview-landscape);
			linphone_core_set_device_rotation(lc, 270);
		}

		if ((oldLinphoneOrientation != linphone_core_get_device_rotation(lc))
			&& linphone_core_get_current_call(lc)) {
			//Orientation has changed, must call update call
			linphone_core_update_call(lc, linphone_core_get_current_call(lc), NULL);
		}
	}


DTMF feedbacks
--------------

Liblinphone provides functions :cpp:func:`to play dtmf <linphone_core_play_dtmf()>` to the local user. Usually this is used to play a sound when the user
presses a digit, inside or outside of any call. On IOS, libLinphone relies on AudioUnits for interfacing with the audio system. Unfortunately the Audio Unit
initialization is a quite long operation that may trigger a bad user experience if performed each time a DTMF is played, the sound being delayed half a
second after the press. To solve this issue and thus insure real-time precision, liblinphone introduces two functions for :cpp:func:`preloading <linphone_core_start_dtmf_stream>`
and :cpp:func:`unloading <linphone_core_start_dtmf_stream>` the underlying audio graph responsible for playing DTMFs.

For an application using function :cpp:func:`linphone_core_play_dtmf`, it is recommanded to call :cpp:func:`linphone_core_start_dtmf_stream` when entering
in foreground and #linphone_core_stop_dtmf_stream() upon entering background mode.


Plugins
-------

On iOS, plugins are built as static libraries so Liblinphone will not be able to load them at runtime dynamically. Instead, you should declare their prototypes:

.. code-block:: c

	extern void libmsamr_init(MSFactory *factory);
	extern void libmsx264_init(MSFactory *factory);
	extern void libmsopenh264_init(MSFactory *factory);
	extern void libmssilk_init(MSFactory *factory);
	extern void libmsbcg729_init(MSFactory *factory);
	extern void libmswebrtc_init(MSFactory *factory);


Then you should register them after the instantiation of :cpp:type:`LinphoneCore`:

.. code-block:: c

	theLinphoneCore = linphone_core_new_with_config(/* options ... */);

	// Load plugins if available in the linphone SDK - otherwise these calls will do nothing
	MSFactory *f = linphone_core_get_ms_factory(theLinphoneCore);
	libmssilk_init(f);
	libmsamr_init(f);
	libmsx264_init(f);
	libmsopenh264_init(f);
	libmsbcg729_init(f);
	libmswebrtc_init(f);
	linphone_core_reload_ms_plugins(theLinphoneCore, NULL);


If the plugin has not been enabled at compilation time, a stubbed library will be generated with only libplugin_init method declared, doing nothing. You should
see these trace in logs, if plugin is stubbed:

.. code-block:: none

	I/lib/Could not find encoder for SILK
	I/lib/Could not find decoder for SILK
