Managing proxies
================

User registration is controled by  :cpp:type:`LinphoneProxyConfig` settings.

Each :cpp:type:`LinphoneProxyConfig` object can be configured with registration informations like :cpp:func:`proxy address <linphone_proxy_config_set_server_addr>`,
:cpp:func:`user id <linphone_proxy_config_set_identity>`, :cpp:func:`refresh period <linphone_proxy_config_expires>`, and so on.

A created proxy config using :cpp:func:`linphone_proxy_config_new`, once configured, must be added to :cpp:type:`LinphoneCore` using function :cpp:func:`linphone_core_add_proxy_config`.

It is recommended to set a default :cpp:type:`proxy config <LinphoneProxyConfig>` using function :cpp:func:`linphone_core_set_default_proxy`. Once done,
if :cpp:type:`proxy config <LinphoneProxyConfig>` has been configured with attribute :cpp:func:`enable register <linphone_proxy_config_enable_register>`,
next call to :cpp:func:`linphone_core_iterate` triggers SIP register.

Registration status is reported by LinphoneCoreRegistrationStateChangedCb.

This pseudo code demonstrates basic registration operations:

.. code-block:: c

	LinphoneProxyConfig* proxy_cfg;
	/*create proxy config*/
	proxy_cfg = linphone_proxy_config_new();
	/*parse identity*/
	LinphoneAddress *from = linphone_address_new("sip:toto@sip.titi.com");
	LinphoneAuthInfo *info;
	if (password!=NULL){
		info=linphone_auth_info_new(linphone_address_get_username(from),NULL,"secret",NULL,NULL); /*create authentication structure from identity*/
		linphone_core_add_auth_info(lc,info); /*add authentication info to LinphoneCore*/
	}
	// configure proxy entries
	linphone_proxy_config_set_identity(proxy_cfg,identity); /*set identity with user name and domain*/
	const char* server_addr = linphone_address_get_domain(from); /*extract domain address from identity*/
	linphone_proxy_config_set_server_addr(proxy_cfg,server_addr); /* we assume domain = proxy server address*/
	linphone_proxy_config_enable_register(proxy_cfg,TRUE); /*activate registration for this proxy config*/
	linphone_address_destroy(from); /*release resource*/

	linphone_core_add_proxy_config(lc,proxy_cfg); /*add proxy config to linphone core*/
	linphone_core_set_default_proxy(lc,proxy_cfg); /*set to default proxy*/

Registration sate call back:

.. code-block:: c

	static void registration_state_changed(struct _LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState cstate, const char *message){
			printf("New registration state %s for user id [%s] at proxy [%s]\n"
					,linphone_registration_state_to_string(cstate)
					,linphone_proxy_config_get_identity(cfg)
					,linphone_proxy_config_get_addr(cfg));
	}

Authentication
--------------

Most of the time, registration requires :doc:`authentication <authentication>` to succeed. :cpp:type:`LinphoneAuthInfo` info must be either added
to :cpp:type:`LinphoneCore` using function :cpp:func:`linphone_core_add_auth_info` before :cpp:type:`LinphoneProxyConfig` is added to Linphone core, or on demand
from call back #LinphoneCoreAuthInfoRequestedCb.

Unregistration
--------------

Unregistration or any changes to :cpp:type:`LinphoneProxyConfig` must be first started by a call to function :cpp:func:`linphone_proxy_config_edit` and validated by
function :cpp:func:`linphone_proxy_config_done`.

This pseudo code shows how to unregister a user associated to a #LinphoneProxyConfig:

.. code-block:: c

	LinphoneProxyConfig* proxy_cfg;
	linphone_core_get_default_proxy(lc,&proxy_cfg); /* get default proxy config*/
	linphone_proxy_config_edit(proxy_cfg); /*start editing proxy configuration*/
	linphone_proxy_config_enable_register(proxy_cfg,FALSE); /*de-activate registration for this proxy config*/
	linphone_proxy_config_done(proxy_cfg); /*initiate REGISTER with expire = 0*/

.. seealso:: A complete tutorial can be found at: :ref:`"Basic registration" <basic_registration_code_sample>` source code.

