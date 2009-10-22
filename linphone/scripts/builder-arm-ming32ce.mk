host:=armv4-mingw32ce
config_site:=cegcc-config.site
library_mode:=""
libosip2_version:=svn
libeXosip2_version:=svn
linphone_configure_controls?= 	--disable-video \
								--with-readline=none  \
								--enable-gtk_ui=no \
								--enable-ssl-hmac=no \
								SPEEX_CFLAGS="-I$(prefix)/include" \
        						SPEEX_LIBS="-L$(prefix)/lib -lspeex " 

include builder-generic.mk


build-linphone:	build-osip2 build-eXosip2  build-speex $(LINPHONE_SRC_DIR)/Makefile 
	 cd $(LINPHONE_SRC_DIR) && make newdate && make  && make install 

clean-linphone: clean-osip2 clean-eXosip2 clean-speex
	 cd  $(LINPHONE_SRC_DIR) && make clean

veryclean-linphone: clean-linphone veryclean-osip2 veryclean-eXosip2 veryclean-speex
	 cd $(LINPHONE_SRC_DIR) && make distclean
	 cd $(LINPHONE_SRC_DIR) && rm configure

clean-makefile-linphone: clean-makefile-osip2 clean-makefile-eXosip2  
	 cd $(LINPHONE_SRC_DIR) && rm Makefile && rm oRTP/Makefile && rm mediastreamer2/Makefile
	

get_dependencies: get_osip2_svn get_eXosip2_svn get_speex_src


