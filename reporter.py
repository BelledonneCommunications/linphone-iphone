#!/usr/bin/python

import fileinput
import json
import sys
import codecs

logfile = 'tests.log'

f = codecs.open(logfile, 'w', 'utf8')

file_events = ['begin-test', 'test-output']

sys.stdout.write("Log file : " + f.name + '\n' )

for line in fileinput.input():
	obj = json.loads(line)

	e = obj['event']

	if e in file_events:
		if e == 'begin-test':
			f.write(obj['test'] )
		elif e == 'test-output':
			f.write(obj['output'] )
	if e == 'end-test':
		success = "OK\n" if obj['succeeded'] else "FAILED\n"
		sys.stdout.write(obj['test'] + " -> " + success )

	elif 'message' in obj and 'begin' in obj['event']:
		sys.stdout.write( obj['message'] + '\n' )
