host?=armv6-apple-darwin
config_site?=iphone-config.site
library_mode?= --disable-shared 
libosip2_version?=3.3.0
libeXosip2_version=3.3.0
libspeex_version=1.2rc1
libgsm_version=1.0.13
ifeq ($(target_arch),armv6) 
	SPEEX_CONFIGURE_OPTION := --enable-fixed-point
	#SPEEX_CONFIGURE_OPTION := --enable-arm5e-asm --enable-fixed-point
endif

LINPHONE_SRC_DIR=$(shell pwd)/../
prefix=$(LINPHONE_SRC_DIR)/liblinphone-sdk/$(host)

all: build-linphone
clean-makefile: clean-makefile-linphone
clean: clean-linphone

delivery:
	cd $(LINPHONE_SRC_DIR) \
	&& zip  -r   linphone-iphone.zip \
		liblinphone-sdk xcode/linphone/ pixmaps/red.png \
		pixmaps/green.png share/ringback.wav \
		share/rings/oldphone-mono.wav \
		-x xcode/linphone/build/Debug-iphoneos/\* \
		 xcode/linphone/build/Debug-iphonesimulator/\* \
		 xcode/linphone/build/Release-iphoneos/\* \
		 xcode/linphone/build/Release-iphonesimulator/\* \
		 xcode/linphone/build/linphone.build/\*


$(LINPHONE_SRC_DIR)/configure:
	 cd $(LINPHONE_SRC_DIR) && ./autogen.sh

$(LINPHONE_SRC_DIR)/Makefile: $(LINPHONE_SRC_DIR)/configure
	 cd $(LINPHONE_SRC_DIR) && \
	 PKG_CONFIG_PATH=$(prefix)/ CONFIG_SITE=$(LINPHONE_SRC_DIR)/scripts/$(config_site) \
	./configure -prefix=$(prefix) --host=$(host) ${library_mode} \
	--disable-video --with-readline=none  --enable-gtk_ui=no --enable-ssl-hmac=no --with-osip=$(prefix) \
	--enable-nonstandard-gsm --with-gsm=$(prefix) \
	SPEEX_CFLAGS="-I$(prefix)/include" \
        SPEEX_LIBS="-L$(prefix)/lib -lspeex "

build-linphone:	build-osip2 build-eXosip2 build-speex build-libgsm $(LINPHONE_SRC_DIR)/Makefile 
	 cd $(LINPHONE_SRC_DIR) && make newdate && make  && make install 

clean-linphone: clean-osip2 clean-eXosip2 clean-speex clean-libgsm clean-libgsm
	 cd  $(LINPHONE_SRC_DIR) && make clean

veryclean-linphone: clean-linphone veryclean-osip2 veryclean-eXosip2 veryclean-speex veryclean-libgsm
	 cd $(LINPHONE_SRC_DIR) && make distclean
	 cd $(LINPHONE_SRC_DIR) && rm configure

clean-makefile-linphone: clean-makefile-osip2 clean-makefile-eXosip2 clean-makefile-speex 
	 cd $(LINPHONE_SRC_DIR) && rm Makefile && rm oRTP/Makefile && rm mediastreamer2/Makefile
	

get_dependencies: get_osip2_src get_eXosip2_src get_speex_src get_libgsm_src

#osip2

get_osip2_svn: 
	 cd $(LINPHONE_SRC_DIR)/ \
	&& svn co svn://svn.sv.gnu.org/osip/trunk/osip libosip2-$(libosip2_version)
get_osip2_src: 
	 cd $(LINPHONE_SRC_DIR)/ \
	&& rm -f libosip2-$(libosip2_version).tar.gz \
	&& wget http://ftp.gnu.org/gnu/osip/libosip2-$(libosip2_version).tar.gz \
	&& tar xvzf libosip2-$(libosip2_version).tar.gz \
	&& rm -rf libosip2-$(libosip2_version).tar.gz \
	&& cd  libosip2-$(libosip2_version) \
	&& patch -p1 <  $(LINPHONE_SRC_DIR)/patches/libosip2-iphone.patch

$(LINPHONE_SRC_DIR)/libosip2-$(libosip2_version)/configure:
	 cd $(LINPHONE_SRC_DIR)/libosip2-$(libosip2_version) && ./autogen.sh

$(LINPHONE_SRC_DIR)/libosip2-$(libosip2_version)/Makefile: $(LINPHONE_SRC_DIR)/libosip2-$(libosip2_version)/configure
	 cd $(LINPHONE_SRC_DIR)/libosip2-$(libosip2_version)/ \
	&& CONFIG_SITE=$(LINPHONE_SRC_DIR)/scripts/$(config_site) \
	./configure -prefix=$(prefix) --host=$(host) ${library_mode}  

build-osip2: $(LINPHONE_SRC_DIR)/libosip2-$(libosip2_version)/Makefile
	 cd $(LINPHONE_SRC_DIR)/libosip2-$(libosip2_version) && make && make install

