Placing and receiving calls
===========================

The :cpp:type:`LinphoneCall` object represents an incoming or outgoing call managed by the :cpp:type:`LinphoneCore`.

Outgoing calls can be created using :cpp:func:`linphone_core_invite` or :cpp:func:`linphone_core_invite_address`, while incoming calls are notified to the application
through the LinphoneCoreVTable::call_state_changed callback.

.. seealso:: :ref:`"Basic Call" <basic_call_code_sample>` source code.
