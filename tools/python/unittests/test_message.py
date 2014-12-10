from nose.tools import assert_equals
from copy import deepcopy
import linphone
from linphonetester import *
import os
import time


class TestMessage:

    @classmethod
    def msg_state_changed(cls, msg, state):
        stats = msg.chat_room.core.user_data.stats
        linphonetester_logger.info("[TESTER] Message [{text}] [{state}]".format(text=msg.text, state=linphone.ChatMessageState.string(state)))
        if state == linphone.ChatMessageState.ChatMessageStateDelivered:
            stats.number_of_LinphoneMessageDelivered += 1
        elif state == linphone.ChatMessageState.ChatMessageStateNotDelivered:
            stats.number_of_LinphoneMessageNotDelivered += 1
        elif state == linphone.ChatMessageState.ChatMessageStateInProgress:
            stats.number_of_LinphoneMessageInProgress += 1
        elif state == linphone.ChatMessageState.ChatMessageStateFileTransferError:
            stats.number_of_LinphoneMessageNotDelivered += 1
        else:
            linphonetester_logger.error("[TESTER] Unexpected state [{state}] for message [{msg}]".format(msg=msg, state=linphone.ChatMessageState.string(state)))

    @classmethod
    def file_transfer_received(cls, msg, content, buf, size):
        print buf, size
        stats = msg.chat_room.core.user_data.stats
        if msg.user_data is None:
            msg.user_data = open('receive_file.dump', 'wb')
            msg.user_data.write(buf)
        else:
            if size == 0: # Transfer complete
                stats.number_of_LinphoneMessageExtBodyReceived += 1
                msg.user_data.close()
            else: # Store content
                msg.user_data.write(buf)

    def wait_for_server_to_purge_messages(self, manager1, manager2):
        # Wait a little bit just to have time to purge message stored in the server
        CoreManager.wait_for_until(manager1, manager2, lambda manager1, manager2: False, 100)
        manager1.stats.reset()
        manager2.stats.reset()

    def test_text_message(self):
        marie = CoreManager('marie_rc')
        pauline = CoreManager('pauline_rc')
        chat_room = pauline.lc.get_chat_room(marie.identity)
        self.wait_for_server_to_purge_messages(marie, pauline)
        msg = chat_room.create_message("Bla bla bla bla")
        chat_room.send_message2(msg, None, None)
        assert_equals(CoreManager.wait_for(pauline, marie, lambda pauline, marie: marie.stats.number_of_LinphoneMessageReceived == 1), True)
        assert marie.lc.get_chat_room(pauline.identity) is not None
        marie.stop()
        pauline.stop()

    def test_text_message_within_dialog(self):
        marie = CoreManager('marie_rc')
        pauline = CoreManager('pauline_rc')
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

    def test_file_transfer_message(self):
        big_file = "big file"
        marie = CoreManager('marie_rc')
        pauline = CoreManager('pauline_rc')
        while len(big_file) < 128000:
            big_file += big_file
        l = list(big_file)
        l[0] = 'S'
        l[-1] = 'E'
        big_file = ''.join(l)
        pauline.lc.file_transfer_server = "https://www.linphone.org:444/lft.php"
        chat_room = pauline.lc.get_chat_room(marie.identity)
        content = pauline.lc.create_content()
        content.type = 'text'
        content.subtype = 'plain'
        content.size = len(big_file) # total size to be transfered
        content.name = 'bigfile.txt'
        message = chat_room.create_file_transfer_message(content)
        self.wait_for_server_to_purge_messages(marie, pauline)
        message.callbacks.msg_state_changed = TestMessage.msg_state_changed
        chat_room.send_chat_message(message)
        assert_equals(CoreManager.wait_for(pauline, marie, lambda pauline, marie: marie.stats.number_of_LinphoneMessageReceivedWithFile == 1), True)
        if marie.stats.last_received_chat_message is not None:
            cbs = marie.stats.last_received_chat_message.callbacks
            cbs.msg_state_changed = TestMessage.msg_state_changed
            cbs.file_transfer_recv = TestMessage.file_transfer_received
            marie.stats.last_received_chat_message.download_file()
        assert_equals(CoreManager.wait_for(pauline, marie, lambda pauline, marie: marie.stats.number_of_LinphoneMessageExtBodyReceived == 1), True)
        assert_equals(pauline.stats.number_of_LinphoneMessageInProgress, 1)
        assert_equals(pauline.stats.number_of_LinphoneMessageDelivered, 1)
        assert_equals(marie.stats.number_of_LinphoneMessageExtBodyReceived, 1)
