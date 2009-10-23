############################################################################
#  builder-arm-mingw32ce.mk
#  Copyright (C) 2009  Belledonne Communications,Grenoble France
#
############################################################################
 #
 # This program is free software; you can redistribute it and/or
 # modify it under the terms of the GNU General Public License
 # as published by the Free Software Foundation; either version 2
 # of the License, or (at your option) any later version.
 #
 # This program is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 # GNU General Public License for more details.
 #
 # You should have received a copy of the GNU General Public License
 # along with this program; if not, write to the Free Software
 # Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 #
############################################################################
host:=armv4-mingw32ce
config_site:=mingw32ce-config.site
library_mode:= --disable-shared --enable-static
libosip2_version:=797
libeXosip2_version:=1047
linphone_configure_controls?= 	--disable-video \
								--with-readline=none  \
								--enable-gtk_ui=no \
								--enable-console_ui=no \
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


