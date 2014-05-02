libvpx_dir?=externals/libvpx

libvpx_configure_options=\
	--enable-static   --disable-shared \
	--enable-error-concealment --disable-examples \
	--enable-realtime-only --enable-spatial-resampling \
	--enable-vp8 --enable-multithread --disable-vp9

take_binary=

# force take binary, it seems libvpx was fixed since http://git.chromium.org/gitweb/?p=webm/libvpx.git;a=commit;h=33df6d1fc1d268b4901b74b4141f83594266f041
force_non_binary_libvpx=1

ifneq (,$(findstring armv6,$(host)))
	libvpx_configure_options+= --target=armv6-darwin-gcc --cpu=arm1176jzf-s
else ifneq (,$(findstring armv7s,$(host)))
	libvpx_configure_options+= --target=armv7s-darwin-gcc
	take_binary = armv7s
else ifneq (,$(findstring armv7,$(host)))
	libvpx_configure_options+= --target=armv7-darwin-gcc
	take_binary = armv7
else
	libvpx_configure_options+= --target=x86-darwin10-gcc
	take_binary = i386
endif

all_p=armv6-darwin-gcc    #neon Cortex-A8
all_p+=armv7-darwin-gcc    #neon Cortex-A8
all_p+=armv7s-darwin-gcc   #neon Cortex-A8

ifeq ($(force_non_binary_libvpx),1)
take_binary=
endif

# $(BUILDER_SRC_DIR)/$(libvpx_dir)/patched.stamp:
# 	cd $(BUILDER_SRC_DIR)/$(libvpx_dir) \
# 	&& git apply $(BUILDER_SRC_DIR)/build/builders.d/libvpx.patch \
# 	&& touch $@


#$(BUILDER_BUILD_DIR)/$(libvpx_dir)/config.mk: $(BUILDER_SRC_DIR)/$(libvpx_dir)/patched.stamp
$(BUILDER_BUILD_DIR)/$(libvpx_dir)/config.mk:
	mkdir -p $(BUILDER_BUILD_DIR)/$(libvpx_dir)
	cd $(BUILDER_BUILD_DIR)/$(libvpx_dir)/ \
	&&  host_alias=${host} . $(BUILDER_SRC_DIR)/build/$(config_site) \
	&& export all_platforms="${all_p}" &&  $(BUILDER_SRC_DIR)/$(libvpx_dir)/configure --prefix=$(prefix) --sdk-path=$$SDK_BIN_PATH/../../ --libc=$$SYSROOT_PATH $(libvpx_configure_options) --extra-cflags="-fno-strict-aliasing"

build-libvpx: $(BUILDER_BUILD_DIR)/$(libvpx_dir)/config.mk
	cd $(BUILDER_BUILD_DIR)/$(libvpx_dir) \
	&& host_alias=${host} . $(BUILDER_SRC_DIR)/build/$(config_site) \
	&& PKG_CONFIG_LIBDIR=$(prefix)/lib/pkgconfig CONFIG_SITE=$(BUILDER_SRC_DIR)/build/$(config_site) make && make install
ifeq ($(force_non_binary_libvpx),1)
	@echo "\033[01;32m DON'T get BINARY version of libvpx for $(take_binary), because 'force_non_binary_libvpx' is 1 \033[0m"
endif
ifneq (,$(take_binary))
# we have to take binary version of libvpx for ARM because Clang introduces bugs in optimized assembly
	@echo "\033[01;32m Getting BINARY version of libvpx for $(take_binary) \033[0m"
	cp $(BUILDER_SRC_DIR)/binaries/libvpx-$(take_binary).a $(prefix)/lib/libvpx.a
endif

clean-libvpx:
	cd  $(BUILDER_BUILD_DIR)/$(libvpx_dir) && make clean

veryclean-libvpx:
	-cd $(BUILDER_BUILD_DIR)/$(libvpx_dir) && make distclean
	cd $(BUILDER_SRC_DIR)/$(libvpx_dir) \
	&& git clean -f && git reset --hard 
	rm -rf $(BUILDER_BUILD_DIR)/$(libvpx_dir)

clean-makefile-libvpx:
	cd $(BUILDER_BUILD_DIR)/$(libvpx_dir) && rm -f config.mak

