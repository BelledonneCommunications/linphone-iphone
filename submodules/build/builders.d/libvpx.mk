libvpx_configure_options=\
	--enable-static   --disable-shared \
	--enable-error-concealment --disable-examples \
	--enable-realtime-only --enable-spatial-resampling \
	--enable-vp8 --enable-multithread

ifneq (,$(findstring armv6,$(host)))
	libvpx_configure_options+= --target=armv6-darwin-gcc --cpu=arm1176jzf-s
else ifneq (,$(findstring armv7s,$(host)))
	libvpx_configure_options+= --target=armv7s-darwin-gcc --cpu=cortex-a8  
else ifneq (,$(findstring armv7,$(host)))
	libvpx_configure_options+= --target=armv7-darwin-gcc --cpu=cortex-a8 
else
	libvpx_configure_options+= --target=x86-darwin10-gcc
endif
libvpx_dir?=externals/libvpx
all_p=armv6-darwin-gcc    #neon Cortex-A8
all_p+=armv7-darwin-gcc    #neon Cortex-A8
all_p+=armv7s-darwin-gcc   #neon Cortex-A8

$(BUILDER_SRC_DIR)/$(libvpx_dir)/patched.stamp:
	cd $(BUILDER_SRC_DIR)/$(libvpx_dir) \
	&& git apply $(BUILDER_SRC_DIR)/build/builders.d/libvpx.patch \
	&& touch $@

$(BUILDER_BUILD_DIR)/$(libvpx_dir)/config.mk: $(BUILDER_SRC_DIR)/$(libvpx_dir)/patched.stamp
	mkdir -p $(BUILDER_BUILD_DIR)/$(libvpx_dir)
	cd $(BUILDER_BUILD_DIR)/$(libvpx_dir)/ \
	&&  host_alias=${host} . $(BUILDER_SRC_DIR)/build/$(config_site) \
	&& export all_platforms="${all_p}" && $(BUILDER_SRC_DIR)/$(libvpx_dir)/configure --prefix=$(prefix) --sdk-path=$$SDK_BIN_PATH/../../ --libc=$$SYSROOT_PATH $(libvpx_configure_options)

build-libvpx: $(BUILDER_BUILD_DIR)/$(libvpx_dir)/config.mk
	cd $(BUILDER_BUILD_DIR)/$(libvpx_dir) make  && make install

clean-libvpx:
	cd  $(BUILDER_BUILD_DIR)/$(libvpx_dir) && make clean

veryclean-libvpx:
	-cd $(BUILDER_BUILD_DIR)/$(libvpx_dir) && make distclean
	cd $(BUILDER_SRC_DIR)/$(libvpx_dir) \
	&& git checkout build/make/configure.sh configure\
	&& rm -f patched.stamp 
	rm -rf $(BUILDER_BUILD_DIR)/$(libvpx_dir)

clean-makefile-libvpx:
	cd $(BUILDER_BUILD_DIR)/$(libvpx_dir) && rm -f config.mak

