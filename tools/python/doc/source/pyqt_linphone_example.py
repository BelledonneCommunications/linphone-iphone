import linphone
import logging
import sys
from PyQt4.QtCore import QTimer
from PyQt4.QtGui import QApplication


def main():
    logging.basicConfig(level=logging.INFO)

    app = QApplication(sys.argv)

    def log_handler(level, msg):
        method = getattr(logging, level)
        method(msg)

    def global_state_changed(*args, **kwargs):
        logging.warning("global_state_changed: %r %r" % (args, kwargs))

    def registration_state_changed(core, call, state, message):
        logging.warning("registration_state_changed: " + str(state) + ", " + message)

    callbacks = {
        'global_state_changed': global_state_changed,
        'registration_state_changed': registration_state_changed,
    }

    linphone.set_log_handler(log_handler)
    core = linphone.Core.new(callbacks, None, None)
    proxy_cfg = core.create_proxy_config()
    proxy_cfg.identity = "sip:toto@test.linphone.org"
    proxy_cfg.server_addr = "sip:test.linphone.org"
    proxy_cfg.register_enabled = True
    core.add_proxy_config(proxy_cfg)

    iterate_timer = QTimer()
    iterate_timer.timeout.connect(core.iterate)
    stop_timer = QTimer()
    stop_timer.timeout.connect(app.quit)
    iterate_timer.start(20)
    stop_timer.start(5000)

    exitcode = app.exec_()
    sys.exit(exitcode)


main()
