from nose.tools import assert_equals
import linphone
from linphonetester import *
import os
import time

class RegisterCoreManager(CoreManager):

    @classmethod
    def authentication_requested(cls, lc, auth_info, method):
        info = linphone.Factory.get().create_auth_info(test_username, None, test_password, None, auth_info.realm, auth_info.domain) # Create authentication structure from identity
        lc.add_auth_info(info) # Add authentication info to LinphoneCore

    def __init__(self, with_auth = False):
        additional_cbs = None
        if with_auth:
            additional_cbs = linphone.Factory.get().create_core_cbs()
            additional_cbs.authentication_requested = RegisterCoreManager.authentication_requested
        CoreManager.__init__(self, additional_cbs=additional_cbs)

    def __del__(self):
        linphonetester_logger.info("deleting" + str(self))

    def register_with_refresh_base(self, refresh, domain, route, late_auth_info = False, transport = linphone.SipTransports(5070, 5070, 5071, 0), expected_final_state = linphone.RegistrationState.Ok):
        assert self.lc is not None
        self.stats.reset()
        self.lc.sip_transports = transport
        proxy_cfg = self.lc.create_proxy_config()
        from_address = create_address(domain)
        proxy_cfg.identity_address = from_address
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

        retry = 0
        expected_count = 1
        if refresh:
            expected_count += 1
        max_retry = 110
        if expected_final_state == linphone.RegistrationState.Progress:
            max_retry += 200
        while self.stats.number_of_LinphoneRegistrationOk < expected_count and retry < max_retry:
            retry += 1
            self.lc.iterate()
            if self.stats.number_of_auth_info_requested > 0 and proxy_cfg.state == linphone.RegistrationState.Failed and late_auth_info:
                if len(self.lc.auth_info_list) == 0:
                    assert_equals(proxy_cfg.error, linphone.Reason.Unauthorized)
                    info = linphone.Factory.get().create_auth_info(test_username, None, test_password, None, None, None) # Create authentication structure from identity
                    self.lc.add_auth_info(info)
            if proxy_cfg.error == linphone.Reason.Forbidden or \
                (self.stats.number_of_auth_info_requested > 2 and proxy_cfg.error == linphone.Reason.Unauthorized):
                break
            time.sleep(0.1)

        assert_equals(proxy_cfg.state, expected_final_state)
        assert_equals(self.stats.number_of_LinphoneRegistrationNone, 0)
        assert self.stats.number_of_LinphoneRegistrationProgress >= 1
        if expected_final_state == linphone.RegistrationState.Ok:
            assert_equals(self.stats.number_of_LinphoneRegistrationOk, expected_count)
            expected_failed = 0
            if late_auth_info:
                expected_failed = 1
            assert_equals(self.stats.number_of_LinphoneRegistrationFailed, expected_failed)
        else:
            assert_equals(self.stats.number_of_LinphoneRegistrationCleared, 0)

    def register_with_refresh(self, refresh, domain, route, late_auth_info = False, transport = linphone.SipTransports(5070, 5070, 5071, 0), expected_final_state = linphone.RegistrationState.Ok):
        self.register_with_refresh_base(refresh, domain, route, late_auth_info, expected_final_state = expected_final_state)
        # Not testable as the callbacks can not be called once the core destruction has started
        #assert_equals(self.stats.number_of_LinphoneRegistrationCleared, 1)


