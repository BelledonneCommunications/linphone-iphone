ffmpeg_configure_options=\
	--disable-mmx \
	--enable-cross-compile \
	--disable-ffprobe --disable-ffserver  --disable-avdevice \
	--disable-avfilter  --disable-network \
	--disable-everything  --enable-decoder=mpeg4 --enable-encoder=mpeg4 \
	--enable-decoder=h264 --disable-avformat --enable-armv6 --enable-armv6t2 \
	--enable-armvfp --enable-neon \
	--source-path=$(BUILDER_SRC_DIR)/$(ffmpeg_dir) \
	--cross-prefix=$$SDK_BIN_PATH/ \
	--sysroot=$$SYSROOT_PATH --arch=$$ARCH \
	--enable-static   --disable-shared --target-os=darwin \
	--cpu=cortex-a8 --extra-cflags="-arch $$ARCH " --extra-ldflags="-arch $$ARCH -Wl,-syslibroot,$$SYSROOT_PATH " \
#	--as=$(BUILDER_SRC_DIR)/externals/x264/extras/gas-preprocessor.pl

#--sysinclude=PATH        location of cross-build system headers

ffmpeg_dir?=externals/ffmpeg

$(BUILDER_BUILD_DIR)/$(ffmpeg_dir)/Makefile: $(BUILDER_SRC_DIR)/$(ffmpeg_dir)/configure
	mkdir -p $(BUILDER_BUILD_DIR)/$(ffmpeg_dir)
	cd $(BUILDER_BUILD_DIR)/$(ffmpeg_dir)/ \
	&&  host_alias=${host} . $(BUILDER_SRC_DIR)/build/$(config_site) \
	&& $(BUILDER_SRC_DIR)/$(ffmpeg_dir)/configure --prefix=$(prefix) 	$(ffmpeg_configure_options)

build-ffmpeg: $(BUILDER_BUILD_DIR)/$(ffmpeg_dir)/Makefile
	cd $(BUILDER_BUILD_DIR)/$(ffmpeg_dir) && PKG_CONFIG_PATH=$(prefix)/lib/pkgconfig CONFIG_SITE=$(BUILDER_SRC_DIR)/build/$(config_site)  make && make install

clean-ffmpeg:
	cd  $(BUILDER_BUILD_DIR)/$(ffmpeg_dir) && make clean

veryclean-ffmpeg:
	cd $(BUILDER_BUILD_DIR)/$(ffmpeg_dir) && make distclean
	cd $(BUILDER_SRC_DIR)/$(ffmpeg_dir) && rm -f configure

clean-makefile-ffmpeg:
	cd $(BUILDER_BUILD_DIR)/$(ffmpeg_dir) && rm -f Makefile

