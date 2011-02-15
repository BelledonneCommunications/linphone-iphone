prefix=/opt/linphone


MSX264_SRC_DIR=$(LINPHONE_SRC_DIR)/mediastreamer2/plugins/msx264
BUDDYLOOKUP_SRC_DIR=$(LINPHONE_SRC_DIR)/coreapi/plugins/buddylookup
LOCALDIR=$(shell pwd)
WORKDIR=$(LOCALDIR)/build
LINPHONE_ZIP=$(WORKDIR)/linphone.zip
BUDDYLOOKUP_ZIP=$(WORKDIR)/buddylookup.zip
MSX264_ZIP=$(WORKDIR)/msx264.zip
INSTALL_ROOT=$(WORKDIR)/root
FILELIST=$(WORKDIR)/linphone-bundle.filelist

LINPHONE_VERSION=strings $(INSTALL_ROOT)/bin/linphone.exe |grep linphone_ident | sed 's/linphone_ident_string=//'

$(WORKDIR):
	mkdir -p $(WORKDIR)

$(INSTALL_ROOT): $(WORKDIR)
	mkdir -p $(INSTALL_ROOT)



#Inno Setup 5 compiler
ISCC="c:\Program Files\Inno setup 5\ISCC.exe"

$(LINPHONE_SRC_DIR)/configure:
	cd $(LINPHONE_SRC_DIR) && ./autogen.sh

$(LINPHONE_SRC_DIR)/Makefile: $(LINPHONE_SRC_DIR)/configure
	cd $(LINPHONE_SRC_DIR) && \
	./configure --prefix=$(prefix) --enable-shared --disable-static $(LINPHONE_CONFIGURE_EXTRA_OPTIONS)

build-linphone:	$(LINPHONE_SRC_DIR)/Makefile
	cd $(LINPHONE_SRC_DIR) && make newdate && make $(LINPHONE_MAKE_OPTS) && make install $(LINPHONE_MAKE_OPTS)

$(LINPHONE_ZIP):	build-linphone $(WORKDIR)
	cd $(LINPHONE_SRC_DIR) && make zip ZIPFILE=$(LINPHONE_ZIP) $(LINPHONE_MAKE_OPTS)

install-linphone: $(LINPHONE_ZIP) $(INSTALL_ROOT)
	cd $(INSTALL_ROOT) && unzip -o $(LINPHONE_ZIP)

clean-linphone:
	- cd  $(LINPHONE_SRC_DIR) && make clean

veryclean-linphone:
	- cd $(LINPHONE_SRC_DIR) && make distclean
	- cd $(LINPHONE_SRC_DIR) && rm configure

##### msx264 rules

$(MSX264_SRC_DIR)/configure:
	cd $(MSX264_SRC_DIR) && ./autogen.sh


$(MSX264_SRC_DIR)/Makefile:	$(MSX264_SRC_DIR)/configure
	cd $(MSX264_SRC_DIR) && \
	PKG_CONFIG_PATH=$(prefix)/lib/pkgconfig ./configure --prefix=$(prefix) --enable-shared --disable-static --enable-hacked-x264


build-msx264:	build-linphone $(MSX264_SRC_DIR)/Makefile
	cd $(MSX264_SRC_DIR) && PKG_CONFIG_PATH=$(prefix)/lib/pkgconfig make

$(MSX264_ZIP):	build-msx264
	cd $(MSX264_SRC_DIR) && make zip ZIPFILE=$(MSX264_ZIP)

install-msx264:	$(MSX264_ZIP) $(INSTALL_ROOT)
	cd $(INSTALL_ROOT) && unzip -o $(MSX264_ZIP)

clean-msx264:
	- cd  $(MSX264_SRC_DIR) && make clean

veryclean-msx264:
	- cd $(MSX264_SRC_DIR) && make distclean
	- cd $(MSX264_SRC_DIR) && rm configure

###### buddylookup rules

$(BUDDYLOOKUP_SRC_DIR)/configure:
	cd $(BUDDYLOOKUP_SRC_DIR) && ./autogen.sh


$(BUDDYLOOKUP_SRC_DIR)/Makefile:	$(BUDDYLOOKUP_SRC_DIR)/configure
	cd $(BUDDYLOOKUP_SRC_DIR) && \
	PKG_CONFIG_PATH=$(prefix)/lib/pkgconfig ./configure --prefix=$(prefix) --enable-shared --disable-static


build-buddylookup:	build-linphone $(BUDDYLOOKUP_SRC_DIR)/Makefile
	cd $(BUDDYLOOKUP_SRC_DIR) && PKG_CONFIG_PATH=$(prefix)/lib/pkgconfig make

$(BUDDYLOOKUP_ZIP):	build-buddylookup
	cd $(BUDDYLOOKUP_SRC_DIR) && make zip ZIPFILE=$(BUDDYLOOKUP_ZIP)

install-buddylookup:	$(BUDDYLOOKUP_ZIP) $(INSTALL_ROOT)
	cd $(INSTALL_ROOT) && unzip -o $(BUDDYLOOKUP_ZIP)

clean-buddylookup:
	- cd  $(BUDDYLOOKUP_SRC_DIR) && make clean

veryclean-buddylookup:
	- cd $(BUDDYLOOKUP_SRC_DIR) && make distclean
	- cd $(BUDDYLOOKUP_SRC_DIR) && rm configure


$(FILELIST): 
	cd $(INSTALL_ROOT) && \
	rm -f $(FILELIST) && \
	for file in `find` ; do \
		if ! test -d $$file ; then \
			echo "Source: $$file; Destdir: {app}\\`dirname $$file`; Flags: ignoreversion" \
			>> $(FILELIST) ;\
		fi \
	done

clean-install:
	rm -rf $(INSTALL_ROOT)
