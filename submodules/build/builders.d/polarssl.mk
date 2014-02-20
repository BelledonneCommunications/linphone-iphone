polarssl_dir?=externals/polarssl

SRC_DIR=$(BUILDER_SRC_DIR)
BUILD_DIR=$(BUILDER_BUILD_DIR)


$(SRC_DIR)/$(polarssl_dir)/configure:
	cd $(SRC_DIR)/$(polarssl_dir) && ./autogen.sh

$(SRC_DIR)/$(polarssl_dir)/Makefile: $(SRC_DIR)/$(polarssl_dir)/configure
	mkdir -p $(BUILD_DIR)/$(polarssl_dir)
	cd $(BUILD_DIR)/$(polarssl_dir)/ \
	&& PKG_CONFIG_LIBDIR=$(prefix)/lib/pkgconfig CONFIG_SITE=$(SRC_DIR)/build/$(config_site) \
	$(SRC_DIR)/$(polarssl_dir)/configure --prefix=$(prefix) --host=$(host) ${library_mode}  

update-tree:  $(SRC_DIR)/$(polarssl_dir)/Makefile
	mkdir -p $(BUILD_DIR)/$(polarssl_dir)
	cd $(BUILD_DIR)/$(polarssl_dir)/ && \
	rsync -rvLpgoc --exclude ".git" $(SRC_DIR)/$(polarssl_dir)/ .

build-polarssl: update-tree
	host_alias=$(host) && . /$(SRC_DIR)/build/$(config_site) && \
	cd $(BUILD_DIR)/$(polarssl_dir) && make && make install

clean-polarssl:
	-cd $(BUILD_DIR)/$(polarssl_dir) && make clean

veryclean-polarssl: 
	-rm -rf $(BUILD_DIR)/$(polarssl_dir)

clean-makefile-polarssl: veryclean-polarssl

