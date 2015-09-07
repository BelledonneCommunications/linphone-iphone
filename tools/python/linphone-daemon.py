import argparse
import logging
import sys
import threading
import time
import linphone


class Response:
	Ok = 0
	Error = 1

	def __init__(self, status, msg = ''):
		self.status = status
		if status == Response.Ok:
			self.body = msg
			self.reason = None
		else:
			self.body = None
			self.reason = msg

	def __str__(self):
		status_str = ["Ok", "Error"][self.status]
		body = ''
		if self.reason:
			body += "Reason: {reason}\n".format(reason=self.reason)
		if self.body:
			body += '\n' + self.body + '\n'
		return \
"""Status: {status}
{body}""".format(status=status_str, body=body)

class RegisterStatusResponse(Response):
	def __init__(self):
		Response.__init__(self, Response.Ok)

	def append(self, id, proxy_cfg):
		self.body += \
"""Id: {id}
State: {state}
""".format(id=id, state=str(linphone.RegistrationState.string(proxy_cfg.state)))

class CommandExample:
	def __init__(self, command, output):
		self.command = command
		self.output = output

	def __str__(self):
		return \
"""> {command}
{output}""".format(command=self.command, output=self.output)

class Command:
	def __init__(self, name, proto):
		self.name = name
		self.proto = proto
		self.examples = []

	def exec_command(self, app, args):
		pass

	def add_example(self, example):
		self.examples.append(example)

	def help(self):
		body = \
"""{proto}

Description:
{description}
""".format(proto=self.proto, description=self.__doc__)
		idx = 0
		for example in self.examples:
			idx += 1
			body += \
"""
Example {idx}:
{example}
""".format(idx=idx, example=str(example))
		return body

class CallCommand(Command):
	"""Place a call."""
	def __init__(self):
		Command.__init__(self, "call", "call <sip-address>")
		self.add_example(CommandExample(
			"call daemon-test@sip.linphone.org",
			"Status: Ok\n\nId: 1"
		))
		self.add_example(CommandExample(
			"call daemon-test@sip.linphone.org",
			"Status: Error\nReason: Call creation failed."
		))

	def exec_command(self, app, args):
		if len(args) >= 1:
			call = app.core.invite(args[0])
			if call is None:
				app.send_response(Response(Response.Error, "Call creation failed."))
			else:
				id = app.update_call_id(call)
				app.send_response(Response(Response.Ok, "Id: " + str(id)))
		else:
			app.send_response(Response(Response.Error, "Missing parameter."))

class CallPauseCommand(Command):
	"""Pause a call (pause current if no id is specified)."""
	def __init__(self):
		Command.__init__(self, "call-pause", "call-pause [call-id]")
		self.add_example(CommandExample(
			"call-pause 1",
			"Status: Ok\n\nCall was paused"
		))
		self.add_example(CommandExample(
			"call-pause 2",
			"Status: Error\nReason: No call with such id."
		))
		self.add_example(CommandExample(
			"call-pause",
			"Status: Error\nReason: No current call available."
		))

	def exec_command(self, app, args):
		current = False
		if len(args) >= 1:
			call = app.find_call(args[0])
			if call is None:
				app.send_response(Response(Response.Error, "No call with such id."))
				return
		else:
			current = True
			call = app.core.current_call
			if call is None:
				app.send_response(Response(Response.Error, "No current call available."))
				return
		if app.core.pause_call(call) == 0:
			msg = "Call was paused."
			if current:
				msg = "Current call was paused."
			app.send_response(Response(Response.Ok, msg))
		else:
			app.send_response(Response(Response.Error, "Error pausing call."))

