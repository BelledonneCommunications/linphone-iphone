from nose.tools import assert_equals
import linphone
import linphonetester
import os
import time


class TestCall:

    @classmethod
    def setup_class(cls):
        base, ext = os.path.splitext(os.path.basename(__file__))
        cls.logger = linphonetester.Logger(base + '.log')

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
