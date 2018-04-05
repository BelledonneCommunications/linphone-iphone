Chat room and messaging
=======================
Exchanging text messages
------------------------

Messages are sent using :cpp:type:`LinphoneChatRoom` object. First step is to create a :cpp:func:`chat room <linphone_core_get_chat_room>`
from a peer sip uri.

.. code-block:: c

	LinphoneChatRoom* chat_room = linphone_core_get_chat_room(lc,"sip:joe@sip.linphone.org");

Once created, messages are sent using function :cpp:func:`linphone_chat_room_send_message`.

.. code-block:: c

	linphone_chat_room_send_message(chat_room,"Hello world"); /*sending message*/

Incoming message are received from call back LinphoneCoreVTable.text_received

.. code-block:: c

	void text_received(LinphoneCore *lc, LinphoneChatRoom *room, const LinphoneAddress *from, const char *message) {
		printf(" Message [%s] received from [%s] \n",message,linphone_address_as_string (from));
	}

.. seealso:: A complete tutorial can be found at :ref:`"Chatroom and messaging" <chatroom_code_sample>` source code.

