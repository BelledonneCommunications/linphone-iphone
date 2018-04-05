Making an audio conference
==========================

This API allows to create a conference entirely managed by the client. No server capabilities are required.

The way such conference is created is by doing the following:

#. The application shall makes "normal" calls to several destinations (using :cpp:func:`linphone_core_invite`), one after another.
#. While initiating the second call, the first one is automatically paused.
#. Then, once the second call is established, the application has the possibility to merge the two calls to form a conference where each participant
   (the local participant, the remote destination of the first call, the remote destination of the second call) can talk together.
   This must be done by adding the two calls to the conference using :cpp:func:`linphone_core_add_to_conference`.

Once merged into a conference the :cpp:type:`LinphoneCall` objects representing the calls that were established remain unchanged, except that
they are tagged as part of the conference (see :cpp:func:`linphone_call_is_in_conference`). The calls in a conference are in the :cpp:enumerator:`LinphoneCallStreamsRunning` state.

Only a single conference can be created: the purpose of this feature is to allow the local user to create, take part and manage the conference.
This API is not designed to create a conference server application.

Up to 10 calls can be merged into the conference, however depending on the CPU usage required for doing the encoding/decoding of the streams of each participants,
the effective limit can be lower.
