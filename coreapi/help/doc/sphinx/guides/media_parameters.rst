Controlling media parameters
============================

Multicast
---------

Call using rtp multicast addresses are supported for both audio and video with some limitations. Limitations are, no stun, no ice, no encryption.

* Incoming call with multicast address are automatically accepted. The called party switches in a media receive only mode.
* Outgoing call willing to send media to a multicast address can activate multicast using :cpp:func:`linphone_core_enable_video_multicast`
  or :cpp:func:`linphone_core_enable_audio_multicast`. The calling party switches in a media listen send only mode.
