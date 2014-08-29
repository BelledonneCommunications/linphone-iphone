from nose.tools import assert_equals
from copy import deepcopy
import linphone
import linphonetester
import os
import time


class TestCall:

    @classmethod
    def setup_class(cls):
        base, ext = os.path.splitext(os.path.basename(__file__))
        cls.logger = linphonetester.Logger(base + '.log')

    def call(self, caller_manager, callee_manager, caller_params = None, callee_params = None, build_callee_params = False):
        initial_caller_stats = deepcopy(caller_manager.stats)
        initial_callee_stats = deepcopy(callee_manager.stats)

        # Use playfile for callee to avoid locking on capture card
        callee_manager.lc.use_files = True
        callee_manager.lc.play_file = os.path.join(linphonetester.tester_resources_path, 'sounds', 'hello8000.wav')

        if caller_params is None:
            call = caller_manager.lc.invite_address(callee_manager.identity)
        else:
            call = caller_manager.lc.invite_address_with_params(callee_manager.identity, caller_params)
        assert call is not None

        assert_equals(linphonetester.CoreManager.wait_for(callee_manager, caller_manager,
            lambda callee_manager, caller_manager: callee_manager.stats.number_of_LinphoneCallIncomingReceived == initial_callee_stats.number_of_LinphoneCallIncomingReceived + 1), True)
        assert_equals(callee_manager.lc.incoming_invite_pending, True)
        assert_equals(caller_manager.stats.number_of_LinphoneCallOutgoingProgress, initial_caller_stats.number_of_LinphoneCallOutgoingProgress + 1)

        retry = 0
        while (caller_manager.stats.number_of_LinphoneCallOutgoingRinging != initial_caller_stats.number_of_LinphoneCallOutgoingRinging + 1) and \
            (caller_manager.stats.number_of_LinphoneCallOutgoingEarlyMedia != initial_caller_stats.number_of_LinphoneCallOutgoingEarlyMedia + 1) and \
            retry < 20:
            retry += 1
            caller_manager.lc.iterate()
            callee_manager.lc.iterate()
            time.sleep(0.1)
        assert ((caller_manager.stats.number_of_LinphoneCallOutgoingRinging == initial_caller_stats.number_of_LinphoneCallOutgoingRinging + 1) or \
            (caller_manager.stats.number_of_LinphoneCallOutgoingEarlyMedia == initial_caller_stats.number_of_LinphoneCallOutgoingEarlyMedia + 1)) == True

        assert callee_manager.lc.current_call_remote_address is not None
        if caller_manager.lc.current_call is None or callee_manager.lc.current_call is None or callee_manager.lc.current_call_remote_address is None:
            return False
        callee_from_address = caller_manager.identity.clone()
        callee_from_address.port = 0 # Remove port because port is never present in from header
        assert_equals(callee_from_address.weak_equal(callee_manager.lc.current_call_remote_address), True)

        if callee_params is not None:
            callee_manager.lc.accept_call_with_params(callee_manager.lc.current_call, callee_params)
        elif build_callee_params:
            default_params = callee_manager.lc.create_call_params(callee_manager.lc.current_call)
            callee_manager.lc.accept_call_with_params(callee_manager.lc.current_call, default_params)
        else:
            callee_manager.lc.accept_call(callee_manager.lc.current_call)
        assert_equals(linphonetester.CoreManager.wait_for(callee_manager, caller_manager,
            lambda callee_manager, caller_manager: (callee_manager.stats.number_of_LinphoneCallConnected == initial_callee_stats.number_of_LinphoneCallConnected + 1) and \
                (caller_manager.stats.number_of_LinphoneCallConnected == initial_caller_stats.number_of_LinphoneCallConnected + 1)), True)
        # Just to sleep
        result = linphonetester.CoreManager.wait_for(callee_manager, caller_manager,
            lambda callee_manager, caller_manager: (callee_manager.stats.number_of_LinphoneCallStreamsRunning == initial_callee_stats.number_of_LinphoneCallStreamsRunning + 1) and \
                (caller_manager.stats.number_of_LinphoneCallStreamsRunning == initial_caller_stats.number_of_LinphoneCallStreamsRunning + 1))

        if caller_manager.lc.media_encryption != linphone.MediaEncryption.MediaEncryptionNone and callee_manager.lc.media_encryption != linphone.MediaEncryption.MediaEncryptionNone:
            # Wait for encryption to be on, in case of zrtp, it can take a few seconds
            if caller_manager.lc.media_encryption == linphone.MediaEncryption.MediaEncryptionZRTP:
                linphonetester.CoreManager.wait_for(callee_manager, caller_manager,
                    lambda callee_manager, caller_manager: caller_manager.stats.number_of_LinphoneCallEncryptedOn == initial_caller_stats.number_of_LinphoneCallEncryptedOn + 1)
            if callee_manager.lc.media_encryption == linphone.MediaEncryption.MediaEncryptionZRTP:
                linphonetester.CoreManager.wait_for(callee_manager, caller_manager,
                    lambda callee_manager, caller_manager: callee_manager.stats.number_of_LinphoneCallEncryptedOn == initial_callee_stats.number_of_LinphoneCallEncryptedOn + 1)
            assert_equals(callee_manager.lc.current_call.current_params.media_encryption, caller_manager.lc.media_encryption)
            assert_equals(caller_manager.lc.current_call.current_params.media_encryption, callee_manager.lc.media_encryption)

        return result

    def end_call(self, caller_manager, callee_manager):
        caller_manager.lc.terminate_all_calls()
        assert_equals(linphonetester.CoreManager.wait_for(caller_manager, callee_manager,
            lambda caller_manager, callee_manager: caller_manager.stats.number_of_LinphoneCallEnd == 1 and callee_manager.stats.number_of_LinphoneCallEnd == 1), True)

    def test_early_declined_call(self):
        marie = linphonetester.CoreManager('marie_rc', logger=TestCall.logger)
        pauline = linphonetester.CoreManager('pauline_rc', logger=TestCall.logger)
        marie.lc.max_calls = 0
        out_call = pauline.lc.invite('marie')

        # Wait until flexisip transfers the busy...
        assert_equals(linphonetester.CoreManager.wait_for_until(pauline, marie, lambda pauline, marie: pauline.stats.number_of_LinphoneCallError == 1, 33000), True)
        assert_equals(pauline.stats.number_of_LinphoneCallError, 1)
        assert_equals(out_call.reason, linphone.Reason.ReasonBusy)
        if len(pauline.lc.call_logs) > 0:
            out_call_log = pauline.lc.call_logs[0]
            assert out_call_log is not None
            assert_equals(out_call_log.status, linphone.CallStatus.CallAborted)
        marie.stop()
        pauline.stop()

    def test_declined_call(self):
        marie = linphonetester.CoreManager('marie_rc', logger=TestCall.logger)
        pauline = linphonetester.CoreManager('pauline_rc', logger=TestCall.logger)
        out_call = pauline.lc.invite_address(marie.identity)
        assert_equals(linphonetester.CoreManager.wait_for(pauline, marie, lambda pauline, marie: marie.stats.number_of_LinphoneCallIncomingReceived == 1), True)
        in_call = marie.lc.current_call
        assert in_call is not None
        if in_call is not None:
            marie.lc.terminate_call(in_call)
            assert_equals(linphonetester.CoreManager.wait_for(pauline, marie, lambda pauline, marie: marie.stats.number_of_LinphoneCallReleased == 1), True)
            assert_equals(linphonetester.CoreManager.wait_for(pauline, marie, lambda pauline, marie: pauline.stats.number_of_LinphoneCallReleased == 1), True)
            assert_equals(marie.stats.number_of_LinphoneCallEnd, 1)
            assert_equals(pauline.stats.number_of_LinphoneCallEnd, 1)
            assert_equals(in_call.reason, linphone.Reason.ReasonDeclined)
            assert_equals(out_call.reason, linphone.Reason.ReasonDeclined)
        marie.stop()
        pauline.stop()

    def test_cancelled_call(self):
        marie = linphonetester.CoreManager('marie_rc', logger=TestCall.logger)
        pauline = linphonetester.CoreManager('pauline_rc', logger=TestCall.logger)
        out_call = pauline.lc.invite('marie')
        assert_equals(linphonetester.CoreManager.wait_for(pauline, marie, lambda pauline, marie: pauline.stats.number_of_LinphoneCallOutgoingInit == 1), True)
        pauline.lc.terminate_call(out_call)
        assert_equals(linphonetester.CoreManager.wait_for(pauline, marie, lambda pauline, marie: pauline.stats.number_of_LinphoneCallEnd == 1), True)
        assert_equals(pauline.stats.number_of_LinphoneCallEnd, 1)
        assert_equals(marie.stats.number_of_LinphoneCallIncomingReceived, 0)
        assert_equals(linphonetester.CoreManager.wait_for(pauline, marie, lambda pauline, marie: pauline.stats.number_of_LinphoneCallReleased == 1), True)
        marie.stop()
        pauline.stop()

    def test_early_cancelled_call(self):
        marie = linphonetester.CoreManager('marie_rc', logger=TestCall.logger)
        pauline = linphonetester.CoreManager('empty_rc', check_for_proxies=False, logger=TestCall.logger)
        out_call = pauline.lc.invite_address(marie.identity)
        assert_equals(linphonetester.CoreManager.wait_for(pauline, marie, lambda pauline, marie: pauline.stats.number_of_LinphoneCallOutgoingInit == 1), True)
        pauline.lc.terminate_call(out_call)

        # Since everything is executed in a row, no response can be received from the server, thus the CANCEL cannot be sent.
        # It will ring at Marie's side.
        assert_equals(linphonetester.CoreManager.wait_for(pauline, marie, lambda pauline, marie: pauline.stats.number_of_LinphoneCallEnd == 1), True)
        assert_equals(pauline.stats.number_of_LinphoneCallEnd, 1)
        assert_equals(linphonetester.CoreManager.wait_for(pauline, marie, lambda pauline, marie: marie.stats.number_of_LinphoneCallIncomingReceived == 1), True)

        # Now the CANCEL should have been sent and the the call at marie's side should terminate
        assert_equals(linphonetester.CoreManager.wait_for(pauline, marie, lambda pauline, marie: marie.stats.number_of_LinphoneCallEnd == 1), True)
        assert_equals(linphonetester.CoreManager.wait_for(pauline, marie, lambda pauline, marie: pauline.stats.number_of_LinphoneCallReleased == 1), True)
        marie.stop()
        pauline.stop()

    def test_cancelled_ringing_call(self):
        marie = linphonetester.CoreManager('marie_rc', logger=TestCall.logger)
        pauline = linphonetester.CoreManager('pauline_rc', logger=TestCall.logger)
        out_call = pauline.lc.invite('marie')
        assert_equals(linphonetester.CoreManager.wait_for(pauline, marie, lambda pauline, marie: marie.stats.number_of_LinphoneCallIncomingReceived == 1), True)
        pauline.lc.terminate_call(out_call)
        assert_equals(linphonetester.CoreManager.wait_for(pauline, marie, lambda pauline, marie: (pauline.stats.number_of_LinphoneCallReleased == 1) and (marie.stats.number_of_LinphoneCallReleased == 1)), True)
        assert_equals(marie.stats.number_of_LinphoneCallEnd, 1)
        assert_equals(pauline.stats.number_of_LinphoneCallEnd, 1)
        marie.stop()
        pauline.stop()

    def test_call_failed_because_of_codecs(self):
        marie = linphonetester.CoreManager('marie_rc', logger=TestCall.logger)
        pauline = linphonetester.CoreManager('pauline_rc', logger=TestCall.logger)
        marie.disable_all_audio_codecs_except_one('pcmu')
        pauline.disable_all_audio_codecs_except_one('pcma')
        out_call = pauline.lc.invite('marie')
        assert_equals(linphonetester.CoreManager.wait_for(pauline, marie, lambda pauline, marie: pauline.stats.number_of_LinphoneCallOutgoingInit == 1), True)

        # flexisip will retain the 488 until the "urgent reply" timeout arrives.
        assert_equals(linphonetester.CoreManager.wait_for_until(pauline, marie, lambda pauline, marie: pauline.stats.number_of_LinphoneCallError == 1, 6000), True)
        assert_equals(out_call.reason, linphone.Reason.ReasonNotAcceptable)
        assert_equals(marie.stats.number_of_LinphoneCallIncomingReceived, 0)
        marie.stop()
        pauline.stop()

    def test_simple_call(self):
        marie = linphonetester.CoreManager('marie_rc', logger=TestCall.logger)
        pauline = linphonetester.CoreManager('pauline_rc', logger=TestCall.logger)
        assert_equals(self.call(pauline, marie), True)
        #liblinphone_tester_check_rtcp(marie,pauline);
        self.end_call(marie, pauline)
        marie.stop()
        pauline.stop()
