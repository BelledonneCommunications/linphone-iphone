Linphone
========

This is Linphone, a free (GPL) video softphone based on the SIP protocol.

**WARNING:** Unless you exactly know what you are doing, you should take at look at *linphone-desktop[1]*.


Building Linphone
-----------------

### Required dependencies

* *BcToolbox[2]*: portability layer
* *BelleSIP[3]*: SIP stack
* *Mediastreamer2[4]*: multimedia engine
* libxml2
* zlib
* libsqlite3: user data storage (disablable)
* libnotify: system notification (GNU/Linux only;disablable)
* libgtk2: graphical interface (disablable)
* gettext and libintl: internationalization support (disablable)


### Opitonal dependencies

* *Belcard[5]*: VCard support
* gtkmacintegration: integration with MacOSX menu


### Build instructions

	cmake . -DCMAKE_INSTALL_PREFIX=<prefix> -DCMAKE_PREFIX_PATH=<search_prefixes>
	
	make
	make install


### Supported build opitons

* `CMAKE_INSTALL_PREFIX=<string>` : install prefix
* `CMAKE_PREFIX_PATH=<string>`    : column-separated list of prefixes where to search for dependencies
* `ENABLE_SHARED=NO`              : do not build the shared library
* `ENABLE_STATIC=NO`              : do not build the static library
* `ENABLE_STRICT=NO`              : build without strict compilation flags (-Wall -Werror)
* `ENABLE_DOC=NO`                 : do not generate the reference documentation of liblinphone
* `ENABLE_GTK_UI=NO`              : do not build the GTK user interface
* `ENABLE_UNIT_TESTS=NO`          : do not build testing binaries
* `ENABLE_VCARD=NO`               : disable VCard support
* `ENABLE_SQLITE_STORAGE=NO`      : disable SQlite user data storage (message, history, contacts list)
* `ENABLE_TOOLS=NO`               : do not build tool binaries
* `ENABLE_NLS=NO`                 : disable internationalization
* `ENABLE_ASSISTANT=NO`           : disable account creation wizard

### Note for packagers

Our CMake scripts may automatically add some paths into research paths of generated binaries.
To ensure that the installed binaries are striped of any rpath, use `-DCMAKE_SKIP_INSTALL_RPATH=ON`
while you invoke cmake.



Notes for developers
--------------------

Here is a short description of the content of the source tree.

- **oRTP/** is a poweful implementation of the RTP protocol. See the oRTP/README for more details.
	    It is used by mediastreamer2 to send and receive streams to the network.

- **mediastreamer2/** is one of the important part of linphone. It is a framework for audio
	              and video processing. It contains several objects for grabing audio and video and outputing
	              it (through rtp, to file).
	              It contains also codec objects to compress audio and video streams.
                      The mediastream.h files contain routines to easyly setup audio streams.

- **coreapi/** is the central point of linphone, which handles relationship between sip signalisation and media
               streaming. It contains an easy to use api to create a sip phone.

- **gtk/** is the directory that contains the gui frontend of linphone. It uses all libraries descibed above.

- **console/**
	* linphonec.c is the main file for the console version of linphone.
	* sipomatic.c / sipomatic.h contains the code for sipomatic, the test program that auto-answer to linphone calls.
	* shell.c (program name: linphonecsh) is a small utilities to send interactive commands to a running linphonec daemon.

- **share/** contains translation, documentation, rings and hello sound files.


------------------------------


- [1] linphone-desktop: git://git.linphone.org/linphone-desktop.git
- [2] bctoolbox: git://git.linphone.org/bctoolbox.git *or* <https://www.linphone.org/releases/sources/bctoolbox>
- [3] belle-sip: git://git.linphone.org/belle-sip.git *or* <https://www.linphone.org/releases/sources/belle-sip>
- [4] mediastreamer2: git://git.linphone.org/mediastreamer2.git *or* <https://www.linphone.org/releases/sources/mediastreamer>
- [5] belcard: git://git.linphone.org/belcard.git *or* <https://www.linphone.org/releases/sources/belcard>