class CallResumeCommand(Command):
	"""Resume a call (resume current if no id is specified)."""
	def __init__(self):
		Command.__init__(self, "call-resume", "call-resume [call-id]")
		self.add_example(CommandExample(
			"call-resume 1",
			"Status: Ok\n\nCall was resumed"
		))
		self.add_example(CommandExample(
			"call-resume 2",
			"Status: Error\nReason: No call with such id."
		))
		self.add_example(CommandExample(
			"call-resume",
			"Status: Error\nReason: No current call available."
		))

	def exec_command(self, app, args):
		current = False
		if len(args) >= 1:
			call = app.find_call(args[0])
			if call is None:
				app.send_response(Response(Response.Error, "No call with such id."))
				return
		else:
			current = True
			call = app.core.current_call
			if call is None:
				app.send_response(Response(Response.Error, "No current call available."))
				return
		if app.core.resume_call(call) == 0:
			msg = "Call was resumed."
			if current:
				msg = "Current call was resumed."
			app.send_response(Response(Response.Ok, msg))
		else:
			app.send_response(Response(Response.Error, "Error resuming call."))

class CallStatusCommand(Command):
	"""Return status of the specified call or of the current call if no id is given."""
	def __init__(self):
		Command.__init__(self, "call-status", "call-status [call-id]")
		self.add_example(CommandExample(
			"call-status 1",
			"Status: Ok\n\nState: LinphoneCallStreamsRunning\nFrom: <sip:daemon-test@sip.linphone.org>\nDirection: out\nDuration: 6"
		))
		self.add_example(CommandExample(
			"call-status 2",
			"Status: Error\nReason: No call with such id."
		))
		self.add_example(CommandExample(
			"call-status",
			"Status: Error\nReason: No current call available."
		))

	def exec_command(self, app, args):
		if len(args) >= 1:
			call = app.find_call(args[0])
			if call is None:
				app.send_response(Response(Response.Error, "No call with such id."))
				return
		else:
			call = app.core.current_call
			if call is None:
				app.send_response(Response(Response.Error, "No current call available."))
				return
		state = call.state
		body = "State: {state}".format(state=linphone.CallState.string(state))
		if state == linphone.CallState.CallOutgoingInit \
			or state == linphone.CallState.CallOutgoingProgress \
			or state == linphone.CallState.CallOutgoingRinging \
			or state == linphone.CallState.CallPaused \
			or state == linphone.CallState.CallStreamsRunning \
			or state == linphone.CallState.CallConnected \
			or state == linphone.CallState.CallIncomingReceived:
			body += "\nFrom: {address}".format(address=call.remote_address.as_string())
		if state == linphone.CallState.CallStreamsRunning \
			or state == linphone.CallState.CallConnected:
			direction_str = 'in'
			if call.dir == linphone.CallDir.CallOutgoing:
				direction_str = 'out'
			body += "\nDirection: {direction}\nDuration: {duration}".format(direction=direction_str, duration=call.duration)
		app.send_response(Response(Response.Ok, body))

class HelpCommand(Command):
	"""Show <command> help notice, if command is unspecified or inexistent show all commands."""
	def __init__(self):
		Command.__init__(self, "help", "help <command>")

	def exec_command(self, app, args):
		body = ''
		if args:
			command = [item for item in app.commands if item.name == args[0]]
			if command:
				body = command[0].help()
			else:
				app.send_response(Response(Response.Error, "Unknown command '{command}'.".format(command=args[0])))
				return
		else:
			for command in app.commands:
				body += command.proto + '\n'
		app.send_response(Response(Response.Ok, body))

class QuitCommand(Command):
	"""Quit the application."""
	def __init__(self):
		Command.__init__(self, "quit", "quit")

	def exec_command(self, app, args):
		app.quit()
		app.send_response(Response(Response.Ok))

