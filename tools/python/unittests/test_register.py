from nose.tools import assert_equals
import linphone
from linphonetester import *
import os
import time

class RegisterCoreManager(CoreManager):

    @classmethod
    def auth_info_requested(cls, lc, realm, username, domain):
        CoreManager.auth_info_requested(cls, lc, realm, username, domain)
        info = linphone.AuthInfo.new(test_username, None, test_password, None, realm, domain) # Create authentication structure from identity
        lc.add_auth_info(info) # Add authentication info to LinphoneCore

    def __init__(self, with_auth = False):
        vtable = {}
        if with_auth:
            vtable['auth_info_requested'] = RegisterCoreManager.auth_info_requested
        CoreManager.__init__(self, vtable=vtable)

    def register_with_refresh(self, refresh, domain, route, late_auth_info = False, expected_final_state = linphone.RegistrationState.RegistrationOk):
        assert self.lc is not None
        self.stats.reset()
        proxy_cfg = self.lc.create_proxy_config()
        from_address = create_address(domain)
        proxy_cfg.identity = from_address.as_string()
        server_addr = from_address.domain
        proxy_cfg.register_enabled = True
        proxy_cfg.expires = 1
        if route is None:
            proxy_cfg.server_addr = server_addr
        else:
            proxy_cfg.route = route
            proxy_cfg.server_addr = route
        self.lc.add_proxy_config(proxy_cfg)
        self.lc.default_proxy_config = proxy_cfg

        #linphone_core_set_sip_transports(lc,&transport);

        retry = 0
        expected_count = 1
        if refresh:
            expected_count += 1
        max_retry = 110
        if expected_final_state == linphone.RegistrationState.RegistrationProgress:
            max_retry += 200
        while self.stats.number_of_LinphoneRegistrationOk < expected_count and retry < max_retry:
            retry += 1
            self.lc.iterate()
            if self.stats.number_of_auth_info_requested > 0 and proxy_cfg.state == linphone.RegistrationState.RegistrationFailed and late_auth_info:
                if len(self.lc.auth_info_list) == 0:
                    assert_equals(proxy_cfg.error, linphone.Reason.ReasonUnauthorized)
                    info = linphone.AuthInfo.new(test_username, None, test_password, None, None, None) # Create authentication structure from identity
                    self.lc.add_auth_info(info)
            if proxy_cfg.error == linphone.Reason.ReasonForbidden or \
                (self.stats.number_of_auth_info_requested > 2 and proxy_cfg.error == linphone.Reason.ReasonUnauthorized):
                break
            time.sleep(0.1)

        assert_equals(proxy_cfg.state, expected_final_state)
        assert_equals(self.stats.number_of_LinphoneRegistrationNone, 0)
        assert self.stats.number_of_LinphoneRegistrationProgress >= 1
        if expected_final_state == linphone.RegistrationState.RegistrationOk:
            assert_equals(self.stats.number_of_LinphoneRegistrationOk, expected_count)
            expected_failed = 0
            if late_auth_info:
                expected_failed = 1
            assert_equals(self.stats.number_of_LinphoneRegistrationFailed, expected_failed)
        else:
            assert_equals(self.stats.number_of_LinphoneRegistrationCleared, 0)

        self.stop()
        # Not testable as the callbacks can not be called once the core destruction has started
        #assert_equals(self.stats.number_of_LinphoneRegistrationCleared, 1)


class TestRegister:

    def test_simple_register(self):
        cm = RegisterCoreManager()
        cm.register_with_refresh(False, None, None)
        assert_equals(cm.stats.number_of_auth_info_requested, 0)