clean-osip2:
	 cd  $(LINPHONE_SRC_DIR)/libosip2-$(libosip2_version) && make clean

veryclean-osip2:
	 cd $(LINPHONE_SRC_DIR)/libosip2-$(libosip2_version) && make distclean
	 cd $(LINPHONE_SRC_DIR)/libosip2-$(libosip2_version) && rm configure

clean-makefile-osip2:
	 cd $(LINPHONE_SRC_DIR)/libosip2-$(libosip2_version) && rm Makefile
#eXosip
	
get_eXosip2_src:
	 cd $(LINPHONE_SRC_DIR)/ \
	&& rm -f libeXosip2-$(libeXosip2_version).tar.gz \
	&& wget http://nongnu.askapache.com/exosip/libeXosip2-$(libeXosip2_version).tar.gz \
	&& tar xvzf libeXosip2-$(libeXosip2_version).tar.gz \
	&& rm -rf libeXosip2-$(libeXosip2_version).tar.gz

$(LINPHONE_SRC_DIR)/libeXosip2-$(libeXosip2_version)/Makefile: 
	 cd $(LINPHONE_SRC_DIR)/libeXosip2-$(libeXosip2_version)/\
	&& CONFIG_SITE=$(LINPHONE_SRC_DIR)/scripts/$(config_site) \
	./configure -prefix=$(prefix) --host=$(host) ${library_mode} --enable-pthread

build-eXosip2: $(LINPHONE_SRC_DIR)/libeXosip2-$(libeXosip2_version)/Makefile
	 cd $(LINPHONE_SRC_DIR)/libeXosip2-$(libeXosip2_version)  && make  && make install

clean-eXosip2:
	 cd  $(LINPHONE_SRC_DIR)/libeXosip2-$(libeXosip2_version)  && make clean

veryclean-eXosip2:
	 cd $(LINPHONE_SRC_DIR)/libeXosip2-$(libeXosip2_version) && make distclean

clean-makefile-eXosip2:
	 cd $(LINPHONE_SRC_DIR)/libeXosip2-$(libeXosip2_version) && rm Makefile

#speex

get_speex_src:
	 cd $(LINPHONE_SRC_DIR)/\
	&& rm -f speex-$(libspeex_version).tar.gz \
	&& wget http://downloads.xiph.org/releases/speex/speex-$(libspeex_version).tar.gz \
	&& tar xvzf speex-$(libspeex_version).tar.gz \
	&& rm -f speex-$(libspeex_version).tar.gz

$(LINPHONE_SRC_DIR)/speex-$(libspeex_version)/Makefile: 
	 cd $(LINPHONE_SRC_DIR)/speex-$(libspeex_version)/\
	&& CONFIG_SITE=$(LINPHONE_SRC_DIR)/scripts/$(config_site) \
	./configure -prefix=$(prefix) --host=$(host) ${library_mode} --disable-oggtest $(SPEEX_CONFIGURE_OPTION)

build-speex: $(LINPHONE_SRC_DIR)/speex-$(libspeex_version)/Makefile
	 cd $(LINPHONE_SRC_DIR)/speex-$(libspeex_version)/libspeex && make  && make install
	 cd $(LINPHONE_SRC_DIR)/speex-$(libspeex_version)/include  && make &&  make install

clean-speex:
	 cd  $(LINPHONE_SRC_DIR)/speex-$(libspeex_version)  && make clean

veryclean-speex:
	 cd $(LINPHONE_SRC_DIR)/speex-$(libspeex_version) && make distclean

clean-makefile-speex:
	 cd $(LINPHONE_SRC_DIR)/speex-$(libspeex_version) && rm Makefile

#GSM

get_libgsm_src:
	- cd $(LINPHONE_SRC_DIR)/\
	&& rm -f gsm-$(libgsm_version).tar.gz \
	&& wget http://user.cs.tu-berlin.de/~jutta/gsm/gsm-$(libgsm_version).tar.gz \
	&& tar xvzf  gsm-$(libgsm_version).tar.gz \
	&& rm -f  gsm-$(libgsm_version).tar.gz
build-libgsm:
	cd $(LINPHONE_SRC_DIR)/gsm-1.0-pl13\
	&& mkdir -p $(prefix)/include/gsm \
	&& host_alias=$(host)  . ../scripts/$(config_site) \
	&&  make CC="$${CC}" INSTALL_ROOT=$(prefix)  GSM_INSTALL_INC=$(prefix)/include/gsm  install

clean-libgsm:
	 cd $(LINPHONE_SRC_DIR)/gsm-1.0-pl13 \
	&& make clean

veryclean-libgsm: clean-libgsm
	 cd $(LINPHONE_SRC_DIR)/gsm-1.0-pl13 \
	&& make uninstall
