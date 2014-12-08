from nose.tools import assert_equals
from copy import deepcopy
import linphone
from linphonetester import *
import os
import time


class TestMessage:

    @classmethod
    def setup_class(cls):
        base, ext = os.path.splitext(os.path.basename(__file__))
        cls.logger = Logger(base + '.log')

    @classmethod
    def teardown_class(cls):
        linphone.testing.clean_accounts()

    def wait_for_server_to_purge_messages(self, manager1, manager2):
        # Wait a little bit just to have time to purge message stored in the server
        CoreManager.wait_for_until(manager1, manager2, lambda manager1, manager2: False, 100)
        manager1.stats.reset()
        manager2.stats.reset()

    def test_text_message(self):
        marie = CoreManager('marie_rc', logger=TestMessage.logger)
        pauline = CoreManager('pauline_rc', logger=TestMessage.logger)
        chat_room = pauline.lc.get_chat_room(marie.identity)
        self.wait_for_server_to_purge_messages(marie, pauline)
        msg = chat_room.create_message("Bla bla bla bla")
        chat_room.send_message2(msg, None, None)
        assert_equals(CoreManager.wait_for(pauline, marie, lambda pauline, marie: marie.stats.number_of_LinphoneMessageReceived == 1), True)
        assert marie.lc.get_chat_room(pauline.identity) is not None
        marie.stop()
        pauline.stop()

    def test_text_message_within_dialog(self):
        marie = CoreManager('marie_rc', logger=TestMessage.logger)
        pauline = CoreManager('pauline_rc', logger=TestMessage.logger)
        pauline.lc.config.set_int('sip', 'chat_use_call_dialogs', 1)
        chat_room = pauline.lc.get_chat_room(marie.identity)
        self.wait_for_server_to_purge_messages(marie, pauline)
        assert_equals(CoreManager.call(marie, pauline), True)
        msg = chat_room.create_message("Bla bla bla bla")
        chat_room.send_message2(msg, None, None)
        assert_equals(CoreManager.wait_for(pauline, marie, lambda pauline, marie: marie.stats.number_of_LinphoneMessageReceived == 1), True)
        assert marie.lc.get_chat_room(pauline.identity) is not None
        marie.stop()
        pauline.stop()
