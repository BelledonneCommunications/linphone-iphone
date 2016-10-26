This is Linphone, a free (GPL) video softphone based on the SIP protocol.

# Warning

Unless you exactly know what you are doing, you should take at look at [linphone-desktop](https://github.com/BelledonneCommunications/linphone-desktop).

# Otherwise…

## Building Linphone

- Install build time dependencies
	- libtool
	- intltool

- you need at least:
 	- belle-sip>=1.3.0
	- speex>=1.2.0 (including libspeexdsp part)
	- libxml2
	- bctoolbox

	+ if you want the gtk/glade interface:
		- libgtk >=2.16.0
	+ if you want video support:
		- libvpx (VP8 codec)
		- libavcodec (ffmpeg)
		- libswscale (part of ffmpeg too) for better scaling performance
		- libxv (x11 video extension)
		- libgl1-mesa (OpenGL API -- GLX development files)
		- libglew (OpenGL Extension Wrangler library)
		- libv4l (Video for linux)
		- libx11 (x11)
		- theora (optional)
	+ gsm codec (gsm source package or libgsm-dev or gsm-devel) (optional)
	+ libreadline (optional: for convenient command line in linphonec)
	+ libsqlite3 (optional : for a local history of chat messages)
	+ if you want uPnP support (optional):
		- libupnp (version 1.6 branch (not patched with 18-url-upnpstrings.patch))

	Here is the command line to get these dependencies installed for Ubuntu && Debian

	$ sudo apt-get install libtool intltool libgtk2.0-dev libspeexdsp-dev \
libavcodec-dev libswscale-dev libx11-dev libxv-dev libgl1-mesa-dev \
libglew1.6-dev libv4l-dev libxml2-dev

	+ for optional library
	$ sudo apt-get install libreadline-dev libgsm1-dev libtheora-dev \
libsqlite3-dev libupnp4-dev libsrtp-dev

	+ Install zrtp (optional), for unbreakable call encryption
		$ git clone git://git.linphone.org/bzrtp.git
		$ cd bzrtp && ./autogen.sh && ./configure && make
		$ sudo make install

- Compile linphone

 $ ./autogen.sh
 $ ./configure
 $ make && sudo make install
 $ sudo ldconfig



For windows compilation see README.mingw.
For macOS X, see README.macos


## Notes for developers

Here is a short description of the content of the source tree.

- oRTP/ is a poweful implementation of the RTP protocol. See the oRTP/README for more details.
	It is used by mediastreamer2 to send and receive streams to the network.

- mediastreamer2/ is one of the important part of linphone. It is a framework for audio
	and video processing. It contains several objects for grabing audio and video and outputing
	it (through rtp, to file).
	It contains also codec objects to compress audio and video streams.
  The mediastream.h files contain routines to easyly setup audio streams.

- coreapi/ is the central point of linphone, which handles relationship between sip signalisation and media
  streaming. It contains an easy to use api to create a sip phone.

- gtk/	is the directory that contains the gui frontend of linphone. It uses all libraries descibed above.

- console/
	* linphonec.c is the main file for the console version of linphone.
	* sipomatic.c / sipomatic.h contains the code for sipomatic, the test program that auto-answer to linphone calls.
	* shell.c (program name: linphonecsh) is a small utilities to send interactive commands to a running linphonec daemon.

- share/ contains translation, documentation, rings and hello sound files.

