openssl_version=1.0.0a
OPENSSL_BUILD_DIR?=$(BUILDER_BUILD_DIR)/externals/openssl

ifneq (,$(findstring mingw,$(host)))
        CONFIGURE_OPTION := mingw
	MAKE_PARAMS:= CC=i586-mingw32msvc-gcc RANLIB=i586-mingw32msvc-ranlib
endif

$(OPENSSL_BUILD_DIR)/Configure:
	mkdir -p $(BUILDER_BUILD_DIR)/externals \
	&& cd $(BUILDER_BUILD_DIR)/externals \
	&& rm -rf openssl \
	&& wget ftp://sunsite.cnlab-switch.ch/mirror/openssl/source/openssl-$(openssl_version).tar.gz \
	&& tar xvzf  openssl-$(openssl_version).tar.gz \
	&& rm -f openssl-$(openssl_version).tar.gz \
	&& mv  openssl-$(openssl_version) openssl  \
	&& cd openssl && patch -p0 < $(BUILDER_SRC_DIR)/build/openssl.patch

$(OPENSSL_BUILD_DIR)/Makefile: $(OPENSSL_BUILD_DIR)/Configure
	cd $(OPENSSL_BUILD_DIR) \
	&&  host_alias=${host} . $(BUILDER_SRC_DIR)/build/$(config_site) \
	&& ./Configure -openssldir=$(prefix) --cross-compile-prefix=$$SDK_BIN_PATH/ BSD-generic32 no-asm

build-openssl: $(OPENSSL_BUILD_DIR)/Makefile
	cd $(OPENSSL_BUILD_DIR) &&  host_alias=${host} . $(BUILDER_SRC_DIR)/build/$(config_site) \
	&& make CC="$$CC" build_crypto build_ssl \
	&& cp -r include  $(prefix)/ \
	&& cp lib*.a  $(prefix)/lib 

clean-openssl:
	cd  $(OPENSSL_BUILD_DIR)  && make clean

clean-makefile-openssl:
	touch $(OPENSSL_BUILD_DIR)/Configure
	
veryclean-openssl:
	rm -rf $(OPENSSL_BUILD_DIR)