class TestRegister:

    def teardown(self):
        linphone.Factory.clean()

    def test_simple_register(self):
        cm = RegisterCoreManager()
        cm.register_with_refresh(False, None, None)
        assert_equals(cm.stats.number_of_auth_info_requested, 0)

    def test_simple_unregister(self):
        cm = RegisterCoreManager()
        cm.register_with_refresh_base(False, None, None)
        pc = cm.lc.default_proxy_config
        pc.edit()
        cm.stats.reset() # clear stats
        # nothing is supposed to arrive until done
        assert_equals(CoreManager.wait_for_until(cm, cm, lambda cm1, cm2: cm1.stats.number_of_LinphoneRegistrationCleared == 1, 3000), False)
        pc.register_enabled = False
        pc.done()
        assert_equals(CoreManager.wait_for(cm, cm, lambda cm1, cm2: cm1.stats.number_of_LinphoneRegistrationCleared == 1), True)

    def test_simple_tcp_register(self):
        cm = RegisterCoreManager()
        cm.register_with_refresh(False, test_domain, "sip:{route};transport=tcp".format(route=test_route))

    def test_simple_tcp_register_compatibility_mode(self):
        cm = RegisterCoreManager()
        cm.register_with_refresh(False, test_domain, "sip:{route}".format(route=test_route), transport=linphone.SipTransports(0, 5070, 0, 0))

    def test_simple_tls_register(self):
        cm = RegisterCoreManager()
        cm.register_with_refresh(False, test_domain, "sip:{route};transport=tls".format(route=test_route))

    def test_tls_register_with_alt_name(self):
        cm = CoreManager('pauline_alt_rc', False)
        cm.lc.root_ca = os.path.join(tester_resources_path, 'certificates', 'cn', 'cafile.pem')
        cm.lc.refresh_registers()
        assert_equals(CoreManager.wait_for(cm, cm, lambda cm1, cm2: cm1.stats.number_of_LinphoneRegistrationOk == 1), True)
        assert_equals(cm.stats.number_of_LinphoneRegistrationFailed, 0)

    def test_tls_wildcard_register(self):
        cm = CoreManager('pauline_wild_rc', False)
        cm.lc.root_ca = os.path.join(tester_resources_path, 'certificates', 'cn', 'cafile.pem')
        cm.lc.refresh_registers()
        assert_equals(CoreManager.wait_for(cm, cm, lambda cm1, cm2: cm1.stats.number_of_LinphoneRegistrationOk == 2), True)
        assert_equals(cm.stats.number_of_LinphoneRegistrationFailed, 0)

    def test_tls_certificate_failure(self):
        cm = CoreManager('pauline_rc', False)
        cm.lc.root_ca = os.path.join(tester_resources_path, 'certificates', 'cn', 'agent.pem') # bad root ca
        cm.lc.network_reachable = True
        assert_equals(CoreManager.wait_for(cm, cm, lambda cm1, cm2: cm1.stats.number_of_LinphoneRegistrationFailed == 1), True)
        cm.lc.root_ca = None # no root ca
        cm.lc.refresh_registers()
        assert_equals(CoreManager.wait_for(cm, cm, lambda cm1, cm2: cm1.stats.number_of_LinphoneRegistrationFailed == 2), True)
        cm.lc.root_ca = os.path.join(tester_resources_path, 'certificates', 'cn', 'cafile.pem') # good root ca
        cm.lc.refresh_registers()
        assert_equals(CoreManager.wait_for(cm, cm, lambda cm1, cm2: cm1.stats.number_of_LinphoneRegistrationOk == 1), True)
        assert_equals(cm.stats.number_of_LinphoneRegistrationFailed, 2)

    def test_tls_with_non_tls_server(self):
        cm = CoreManager('marie_rc', False)
        cm.lc.sip_transport_timeout = 3000
        pc = cm.lc.default_proxy_config
        pc.edit()
        addr = linphone.Factory.get().create_address(pc.server_addr)
        port = addr.port
        if port <= 0:
            port = 5060
        pc.server_addr = "sip:{domain}:{port};transport=tls".format(domain=addr.domain, port=port)
        pc.done()
        assert_equals(CoreManager.wait_for_until(cm, cm, lambda cm1, cm2: cm1.stats.number_of_LinphoneRegistrationFailed == 1, 10000), True)

    def test_simple_authenticated_register(self):
        cm = RegisterCoreManager()
        info = linphone.Factory.get().create_auth_info(test_username, None, test_password, None, auth_domain, None) # Create authentication structure from identity
        cm.lc.add_auth_info(info)
        cm.register_with_refresh(False, auth_domain, "sip:{route}".format(route=test_route))
        assert_equals(cm.stats.number_of_auth_info_requested, 0)

    def test_digest_auth_without_initial_credentials(self):
        cm = RegisterCoreManager(with_auth=True)
        cm.register_with_refresh(False, auth_domain, "sip:{route}".format(route=test_route))
        assert_equals(cm.stats.number_of_auth_info_requested, 1)

    def test_authenticated_register_with_late_credentials(self):
        cm = RegisterCoreManager()
        cm.register_with_refresh(False, auth_domain, "sip:{route}".format(route=test_route), True, linphone.SipTransports(5070, 5070, 5071, 0))
        assert_equals(cm.stats.number_of_auth_info_requested, 1)

    def test_simple_register_with_refresh(self):
        cm = RegisterCoreManager()
        cm.register_with_refresh(True, None, None)
        assert_equals(cm.stats.number_of_auth_info_requested, 0)

    def test_simple_auth_register_with_refresh(self):
        cm = RegisterCoreManager(with_auth=True)
        cm.register_with_refresh(True, auth_domain, "sip:{route}".format(route=test_route))
        assert_equals(cm.stats.number_of_auth_info_requested, 1)

    def test_multiple_accounts(self):
        CoreManager('multi_account_rc', False)

    def test_transport_change(self):
        cm = CoreManager('multi_account_rc', True)
        number_of_udp_proxies = reduce(lambda x, y: x + int(y.transport == "udp"), cm.lc.proxy_config_list, 0)
        total_number_of_proxies = len(cm.lc.proxy_config_list)
        register_ok = cm.stats.number_of_LinphoneRegistrationOk
        # Keep only UDP
        tr = linphone.SipTransports(0, 0, 0, 0)
        tr.udp_port = cm.lc.sip_transports.udp_port
        cm.lc.sip_transports = tr
        assert_equals(CoreManager.wait_for(cm, cm, lambda cm1, cm2: cm1.stats.number_of_LinphoneRegistrationOk == (register_ok + number_of_udp_proxies)), True)
        assert_equals(CoreManager.wait_for(cm, cm, lambda cm1, cm2: cm1.stats.number_of_LinphoneRegistrationFailed == (total_number_of_proxies - number_of_udp_proxies)), True)