class RegisterCommand(Command):
	"""Register the daemon to a SIP proxy. If one of the parameters <password>, <userid> and <realm> is not needed, send the string "NULL"."""
	def __init__(self):
		Command.__init__(self, "register", "register <identity> <proxy-address> [password] [userid] [realm] [contact-parameters]")
		self.add_example(CommandExample(
			"register sip:daemon-test@sip.linphone.org sip.linphone.org password bob linphone.org",
			"Status: Ok\n\nId: 1"
		))

	def exec_command(self, app, args):
		if len(args) >= 2:
			password = None
			userid = None
			realm = None
			contact_parameters = None
			identity = args[0]
			proxy = args[1]
			if len(args) > 2 and args[2] != "NULL":
				password = args[2]
			if len(args) > 3 and args[3] != "NULL":
				userid = args[3]
			if len(args) > 4 and args[4] != "NULL":
				realm = args[4]
			if len(args) > 5 and args[5] != "NULL":
				contact_parameters = args[5]
			proxy_cfg = app.core.create_proxy_config()
			if password is not None:
				addr = linphone.Address.new(identity)
				if addr is not None:
					info = linphone.AuthInfo.new(addr.username, userid, password, None, realm, None)
					app.core.add_auth_info(info)
					print(info)
			proxy_cfg.identity = identity
			proxy_cfg.server_addr = proxy
			proxy_cfg.register_enabled = True
			proxy_cfg.contact_parameters = contact_parameters
			app.core.add_proxy_config(proxy_cfg)
			id = app.update_proxy_id(proxy_cfg)
			app.send_response(Response(Response.Ok, "Id: " + str(id)))
		else:
			app.send_response(Response(Response.Error, "Missing/Incorrect parameter(s)."))

class RegisterStatusCommand(Command):
	"""Return status of a registration or of all registrations."""
	def __init__(self):
		Command.__init__(self, "register-status", "register-status <register_id|ALL>")
		self.add_example(CommandExample(
			"register-status 1",
			"Status: Ok\n\nId: 1\nState: LinphoneRegistrationOk"
		))
		self.add_example(CommandExample(
			"register-status ALL",
			"Status: Ok\n\nId: 1\nState: LinphoneRegistrationOk\n\nId: 2\nState: LinphoneRegistrationFailed"
		))
		self.add_example(CommandExample(
			"register-status 3",
			"Status: Error\nReason: No register with such id."
		))

	def exec_command(self, app, args):
		if len(args) == 0:
			app.send_response(Response(Response.Error, "Missing parameter."))
		else:
			id = args[0]
			if id == "ALL":
				response = RegisterStatusResponse()
				for id in app.proxy_ids_map:
					response.append(id, app.proxy_ids_map[id])
				app.send_response(response)
			else:
				proxy_cfg = app.find_proxy(id)
				if proxy_cfg is None:
					app.send_response(Response(Response.Error, "No register with such id."))
				else:
					app.send_response(RegisterStatusResponse().append(id, proxy_cfg))

class TerminateCommand(Command):
	"""Terminate the specified call or the current call if no id is given."""
	def __init__(self):
		Command.__init__(self, "terminate", "terminate [call id]")
		self.add_example(CommandExample(
			"terminate 2",
			"Status: Error\nReason: No call with such id."
		))
		self.add_example(CommandExample(
			"terminate 1",
			"Status: Ok\n"
		))
		self.add_example(CommandExample(
			"terminate",
			"Status: Ok\n"
		))
		self.add_example(CommandExample(
			"terminate",
			"Status: Error\nReason: No active call."
		))

	def exec_command(self, app, args):
		if len(args) >= 1:
			call = app.find_call(args[0])
			if call is None:
				app.send_response(Response(Response.Error, "No call with such id."))
				return
		else:
			call = app.core.current_call
			if call is None:
				app.send_response(Response(Response.Error, "No active call."))
				return
		app.core.terminate_call(call)
		app.send_response(Response(Response.Ok))


