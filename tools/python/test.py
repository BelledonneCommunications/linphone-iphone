import linphone
import logging
import threading
import time


class StoppableThread(threading.Thread):
	def __init__(self):
		threading.Thread.__init__(self)
		self.stop_event = threading.Event()

	def stop(self):
		if self.isAlive() == True:
			# Set an event to signal the thread to terminate
			self.stop_event.set()
			# Block the calling thread until the thread really has terminated
			self.join()

class IntervalTimer(StoppableThread):
	def __init__(self, interval, worker_func, kwargs={}):
		StoppableThread.__init__(self)
		self._interval = interval
		self._worker_func = worker_func
		self._kwargs = kwargs

	def run(self):
		while not self.stop_event.is_set():
			self._worker_func(self._kwargs)
			time.sleep(self._interval)


# Configure logging module
logging.addLevelName(logging.DEBUG, "\033[1;37m%s\033[1;0m" % logging.getLevelName(logging.DEBUG))
logging.addLevelName(logging.INFO, "\033[1;36m%s\033[1;0m" % logging.getLevelName(logging.INFO))
logging.addLevelName(logging.WARNING, "\033[1;31m%s\033[1;0m" % logging.getLevelName(logging.WARNING))
logging.addLevelName(logging.ERROR, "\033[1;41m%s\033[1;0m" % logging.getLevelName(logging.ERROR))
logging.basicConfig(level=logging.INFO, format="%(asctime)s.%(msecs)03d %(levelname)s: %(message)s", datefmt="%H:%M:%S")

# Define the linphone module log handler
def log_handler(level, msg):
	method = getattr(logging, level)
	if not msg.strip().startswith('[PYLINPHONE]'):
		msg = '[CORE] ' + msg
	method(msg)

# Define the iteration function
def iterate(kwargs):
	core = kwargs['core']
	core.iterate()

def interact():
	choice = raw_input('> ')
	if choice == "quit":
		return False
	return True

def global_state_changed(core, state, message):
	logging.warning("[PYTHON] global_state_changed: " + str(state) + ", " + message)
	if state == linphone.GlobalState.GlobalOn:
		logging.warning("[PYTHON] core version: " + str(core.version))

# Create a linphone core and iterate every 20 ms
linphone.set_log_handler(log_handler)
callbacks = {
	'global_state_changed':global_state_changed
}
core = linphone.Core.new(callbacks, None, None)
interval = IntervalTimer(0.02, iterate, kwargs={'core':core})
interval.start()
while interact():
	pass
interval.stop()
