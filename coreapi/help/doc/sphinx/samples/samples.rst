.. _basic_call_code_sample:

Basic call
==========

This program is a *very* simple usage example of liblinphone. It just takes a sip-uri as first argument and attempts to call it

.. literalinclude:: helloworld.c
	:language: c


.. _basic_registration_code_sample:

Basic registration
==================

This program is a *very* simple usage example of liblinphone, desmonstrating how to initiate a SIP registration from a sip uri identity
passed from the command line. First argument must be like sip:jehan@sip.linphone.org , second must be *password* . Registration is cleared
on *SIGINT*.

Example: ``registration sip:jehan@sip.linphone.org secret``

.. literalinclude:: registration.c
	:language: c


.. _subscribe_notify_code_sample:

Generic subscribe/notify example
================================

This program is a *very* simple usage example of liblinphone. It demonstrates how to listen to a SIP subscription.
It then sends notify requests back periodically. First argument must be like sip:jehan@sip.linphone.org , second must be *password*.
Registration is cleared on *SIGINT*.

Example: ``registration sip:jehan@sip.linphone.org secret``

.. literalinclude:: registration.c
	:language: c



.. _buddy_status_notification_code_sample:

Basic buddy status notification
===============================

This program is a *very* simple usage example of liblinphone, demonstrating how to initiate  SIP subscriptions and receive
notifications from a sip uri identity passed from the command line. Argument must be like sip:jehan@sip.linphone.org .
Subscription is cleared on *SIGINT* signal.

Example: ``budy_list sip:jehan@sip.linphone.org``

.. literalinclude:: buddy_status.c
	:language: c


.. _chatroom_code_sample:

Chat room and messaging
=======================

This program is a *very* simple usage example of liblinphone, desmonstrating how to send/receive  SIP MESSAGE from a sip uri
identity passed from the command line. Argument must be like sip:jehan@sip.linphone.org .

Example: ``chatroom sip:jehan@sip.linphone.org``

.. literalinclude:: chatroom.c
	:language: c


.. _file_transfer_code_sample:

File transfer
=============

.. literalinclude:: filetransfer.c
	:language: c



.. _RT text receiver_code_sample:

Real Time Text Receiver
=======================

This program is able to receive chat message in real time on port 5060. Use realtimetext_sender to generate chat message

Example: ``./realtimetext_receiver``

.. literalinclude:: realtimetext_sender.c
	:language: c


.. _RT_text_sender_code_sample:

Real Time Text Sender
=====================

This program just send chat message in real time to dest uri. Use realtimetext_receiver to receive  message.

Example: ``./realtimetext_sender sip:localhost:5060``

.. literalinclude:: realtimetext_sender.c
	:language: c