class Daemon:
	def __init__(self):
		self.quitting = False
		self._next_proxy_id = 1
		self.proxy_ids_map = {}
		self._next_call_id = 1
		self.call_ids_map = {}
		self.command_mutex = threading.Lock()
		self.command_executed_event = threading.Event()
		self.command_to_execute = None
		self.commands = [
			CallCommand(),
			CallPauseCommand(),
			CallResumeCommand(),
			CallStatusCommand(),
			HelpCommand(),
			QuitCommand(),
			RegisterCommand(),
			RegisterStatusCommand(),
			TerminateCommand()
		]

	def global_state_changed(self, core, state, message):
		logging.warning("[PYTHON] global_state_changed: " + str(state) + ", " + message)
		if state == linphone.GlobalState.GlobalOn:
			logging.warning("[PYTHON] core version: " + str(core.version))

	def registration_state_changed(self, core, proxy_cfg, state, message):
		logging.warning("[PYTHON] registration_state_changed: " + str(state) + ", " + message)

	def call_state_changed(self, core, call, state, message):
		logging.warning("[PYTHON] call_state_changed: " + str(state) + ", " + message)

	def send_response(self, response):
		print(response)

	def exec_command(self, command_line):
		splitted_command_line = command_line.split()
		name = splitted_command_line[0]
		args = splitted_command_line[1:]
		command = [item for item in self.commands if item.name == name]
		if command:
			command[0].exec_command(self, args)
		else:
			self.send_response(Response(Response.Error, "Unknown command."))

	def interact(self):
		command_line = raw_input('> ').strip()
		if command_line != '':
			self.command_mutex.acquire()
			self.command_to_execute = command_line
			self.command_mutex.release()
			self.command_executed_event.wait()
			self.command_executed_event.clear()

	def run(self, args):
		def command_read(daemon):
			while not daemon.quitting:
				daemon.interact()

		callbacks = {
			'global_state_changed':self.global_state_changed,
			'registration_state_changed':self.registration_state_changed,
			'call_state_changed':self.call_state_changed
		}

		# Create a linphone core and iterate every 20 ms
		self.core = linphone.Core.new(callbacks, args.config, args.factory_config)
		t = threading.Thread(target=command_read, kwargs={'daemon':self})
		t.start()
		while not self.quitting:
			self.command_mutex.acquire()
			command_line = self.command_to_execute
			if command_line is not None:
				self.exec_command(command_line)
				self.command_to_execute = None
				self.command_executed_event.set()
			self.command_mutex.release()
			self.core.iterate()
			time.sleep(0.02)
		t.join()

	def quit(self):
		self.quitting = True

	def update_proxy_id(self, proxy):
		id = self._next_proxy_id
		self.proxy_ids_map[str(id)] = proxy
		self._next_proxy_id += 1
		return id

	def find_proxy(self, id):
		if self.proxy_ids_map.has_key(id):
			return self.proxy_ids_map[id]
		return None

	def update_call_id(self, call):
		id = self._next_call_id
		self.call_ids_map[str(id)] = call
		self._next_call_id += 1
		return id

	def find_call(self, id):
		if self.call_ids_map.has_key(id):
			return self.call_ids_map[id]
		return None

def setup_log_colors():
	logging.addLevelName(logging.DEBUG, "\033[1;37m%s\033[1;0m" % logging.getLevelName(logging.DEBUG))
	logging.addLevelName(logging.INFO, "\033[1;36m%s\033[1;0m" % logging.getLevelName(logging.INFO))
	logging.addLevelName(logging.WARNING, "\033[1;31m%s\033[1;0m" % logging.getLevelName(logging.WARNING))
	logging.addLevelName(logging.ERROR, "\033[1;41m%s\033[1;0m" % logging.getLevelName(logging.ERROR))

def setup_log(log, trace):
	if log is None:
		setup_log_colors()
	format = "%(asctime)s.%(msecs)03d %(levelname)s: %(message)s"
	datefmt = "%H:%M:%S"
	if trace:
		level = logging.DEBUG
	else:
		level = logging.INFO
	logging.basicConfig(filename=log, level=level, format=format, datefmt=datefmt)

# Define the linphone module log handler
def log_handler(level, msg):
	method = getattr(logging, level)
	if not msg.strip().startswith('[PYLINPHONE]'):
		msg = '[CORE] ' + msg
	method(msg)

def main(argv = None):
	if argv is None:
		argv = sys.argv
	argparser = argparse.ArgumentParser(description="Linphone console interface in Python.")
	argparser.add_argument('--config', default=None, help="Path to the linphonerc configuration file to use.")
	argparser.add_argument('--factory_config', default=None, help="Path to the linphonerc factory configuration file to use.")
	argparser.add_argument('--log', default=None, help="Path to the file used for logging (default is the standard output).")
	argparser.add_argument('--trace', action='store_true', help="Output linphone Python module tracing logs (for debug purposes).")
	args = argparser.parse_args()
	setup_log(args.log, args.trace)
	linphone.set_log_handler(log_handler)
	d = Daemon()
	d.run(args)

if __name__ == "__main__":
	sys.exit(main())
