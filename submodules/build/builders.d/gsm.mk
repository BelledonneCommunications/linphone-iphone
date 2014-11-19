
#GSM

gsm_dir?=externals/gsm


build-libgsm:
	cp -rf $(BUILDER_SRC_DIR)/$(gsm_dir) $(BUILDER_BUILD_DIR)/$(gsm_dir)
	rm -rf $(BUILDER_BUILD_DIR)/$(gsm_dir)/gsm/.git
	rm -f $(prefix)/lib/libgsm.a
	rm -rf $(prefix)/include/gsm
	cd $(BUILDER_BUILD_DIR)/$(gsm_dir)\
	&& mkdir -p $(prefix)/include/gsm \
	&& host_alias=$(host)  . $(BUILDER_SRC_DIR)/build/$(config_site) \
	&&  make -j1 CC="$${CC}" INSTALL_ROOT=$(prefix)  GSM_INSTALL_INC=$(prefix)/include/gsm  install

clean-libgsm:
	cd $(BUILDER_BUILD_DIR)/$(gsm_dir)\
	&& make clean

veryclean-libgsm:
	 -cd $(BUILDER_BUILD_DIR)/$(gsm_dir) \
	&& make uninstall
