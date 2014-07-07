cunit_dir?=cunit

SRC_DIR=$(BUILDER_SRC_DIR)
BUILD_DIR=$(BUILDER_BUILD_DIR)


$(SRC_DIR)/$(cunit_dir)/configure $(SRC_DIR)/$(cunit_dir)/autogened:
	cd $(SRC_DIR)/$(cunit_dir) \
	&& ./autogen.sh \
	&& touch autogened

$(BUILD_DIR)/$(cunit_dir)/rsynced: $(SRC_DIR)/$(cunit_dir)/configure $(SRC_DIR)/$(cunit_dir)/autogened
	mkdir -p $(BUILD_DIR)/$(cunit_dir)
	cd $(BUILD_DIR)/$(cunit_dir)/ \
	&& rsync -rvLpgoc --exclude ".git" $(SRC_DIR)/$(cunit_dir)/* . \
	&& touch $(BUILD_DIR)/$(cunit_dir)/rsynced

$(BUILD_DIR)/$(cunit_dir)/Makefile: $(BUILD_DIR)/$(cunit_dir)/rsynced 
	mkdir -p $(BUILD_DIR)/$(cunit_dir)
	cd $(BUILD_DIR)/$(cunit_dir) \
	&& PKG_CONFIG_LIBDIR=$(prefix)/lib/pkgconfig CONFIG_SITE=$(SRC_DIR)/build/$(config_site) \
	$(SRC_DIR)/$(cunit_dir)/configure --prefix=$(prefix) --host=$(host) ${library_mode}  

build-cunit: $(BUILD_DIR)/$(cunit_dir)/Makefile
	cd $(BUILD_DIR)/${cunit_dir} \
	&& host_alias=$(host) \
	&& . $(SRC_DIR)/build/$(config_site) \
	&& make V=1 \
	&& make install V=1

clean-cunit:
	-cd $(BUILD_DIR)/$(cunit_dir) && make clean

veryclean-cunit: 
	-rm -rf $(BUILD_DIR)/$(cunit_dir)
	-rm -f $(SRC_DIR)/$(cunit_dir)/autogened

clean-makefile-cunit: veryclean-cunit

