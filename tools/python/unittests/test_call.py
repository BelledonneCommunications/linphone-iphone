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
