from nose.tools import assert_equals
import linphone
from linphonetester import *
import os

class TestSetup:

    def test_address(self):
        create_address(None)

    def test_core_init(self):
        lc = linphone.Core.new({}, None, None)
        assert lc is not None
        if lc is not None:
            lc.verify_server_certificates(False)

    def test_interpret_url(self):
        lc = linphone.Core.new({}, None, None)
        assert lc is not None
        sips_address = "sips:margaux@sip.linphone.org"
        address = lc.interpret_url(sips_address)
        assert address is not None
        assert_equals(address.scheme, "sips")
        assert_equals(address.username, "margaux")
        assert_equals(address.domain, "sip.linphone.org")

    def test_lpconfig_from_buffer(self):
        buffer = "[buffer]\ntest=ok"
        buffer_linebreaks = "[buffer_linebreaks]\n\n\n\r\n\n\r\ntest=ok"
        conf = linphone.LpConfig.new_from_buffer(buffer)
        assert_equals(conf.get_string("buffer", "test", ""), "ok")
        conf = linphone.LpConfig.new_from_buffer(buffer_linebreaks)
        assert_equals(conf.get_string("buffer_linebreaks", "test", ""), "ok")

    def test_create_chat_room(self):
        lc = linphone.Core.new({}, None, None)
        assert lc is not None
        cr = lc.get_chat_room_from_uri("sip:toto@titi.com")
        assert cr is not None
