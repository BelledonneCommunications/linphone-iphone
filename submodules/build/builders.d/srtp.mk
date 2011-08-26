srtp_version?="1.4.2"
srtp_url?="http://srtp.sourceforge.net/srtp-$(srtp_version).tgz"


$(BUILDER_SRC_DIR)/$(srtp_dir)/configure:
	cd $(BUILDER_SRC_DIR)/externals \
	&& wget $(stp_url) \
	&& tar zxvf srtp-$(srtp_version).tgz

$(BUILDER_BUILD_DIR)/$(srtp_dir)/Makefile: $(BUILDER_SRC_DIR)/$(srtp_dir)/configure
	mkdir -p $(BUILDER_BUILD_DIR)/$(srtp_dir)
	cd $(BUILDER_BUILD_DIR)/$(srtp_dir)/\
	&& CONFIG_SITE=$(BUILDER_SRC_DIR)/build/$(config_site) \
	$(BUILDER_SRC_DIR)/$(srtp_dir)/configure -prefix=$(prefix) --host=$(host) ${library_mode}

build-srtp: $(BUILDER_BUILD_DIR)/$(srtp_dir)/Makefile
	cp -rf $(BUILDER_SRC_DIR)/$(srtp_dir)/include $(BUILDER_BUILD_DIR)/$(srtp_dir)
	cp -rf $(BUILDER_SRC_DIR)/$(srtp_dir)/crypto/include $(BUILDER_BUILD_DIR)/$(srtp_dir)
	-cd $(BUILDER_BUILD_DIR)/$(srtp_dir) && make uninstall
	cd $(BUILDER_BUILD_DIR)/$(srtp_dir) && make libsrtp.a && make install

clean-srtp:
	cd $(BUILDER_BUILD_DIR)/$(srtp_dir)  && make clean

veryclean-srtp:
	-cd $(BUILDER_BUILD_DIR)/$(srtp_dir) && make distclean

clean-makefile-srtp:
	cd $(BUILDER_BUILD_DIR)/$(srtp_dir) && rm -f Makefile

