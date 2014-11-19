#speex

speex_dir=externals/speex


ifneq (,$(findstring arm,$(host)))
	#SPEEX_CONFIGURE_OPTION := --enable-fixed-point --disable-float-api
	CFLAGS := $(CFLAGS) -marm
	SPEEX_CONFIGURE_OPTION := --disable-float-api --enable-arm5e-asm --enable-fixed-point
endif

ifneq (,$(findstring armv7,$(host)))
	SPEEX_CONFIGURE_OPTION += --enable-armv7neon-asm
endif
ifneq (,$(findstring aarch64,$(host)))
	SPEEX_CONFIGURE_OPTION += --disable-armv7neon-asm
endif

ifeq ($(enable_debug),yes)
	CFLAGS := $(CFLAGS) -g
	SPEEX_CONFIGURE_OPTION += --enable-debug
endif

$(BUILDER_SRC_DIR)/$(speex_dir)/configure:
	 cd $(BUILDER_SRC_DIR)/$(speex_dir) && ./autogen.sh

$(BUILDER_BUILD_DIR)/$(speex_dir)/Makefile: $(BUILDER_SRC_DIR)/$(speex_dir)/configure
	mkdir -p $(BUILDER_BUILD_DIR)/$(speex_dir)
	cd $(BUILDER_BUILD_DIR)/$(speex_dir)/\
	&& CONFIG_SITE=$(BUILDER_SRC_DIR)/build/$(config_site) CFLAGS="$(CFLAGS) -O2" \
	$(BUILDER_SRC_DIR)/$(speex_dir)/configure -prefix=$(prefix) --host=$(host) ${library_mode} --disable-ogg  $(SPEEX_CONFIGURE_OPTION)

build-speex: $(BUILDER_BUILD_DIR)/$(speex_dir)/Makefile
	cd $(BUILDER_BUILD_DIR)/$(speex_dir) && make  && make install

clean-speex:
	cd  $(BUILDER_BUILD_DIR)/$(speex_dir)  && make clean

veryclean-speex:
#	-cd $(BUILDER_BUILD_DIR)/$(speex_dir) && make distclean
	-rm -f $(BUILDER_SRC_DIR)/$(speex_dir)/configure

clean-makefile-speex:
	cd $(BUILDER_BUILD_DIR)/$(speex_dir) && rm -f Makefile
