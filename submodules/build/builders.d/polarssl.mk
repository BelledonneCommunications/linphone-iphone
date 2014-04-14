polarssl_dir?=externals/polarssl

SRC_DIR=$(BUILDER_SRC_DIR)
BUILD_DIR=$(BUILDER_BUILD_DIR)


$(SRC_DIR)/$(polarssl_dir)/configure:
	cd $(SRC_DIR)/$(polarssl_dir) && ./autogen.sh

$(BUILD_DIR)/$(polarssl_dir)/Makefile: $(SRC_DIR)/$(polarssl_dir)/configure
	mkdir -p $(BUILD_DIR)/$(polarssl_dir)
	cd $(BUILD_DIR)/$(polarssl_dir) \
	&& PKG_CONFIG_LIBDIR=$(prefix)/lib/pkgconfig CONFIG_SITE=$(SRC_DIR)/build/$(config_site) \
	$(SRC_DIR)/$(polarssl_dir)/configure --prefix=$(prefix) --host=$(host) ${library_mode}  

build-polarssl: $(BUILD_DIR)/$(polarssl_dir)/Makefile
	cd $(BUILD_DIR)/${polarssl_dir} && \
	host_alias=$(host) && . $(SRC_DIR)/build/$(config_site) && \
	make && make install

clean-polarssl:
	-cd $(BUILD_DIR)/$(polarssl_dir) && make clean

veryclean-polarssl: 
	-rm -rf $(BUILD_DIR)/$(polarssl_dir)

clean-makefile-polarssl: veryclean-polarssl

