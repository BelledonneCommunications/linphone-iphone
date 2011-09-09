srtp_version?=1.4.4
#srtp_url?=http://srtp.sourceforge.net/srtp-$(srtp_version).tgz
srtp_url=http://sourceforge.net/projects/srtp/files/srtp/$(srtp_version)/srtp-$(srtp_version).tgz/download
srtp_tgz_file=srtp-$(srtp_version).tgz

$(BUILDER_SRC_DIR)/externals/$(srtp_tgz_file):
	cd $(BUILDER_SRC_DIR)/externals \
        && wget $(srtp_url) -O $(srtp_tgz_file) 

$(BUILDER_SRC_DIR)/$(srtp_dir)/configure: $(BUILDER_SRC_DIR)/externals/$(srtp_tgz_file)
	cd $(BUILDER_SRC_DIR)/externals \
	&& tar zxvf $(srtp_tgz_file) \
	&& cd srtp && patch -p0 < $(BUILDER_SRC_DIR)/build/builders.d/srtp.patch

$(BUILDER_BUILD_DIR)/$(srtp_dir)/Makefile: $(BUILDER_SRC_DIR)/$(srtp_dir)/configure
	mkdir -p $(BUILDER_BUILD_DIR)/$(srtp_dir)
	cd $(BUILDER_BUILD_DIR)/$(srtp_dir)/\
	&& CONFIG_SITE=$(BUILDER_SRC_DIR)/build/$(config_site) \
	$(BUILDER_SRC_DIR)/$(srtp_dir)/configure -prefix=$(prefix) --host=$(host) ${library_mode}

build-srtp: $(BUILDER_BUILD_DIR)/$(srtp_dir)/Makefile
	cp -rf $(BUILDER_SRC_DIR)/$(srtp_dir)/include $(BUILDER_BUILD_DIR)/$(srtp_dir)
	cp -rf $(BUILDER_SRC_DIR)/$(srtp_dir)/crypto/include $(BUILDER_BUILD_DIR)/$(srtp_dir)
	-cd $(BUILDER_BUILD_DIR)/$(srtp_dir) && make uninstall && make clean
	cd $(BUILDER_BUILD_DIR)/$(srtp_dir) && make libsrtp.a && make install

clean-srtp:
	cd $(BUILDER_BUILD_DIR)/$(srtp_dir)  && make clean

veryclean-srtp:
	-cd $(BUILDER_BUILD_DIR)/$(srtp_dir) && make distclean

clean-makefile-srtp:
	cd $(BUILDER_BUILD_DIR)/$(srtp_dir) && rm -f Makefile

