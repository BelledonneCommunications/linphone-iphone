TUNNEL_SRC_DIR?=$(BUILDER_SRC_DIR)/tunnel
TUNNEL_BUILD_DIR?=$(BUILDER_BUILD_DIR)/tunnel

#############################TUNNEL############################
$(TUNNEL_SRC_DIR)/configure:
	cd $(TUNNEL_SRC_DIR) && ./autogen.sh

$(TUNNEL_BUILD_DIR)/Makefile: $(TUNNEL_SRC_DIR)/configure
	mkdir -p  $(TUNNEL_BUILD_DIR) \
	&& cd $(TUNNEL_BUILD_DIR) \
	&& CONFIG_SITE=$(BUILDER_SRC_DIR)/build/$(config_site) PKG_CONFIG_PATH=$(prefix)/lib/pkgconfig \
	$(TUNNEL_SRC_DIR)/configure -prefix=$(prefix) --host=$(host) --enable-polarssl --with-polarssl=$(prefix) --disable-servers  ${library_mode}

build-tunnel: $(TUNNEL_BUILD_DIR)/Makefile
	cd $(TUNNEL_BUILD_DIR) \
	&& host_alias=$(host) . $(BUILDER_SRC_DIR)/build/$(config_site) \
	&& make RANLIB="$$RANLIB" && make install

clean-tunnel:
	cd  $(TUNNEL_BUILD_DIR)  && make clean

veryclean-tunnel:
	rm -f  $(TUNNEL_SRC_DIR)/configure 

clean-makefile-tunnel:
	cd $(TUNNEL_BUILD_DIR) && rm -f Makefile


