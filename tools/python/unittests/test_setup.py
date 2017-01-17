from nose.tools import assert_equals
import linphone
from linphonetester import *
import os

class TestSetup:

    def teardown(self):
        linphone.Factory.clean()

    def test_version(self):
        lc = linphone.Factory.get().create_core(None, None, None)
        assert_equals(lc.version.find("unknown"), -1)

    def test_address(self):
        create_address(None)

    def test_core_init(self):
        lc = linphone.Factory.get().create_core(None, None, None)
        assert lc is not None
        if lc is not None:
            lc.verify_server_certificates(False)

    def test_random_transport(self):
        lc = linphone.Factory.get().create_core(None, None, None)
        assert lc is not None
        tr = lc.sip_transports
        assert_equals(tr.udp_port, 5060) # default config
        assert_equals(tr.tcp_port, 5060) # default config
        tr.udp_port = -1
        tr.tcp_port = -1
        tr.tls_port = -1
        lc.sip_transports = tr
        tr = lc.sip_transports
        assert tr.udp_port != 5060 # default config
        assert tr.tcp_port != 5060 # default config
        assert_equals(lc.config.get_int('sip', 'sip_port', -2), -1)
        assert_equals(lc.config.get_int('sip', 'sip_tcp_port', -2), -1)
        assert_equals(lc.config.get_int('sip', 'sip_tls_port', -2), -1)

    def test_config_from_buffer(self):
        buffer = "[buffer]\ntest=ok"
        buffer_linebreaks = "[buffer_linebreaks]\n\n\n\r\n\n\r\ntest=ok"
        conf = linphone.Config.new_from_buffer(buffer)
        assert_equals(conf.get_string("buffer", "test", ""), "ok")
        conf = linphone.Config.new_from_buffer(buffer_linebreaks)
        assert_equals(conf.get_string("buffer_linebreaks", "test", ""), "ok")

    def test_config_zerolen_value_from_buffer(self):
        zerolen = "[test]\nzero_len=\nnon_zero_len=test"
        conf = linphone.Config.new_from_buffer(zerolen)
        assert_equals(conf.get_string("test", "zero_len", "LOL"), "LOL")
        assert_equals(conf.get_string("test", "non_zero_len", ""), "test")
        conf.set_string("test", "non_zero_len", "") # should remove "non_zero_len"
        assert_equals(conf.get_string("test", "non_zero_len", "LOL"), "LOL")

    def test_config_zerolen_value_from_file(self):
        conf = linphone.Config.new_with_factory(None, os.path.join(tester_resources_path, 'rcfiles', 'zero_length_params_rc'))
        assert_equals(conf.get_string("test", "zero_len", "LOL"), "LOL")
        # non_zero_len=test -> should return test
        assert_equals(conf.get_string("test", "non_zero_len", ""), "test")
        conf.set_string("test", "non_zero_len", "") # should remove "non_zero_len"
        assert_equals(conf.get_string("test", "non_zero_len", "LOL"), "LOL")

    def test_create_chat_room(self):
        lc = linphone.Factory.get().create_core(None, None, None)
        assert lc is not None
        cr = lc.get_chat_room_from_uri("sip:toto@titi.com")
        assert cr is not None
