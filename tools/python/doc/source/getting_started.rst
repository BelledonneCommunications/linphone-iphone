Getting started
===============

Installing the Python module
----------------------------

You can install prebuilt packages of Linphone for Python. You will find the
releases at https://pypi.python.org/pypi/linphone. This includes only packages
for the Windows platform right now. The easiest way to install is to use pip,
eg.:

.. code-block:: none

   > pip install linphone --pre

You can also find nightly-built packages for Windows, Mac OS X and Linux at
https://www.linphone.org/snapshots/linphone-python/.

Otherwise you can compile the Python module. To do so get the build system code
using the command:

.. code-block:: none

   > git clone git://git.linphone.org/linphone-cmake-builder.git

Then follow the instructions in the *README.python* file.

Running some code
-----------------

Here is a sample source code using PyQt4 that enables you to register on a SIP
server in just a few lines of code. This is a very basic example, but it shows
how to create a linphone.Core object that is the main object of Linphone. From
there, you can use the API reference below to use more advanced feature and
perform some SIP calls, use text messaging and more...

.. literalinclude:: pyqt_linphone_example.py

In the Linphone Python module package you installed previously you will also
find some unit tests that you can run. These unit tests will be located in the
*site-packages/linphone/unittests/* directory of Python installation where you
installed the Linphone Python module package. To run these unit tests, follow
the instructions contained in the README.txt file of this *unittests/*
directory.
