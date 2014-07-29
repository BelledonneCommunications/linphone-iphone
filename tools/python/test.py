import linphone
import logging
import signal
import sys
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
logging.basicConfig(level=logging.DEBUG, format="%(asctime)s.%(msecs)-3d %(levelname)s: %(message)s", datefmt="%H:%M:%S")

# Define the linphone module log handler
def log_handler(level, msg):
	method = getattr(logging, level)
	method(msg)


def test_friend():
	f = linphone.Friend.new()
	print(f.address)
	a1 = linphone.Address.new("sip:cotcot@sip.linphone.org")
	print(a1.username)
	print(a1.domain)
	a1.domain = "sip2.linphone.org"
	print(a1.domain)
	f.address = a1
	a2 = f.address


def signal_handler(signal, frame):
	cont = False
	raise KeyError("Ctrl+C")

# Define the iteration function
def iterate(kwargs):
	core = kwargs['core']
	core.iterate()

# Create a linphone core and iterate every 20 ms
linphone.set_log_handler(log_handler)
core = linphone.Core.new(None, None)
interval = IntervalTimer(0.02, iterate, kwargs={'core':core})
signal.signal(signal.SIGINT, signal_handler)
try:
	interval.start()
	signal.pause()
except KeyError:
	interval.stop()
	del interval
del core
