ffmpeg_configure_options=\
	--disable-mmx \
	--enable-cross-compile \
	--disable-ffprobe --disable-ffserver  --disable-avdevice \
	--disable-avfilter  --disable-network \
	--disable-everything  --enable-decoder=mjpeg --enable-encoder=mjpeg --enable-decoder=mpeg4 --enable-encoder=mpeg4 \
	--enable-decoder=h264 --disable-avformat --enable-pic\
	--enable-decoder=h263p --enable-encoder=h263p --enable-decoder=h263 --enable-encoder=h263 \
	--cross-prefix=$$SDK_BIN_PATH/ \
	--sysroot=$$SYSROOT_PATH --arch=$$ARCH \
	--enable-static --disable-shared --target-os=darwin \
	--extra-cflags="$$COMMON_FLAGS" --extra-ldflags="$$COMMON_FLAGS" \
	--disable-iconv \
	--ar="$$AR" \
	--nm="$$NM" \
	--cc="$$CC"

#	--as=$(BUILDER_SRC_DIR)/externals/x264/extras/gas-preprocessor.pl

#--sysinclude=PATH        location of cross-build system headers
ifneq (,$(findstring armv6,$(host)))
	ffmpeg_configure_options+= --cpu=arm1176jzf-s --disable-armv5te  --enable-armv6 --enable-armv6t2 
endif

ifneq (,$(findstring armv7,$(host)))
	ffmpeg_configure_options+= --enable-neon --cpu=cortex-a8 --disable-armv5te  --enable-armv6 --enable-armv6t2 
endif

ffmpeg_dir?=externals/ffmpeg
$(BUILDER_SRC_DIR)/$(ffmpeg_dir)/patched :
	cd $(BUILDER_SRC_DIR)/$(ffmpeg_dir) \
	&& git apply $(BUILDER_SRC_DIR)/build/builders.d/ffmpeg.patch \
	&& touch $(BUILDER_SRC_DIR)/$(ffmpeg_dir)/patched
	
$(BUILDER_BUILD_DIR)/$(ffmpeg_dir)/config.mak:
	mkdir -p $(BUILDER_BUILD_DIR)/$(ffmpeg_dir)
	cd $(BUILDER_BUILD_DIR)/$(ffmpeg_dir)/ \
	&&  host_alias=${host} . $(BUILDER_SRC_DIR)/build/$(config_site) \
	&& $(BUILDER_SRC_DIR)/$(ffmpeg_dir)/configure --prefix=$(prefix) $(ffmpeg_configure_options)

build-ffmpeg: $(BUILDER_BUILD_DIR)/$(ffmpeg_dir)/config.mak
	cd $(BUILDER_BUILD_DIR)/$(ffmpeg_dir) \
	&&  host_alias=${host} . $(BUILDER_SRC_DIR)/build/$(config_site) \
	&& PKG_CONFIG_LIBDIR=$(prefix)/lib/pkgconfig make RANLIB="$$RANLIB" && make RANLIB="$$RANLIB" install

clean-ffmpeg:
	-cd  $(BUILDER_BUILD_DIR)/$(ffmpeg_dir) && make clean

veryclean-ffmpeg:
	-cd $(BUILDER_BUILD_DIR)/$(ffmpeg_dir) && make distclean
	rm -rf $(BUILDER_BUILD_DIR)/$(ffmpeg_dir)

clean-makefile-ffmpeg:
	cd $(BUILDER_BUILD_DIR)/$(ffmpeg_dir) && rm -f config.mak

