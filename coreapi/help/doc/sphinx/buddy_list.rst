Managing Buddies and buddy list and presence
============================================
Buddies and buddy list
----------------------

Each buddy is represented by a :cpp:type:`LinphoneFriend` object created by function :cpp:func:`linphone_friend_new`.

Buddy configuration parameters like :cpp:func:`sip uri <linphone_friend_set_addr>` or :cpp:func:`status publication <linphone_friend_set_inc_subscribe_policy>` policy for
this :cpp:type:`friend <LinphoneFriend>` are configurable for each buddy.

Here under a typical buddy creation:

.. code-block:: c

	LinphoneFriend* my_friend=linphone_friend_new_with_addr("sip:joe@sip.linphone.org"); /*creates friend object for buddy joe*/
	linphone_friend_enable_subscribes(my_friend,TRUE); /*configure this friend to emit SUBSCRIBE message after being added to LinphoneCore*/
	linphone_friend_set_inc_subscribe_policy(my_friend,LinphoneSPAccept); /* accept Incoming subscription request for this friend*/

:cpp:type:`Friends <LinphoneFriend>` status changes are reported by callback LinphoneCoreVTable.notify_presence_recv

.. code-block:: c

	static void notify_presence_recv_updated (struct _LinphoneCore *lc,  LinphoneFriend *friend) {
		const LinphoneAddress* friend_address = linphone_friend_get_address(friend);
		printf("New state state [%s] for user id [%s] \n"
					,linphone_online_status_to_string(linphone_friend_get_status(friend))
					,linphone_address_as_string (friend_address));
	}

Once created a buddy can be added to the buddy list using function :cpp:func:`linphone_core_add_friend`. Added friends will be notified
about :cpp:func:`local status changes <linphone_core_set_presence_info>`.

Any subsequente modifications to :cpp:type:`LinphoneFriend` must be first started by a call to function :cpp:func:`linphone_friend_edit` and validated by function :cpp:func:`linphone_friend_done`.

.. code-block:: c

	linphone_friend_edit(my_friend); /* start editing friend */
	linphone_friend_enable_subscribes(my_friend,FALSE); /*disable subscription for this friend*/
	linphone_friend_done(my_friend); /*commit changes triggering an UNSUBSCRIBE message*/


Publishing presence status
--------------------------

Local presence status can be changed using function :cpp:func:`linphone_core_set_presence_model`. New status is propagated to all
friends :cpp:func:`previously added <linphone_core_add_friend>` to :cpp:type:`LinphoneCore`.


Handling incoming subscription request
--------------------------------------

New incoming subscription requests are process according to :cpp:func:`the incoming subscription policy state <linphone_friend_set_inc_subscribe_policy>` for subscription
initiated by :cpp:func:`members of the buddy list <linphone_core_add_friend>`.

For incoming request comming from an unknown buddy, the call back LinphoneCoreVTable.new_subscription_request is invoked.

A complete tutorial can be found at : \ref buddy_tutorials "Registration tutorial"
