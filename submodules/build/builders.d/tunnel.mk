TUNNEL_SRC_DIR?=$(BUILDER_SRC_DIR)/linphone/tunnel
TUNNEL_BUILD_DIR?=$(BUILDER_BUILD_DIR)/linphone/tunnel

#############################TUNNEL############################
$(TUNNEL_SRC_DIR)/configure:
	cd $(TUNNEL_SRC_DIR) && ./autogen.sh

$(TUNNEL_BUILD_DIR)/Makefile: $(TUNNEL_SRC_DIR)/configure
	mkdir -p  $(TUNNEL_BUILD_DIR) \
	&& cd $(TUNNEL_BUILD_DIR) \
	&& OPENSSL_CFLAGS="-I$(prefix)/include" OPENSSL_LIBS="-L$(prefix)/lib -lssl -lcrypto" CONFIG_SITE=$(BUILDER_SRC_DIR)/build/$(config_site) \
	$(TUNNEL_SRC_DIR)/configure -prefix=$(prefix) --host=$(host) ${library_mode}

build-tunnel: $(TUNNEL_BUILD_DIR)/Makefile
	cd $(TUNNEL_BUILD_DIR) \
	&& export OPENSSL_CFLAGS="-I$(prefix)/include" export OPENSSL_LIBS="-L$(prefix)/lib -lssl -lcrypto" export CONFIG_SITE=$(BUILDER_SRC_DIR)/build/$(config_site) \
	&& make  && make install

clean-tunnel:
	cd  $(TUNNEL_BUILD_DIR)  && make clean

veryclean-tunnel:
	rm -f  $(TUNNEL_SRC_DIR)/configure 

clean-makefile-tunnel:
	cd $(TUNNEL_BUILD_DIR) && rm -f Makefile


