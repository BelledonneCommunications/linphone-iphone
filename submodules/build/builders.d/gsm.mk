
#GSM

gsm_dir?=externals/gsm

#GSM build is a bit different: since there is only a Makefile,
#we must force CC to contains CFLAGS to compile all architectures
#as expected
build-libgsm:
	cp -rf $(BUILDER_SRC_DIR)/$(gsm_dir) $(BUILDER_BUILD_DIR)/$(gsm_dir)
	rm -rf $(BUILDER_BUILD_DIR)/$(gsm_dir)/gsm/.git $(BUILDER_BUILD_DIR)/$(gsm_dir)/.git
	rm -f $(prefix)/lib/libgsm.a
	rm -rf $(prefix)/include/gsm
	cd $(BUILDER_BUILD_DIR)/$(gsm_dir)\
	&& mkdir -p $(prefix)/include/gsm \
	&& host_alias=$(host)  . $(BUILDER_SRC_DIR)/build/$(config_site) \
	&& make install \
			CC="$${CC} $${COMMON_FLAGS} -w" \
			INSTALL_ROOT=$(prefix) \
			GSM_INSTALL_INC=$(prefix)/include/gsm

clean-libgsm:
	cd $(BUILDER_BUILD_DIR)/$(gsm_dir)\
	&& make clean

veryclean-libgsm:
	-rm -rf $(BUILD_DIR)/$(gsm_dir)


