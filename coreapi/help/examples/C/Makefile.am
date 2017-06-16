if ENABLE_TUTORIALS

noinst_PROGRAMS=helloworld registration buddy_status chatroom notify filetransfer realtimetext_sender realtimetext_receiver

helloworld_SOURCES=helloworld.c
LINPHONE_TUTOS=$(helloworld_SOURCES)

helloworld_LDADD=$(top_builddir)/coreapi/liblinphone.la \
				$(MEDIASTREAMER_LIBS) \
				$(ORTP_LIBS) \
				$(BELLESIP_LIBS)

registration_SOURCES=registration.c
LINPHONE_TUTOS+=$(registration_SOURCES)

registration_LDADD=$(helloworld_LDADD)

buddy_status_SOURCES=buddy_status.c
LINPHONE_TUTOS+=$(buddy_status_SOURCES)

buddy_status_LDADD=$(helloworld_LDADD)

chatroom_SOURCES=chatroom.c
LINPHONE_TUTOS+=$(chatroom_SOURCES)

chatroom_LDADD=$(helloworld_LDADD)

notify_SOURCES=notify.c
LINPHONE_TUTOS+=$(notify_SOURCES)

notify_LDADD=$(helloworld_LDADD)

filetransfer_SOURCES=filetransfer.c
LINPHONE_TUTOS+=$(filetransfer_SOURCES)

filetransfer_LDADD=$(helloworld_LDADD)

realtimetext_sender_SOURCES=realtimetext_sender.c
LINPHONE_TUTOS+=$(realtimetext_sender_SOURCES)

realtimetext_sender_LDADD=$(helloworld_LDADD)

realtimetext_receiver_SOURCES=realtimetext_receiver.c
LINPHONE_TUTOS+=$(realtimetext_receiver_SOURCES)

realtimetext_receiver_LDADD=$(helloworld_LDADD)

AM_CFLAGS=\
	-I$(top_srcdir)/include \
	$(STRICT_OPTIONS) \
	$(STRICT_OPTIONS_CC) \
	$(ORTP_CFLAGS) \
	$(MEDIASTREAMER_CFLAGS) \
	-DENABLE_TRACE 	\
	-DLOG_DOMAIN=\"LinphoneCore\" \
	$(IPV6_CFLAGS) \
	-DORTP_INET6 \
	$(VIDEO_CFLAGS) \
	$(BELLESIP_CFLAGS)


tutodir=$(datadir)/tutorials/linphone

tuto_DATA=$(LINPHONE_TUTOS)
endif

