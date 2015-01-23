from nose.tools import assert_equals
from copy import deepcopy
import filecmp
import linphone
from linphonetester import *
import os
import os.path
import time


class TestMessage:

    @classmethod
    def msg_state_changed(cls, msg, state):
        stats = msg.chat_room.core.user_data.stats
        linphonetester_logger.info("[TESTER] Message [{text}] [{state}]".format(text=msg.text, state=linphone.ChatMessageState.string(state)))
        if state == linphone.ChatMessageState.Delivered:
            stats.number_of_LinphoneMessageDelivered += 1
        elif state == linphone.ChatMessageState.NotDelivered:
            stats.number_of_LinphoneMessageNotDelivered += 1
        elif state == linphone.ChatMessageState.InProgress:
            stats.number_of_LinphoneMessageInProgress += 1
        elif state == linphone.ChatMessageState.FileTransferError:
            stats.number_of_LinphoneMessageNotDelivered += 1
        else:
            linphonetester_logger.error("[TESTER] Unexpected state [{state}] for message [{msg}]".format(msg=msg, state=linphone.ChatMessageState.string(state)))

    @classmethod
    def file_transfer_progress_indication(cls, msg, content, offset, total):
        stats = msg.chat_room.core.user_data.stats
        progress = int((offset * 100) / total)
        direction = 'received'
        tofrom = 'from'
        address = msg.from_address
        if msg.outgoing:
            direction = 'sent'
            tofrom = 'to'
            address = msg.to_address
        linphonetester_logger.info("[TESTER] File transfer [{progress}%] {direction} of type [{type}/{subtype}] {tofrom} {address}".format(
            progress=progress, direction=direction, type=content.type, subtype=content.subtype, tofrom=tofrom, address=address.as_string()));
        stats.progress_of_LinphoneFileTransfer = progress

    @classmethod
    def file_transfer_send(cls, msg, content, offset, size):
        send_filepath = msg.user_data
        send_filesize = os.path.getsize(send_filepath)
        if offset >= send_filesize:
            return linphone.Buffer.new() # end of file
        f = open(send_filepath, 'rb')
        f.seek(offset, 0)
        if (send_filesize - offset) < size:
            size = send_filesize - offset
        lb = linphone.Buffer.new_from_data(bytearray(f.read(size)))
        f.close()
        return lb

    @classmethod
    def file_transfer_recv(cls, msg, content, buf):
        receive_filepath = msg.user_data
        stats = msg.chat_room.core.user_data.stats
        if buf.empty: # Transfer complete
            stats.number_of_LinphoneMessageExtBodyReceived += 1
        else: # Store content
            f = open(receive_filepath, 'ab')
            f.write(buf.content)
            f.close()

    @classmethod
    def memory_file_transfer_send(cls, msg, content, offset, size):
        send_buf = msg.user_data
        send_size = len(send_buf)
        if offset >= send_size:
            return linphone.Buffer.new()
        if (send_size - offset) < size:
            size = send_size - offset
        return linphone.Buffer.new_from_string(send_buf[offset:offset+size])

    @classmethod
    def memory_file_transfer_recv(cls, msg, content, buf):
        stats = msg.chat_room.core.user_data.stats
        if buf.empty: # Transfer complete
            stats.number_of_LinphoneMessageExtBodyReceived += 1
        else: # Store content
            msg.user_data += buf.string_content

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
        chat_room.send_chat_message(msg)
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
        chat_room.send_chat_message(msg)
        assert_equals(CoreManager.wait_for(pauline, marie, lambda pauline, marie: marie.stats.number_of_LinphoneMessageReceived == 1), True)
        assert marie.lc.get_chat_room(pauline.identity) is not None
        marie.stop()
        pauline.stop()

    def test_file_transfer_message(self):
        marie = CoreManager('marie_rc')
        pauline = CoreManager('pauline_rc')
        send_filepath = os.path.join(tester_resources_path, 'images', 'nowebcamCIF.jpg')
        receive_filepath = 'receive_file.dump'
        pauline.lc.file_transfer_server = "https://www.linphone.org:444/lft.php"
        chat_room = pauline.lc.get_chat_room(marie.identity)
        content = pauline.lc.create_content()
        content.type = 'image'
        content.subtype = 'jpeg'
        content.size = os.path.getsize(send_filepath) # total size to be transfered
        content.name = 'nowebcamCIF.jpg'
        message = chat_room.create_file_transfer_message(content)
        message.user_data = send_filepath
        self.wait_for_server_to_purge_messages(marie, pauline)
        message.callbacks.msg_state_changed = TestMessage.msg_state_changed
        message.callbacks.file_transfer_send = TestMessage.file_transfer_send
        chat_room.send_chat_message(message)
        assert_equals(CoreManager.wait_for(pauline, marie, lambda pauline, marie: marie.stats.number_of_LinphoneMessageReceivedWithFile == 1), True)
        if marie.stats.last_received_chat_message is not None:
            cbs = marie.stats.last_received_chat_message.callbacks
            cbs.msg_state_changed = TestMessage.msg_state_changed
            cbs.file_transfer_recv = TestMessage.file_transfer_recv
            marie.stats.last_received_chat_message.user_data = receive_filepath
            marie.stats.last_received_chat_message.download_file()
        assert_equals(CoreManager.wait_for(pauline, marie, lambda pauline, marie: marie.stats.number_of_LinphoneMessageExtBodyReceived == 1), True)
        assert_equals(pauline.stats.number_of_LinphoneMessageInProgress, 1)
        assert_equals(pauline.stats.number_of_LinphoneMessageDelivered, 1)
        assert_equals(marie.stats.number_of_LinphoneMessageExtBodyReceived, 1)
        assert_equals(filecmp.cmp(send_filepath, receive_filepath, shallow=False), True)
        if os.path.exists(receive_filepath):
            os.remove(receive_filepath)

    def test_small_file_transfer_message(self):
        send_buf = "small file"
        marie = CoreManager('marie_rc')
        pauline = CoreManager('pauline_rc')
        while len(send_buf) < 160:
            send_buf += send_buf
        l = list(send_buf[0:160])
        l[0] = 'S'
        l[-1] = 'E'
        send_buf = ''.join(l)
        pauline.lc.file_transfer_server = "https://www.linphone.org:444/lft.php"
        chat_room = pauline.lc.get_chat_room(marie.identity)
        content = pauline.lc.create_content()
        content.type = 'text'
        content.subtype = 'plain'
        content.size = len(send_buf) # total size to be transfered
        content.name = 'small_file.txt'
        message = chat_room.create_file_transfer_message(content)
        message.user_data = send_buf
        self.wait_for_server_to_purge_messages(marie, pauline)
        message.callbacks.msg_state_changed = TestMessage.msg_state_changed
        message.callbacks.file_transfer_send = TestMessage.memory_file_transfer_send
        chat_room.send_chat_message(message)
        assert_equals(CoreManager.wait_for(pauline, marie, lambda pauline, marie: marie.stats.number_of_LinphoneMessageReceivedWithFile == 1), True)
        if marie.stats.last_received_chat_message is not None:
            cbs = marie.stats.last_received_chat_message.callbacks
            cbs.msg_state_changed = TestMessage.msg_state_changed
            cbs.file_transfer_recv = TestMessage.memory_file_transfer_recv
            marie.stats.last_received_chat_message.user_data = ''
            marie.stats.last_received_chat_message.download_file()
        assert_equals(CoreManager.wait_for(pauline, marie, lambda pauline, marie: marie.stats.number_of_LinphoneMessageExtBodyReceived == 1), True)
        assert_equals(pauline.stats.number_of_LinphoneMessageInProgress, 1)
        assert_equals(pauline.stats.number_of_LinphoneMessageDelivered, 1)
        assert_equals(marie.stats.number_of_LinphoneMessageExtBodyReceived, 1)
        assert_equals(send_buf, marie.stats.last_received_chat_message.user_data)

    def test_file_transfer_message_upload_cancelled(self):
        send_buf = "big file"
        marie = CoreManager('marie_rc')
        pauline = CoreManager('pauline_rc')
        while len(send_buf) < 128000:
            send_buf += send_buf
        l = list(send_buf[0:128000])
        l[0] = 'S'
        l[-1] = 'E'
        send_buf = ''.join(l)
        pauline.lc.file_transfer_server = "https://www.linphone.org:444/lft.php"
        chat_room = pauline.lc.get_chat_room(marie.identity)
        content = pauline.lc.create_content()
        content.type = 'text'
        content.subtype = 'plain'
        content.size = len(send_buf) # total size to be transfered
        content.name = 'big_file.txt'
        message = chat_room.create_file_transfer_message(content)
        message.user_data = send_buf
        self.wait_for_server_to_purge_messages(marie, pauline)
        message.callbacks.msg_state_changed = TestMessage.msg_state_changed
        message.callbacks.file_transfer_send = TestMessage.memory_file_transfer_send
        message.callbacks.file_transfer_progress_indication = TestMessage.file_transfer_progress_indication
        chat_room.send_chat_message(message)
        # Wait for file to be at least 50% uploaded and cancel the transfer
        assert_equals(CoreManager.wait_for(pauline, marie, lambda pauline, marie: pauline.stats.progress_of_LinphoneFileTransfer >= 50), True)
        message.cancel_file_transfer()
        assert_equals(CoreManager.wait_for(pauline, marie, lambda pauline, marie: pauline.stats.number_of_LinphoneMessageNotDelivered == 1), True)
        assert_equals(pauline.stats.number_of_LinphoneMessageNotDelivered, 1)
        assert_equals(marie.stats.number_of_LinphoneMessageExtBodyReceived, 0)
