liblinphone
========

This is liblinphone, a free (GPL) video voip library based on the SIP protocol.

This library is used by Linphone. It's source code is available at *linphone-desktop[1]*.


Building liblinphone
-----------------

### Required dependencies

* *BcToolbox[2]*: portability layer
* *BelleSIP[3]*: SIP stack
* *Mediastreamer2[4]*: multimedia engine
* *Belcard[5]*: VCard support
* libxml2
* zlib
* libsqlite3: user data storage (disablable)
* gettext and libintl: internationalization support (disablable)


### Opitonal dependencies

* *Bzrtp[6]*: zrtp stack used for Linphone Instant Messaging Encryption


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
* `ENABLE_UNIT_TESTS=NO`          : do not build testing binaries
* `ENABLE_VCARD=NO`               : disable VCard support
* `ENABLE_SQLITE_STORAGE=NO`      : disable SQlite user data storage (message, history, contacts list)
* `ENABLE_TOOLS=NO`               : do not build tool binaries
* `ENABLE_NLS=NO`                 : disable internationalization
* `ENABLE_LIME=YES`               : disable Linphone Instant Messaging Encryption

### Note for packagers

Our CMake scripts may automatically add some paths into research paths of generated binaries.
To ensure that the installed binaries are striped of any rpath, use `-DCMAKE_SKIP_INSTALL_RPATH=ON`
while you invoke cmake.

Rpm packaging
liblinphone can be generated with cmake3 using the following command:
mkdir WORK
cd WORK
cmake3 ../
make package_source
rpmbuild -ta --clean --rmsource --rmspec liblinphone-<version>-<release>.tar.gz


Notes for developers
--------------------

Here is a short description of the content of the source tree.


- **coreapi/** is the central point of linphone, which handles relationship between sip signalisation and media
               streaming. It contains an easy to use api to create a sip phone.


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
- [5] bzrtp: git://git.linphone.org/bzrtp.git *or* <https://www.linphone.org/releases/sources/bzrtp>
