from nose.tools import assert_equals
import linphone

test_username = "liblinphone_tester"
test_route = "sip2.linphone.org"

def create_address(domain):
    addr = linphone.Address.new(None)
    assert addr != None
    addr.username = test_username
    assert_equals(addr.username, test_username)
    if domain is not None:
        domain = test_route
    addr.domain = domain
    assert_equals(addr.domain, domain)
    addr.display_name = None
    addr.display_name = "Mr Tester"
    assert_equals(addr.display_name, "Mr Tester")
    return addr

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
