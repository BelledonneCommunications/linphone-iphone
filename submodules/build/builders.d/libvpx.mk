libvpx_configure_options=\
	--enable-static   --disable-shared \
	--disable-examples 
#	--extra-cflags="-isysroot $$SYSROOT_PATH "
# -Wl,-syslibroot,$$SYSROOT_PATH " \
	--enable-error-concealment --disable-examples

ifneq (,$(findstring armv6,$(host)))
	libvpx_configure_options+= --target=armv6-darwin-gcc --cpu=arm1176jzf-s
else ifneq (,$(findstring armv7,$(host)))
	libvpx_configure_options+= --target=armv7-darwin-gcc --cpu=cortex-a8 
else
	libvpx_configure_options+= --force-target=x86-darwin10-gcc
endif
libvpx_dir?=externals/libvpx
$(BUILDER_SRC_DIR)/$(libvpx_dir)/patched :
	cd $(BUILDER_SRC_DIR)/$(libvpx_dir) \
	&& git apply $(BUILDER_SRC_DIR)/build/builders.d/libvpx.patch \
	&& touch $(BUILDER_SRC_DIR)/$(libvpx_dir)/patched

$(BUILDER_BUILD_DIR)/$(libvpx_dir)/config.mak: $(BUILDER_SRC_DIR)/$(libvpx_dir)/patched
	mkdir -p $(BUILDER_BUILD_DIR)/$(libvpx_dir)
	cd $(BUILDER_BUILD_DIR)/$(libvpx_dir)/ \
	&&  host_alias=${host} . $(BUILDER_SRC_DIR)/build/$(config_site) \
	&& $(BUILDER_SRC_DIR)/$(libvpx_dir)/configure --prefix=$(prefix) $(libvpx_configure_options)

build-libvpx: $(BUILDER_BUILD_DIR)/$(libvpx_dir)/config.mk
	cd $(BUILDER_BUILD_DIR)/$(libvpx_dir) && PKG_CONFIG_PATH=$(prefix)/lib/pkgconfig CONFIG_SITE=$(BUILDER_SRC_DIR)/build/$(config_site)  make  && make install

clean-libvpx:
	cd  $(BUILDER_BUILD_DIR)/$(libvpx_dir) && make clean

veryclean-libvpx:
	-cd $(BUILDER_BUILD_DIR)/$(libvpx_dir) && make distclean
	rm -rf $(BUILDER_BUILD_DIR)/$(libvpx_dir)

clean-makefile-libvpx:
	cd $(BUILDER_BUILD_DIR)/$(libvpx_dir) && rm -f config.mak

