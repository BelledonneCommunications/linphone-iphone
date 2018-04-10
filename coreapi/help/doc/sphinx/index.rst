.. Linphone API documentation master file, created by
   sphinx-quickstart on Mon Jun 19 11:58:21 2017.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to Linphone API's documentation!
========================================
What is liblinphone
-------------------

Liblinphone is a high level library for bringing SIP video call functionnality
into an application. It aims at making easy the integration of the SIP
video calls into any applications. All variants of linphone are directly based
on it:

* linphone (gtk interface)
* linphonec (console interface)
* linphone for iOS
* linphone for Android

Liblinphone is GPL (see COPYING file). Please understand the licencing details
before using it!

For any use of this library beyond the rights granted to you by the
GPL license, please contact Belledonne Communications
(contact@belledonne-communications.com).


Beginners' guides
-----------------

.. toctree::
	:maxdepth: 1

	guides/initializing
	guides/call_control
	guides/call_misc
	guides/media_parameters
	guides/proxies
	guides/network_parameters
	guides/authentication
	guides/buddy_list
	guides/chatroom
	guides/call_logs
	guides/linphone_address
	guides/conferencing
	guides/event_api
	guides/misc
	guides/ios_portability


Code samples
------------

.. toctree::
	:maxdepth: 1

	samples/samples


API's reference documentation
-----------------------------

.. toctree::
   :maxdepth: 1
   
   reference/c/index
   reference/cpp/index
   reference/java/index
   reference/csharp/index
