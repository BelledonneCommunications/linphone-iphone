***************************************
** Linphone Python module unit tests **
***************************************

To run these unit tests, you need to have installed the Python module for Linphone
and to have install the nose unit tests framework.
Then use this command to run the tests:
	nosetests -v --nologcapture

The logs from the tests are put in some .log files.

A single test file can be run by specifying it at the command line. For example,
to run only the message unit tests use:
	nosetests -v --nologcapture test_message.py
