polarssl_dir?=externals/polarssl

$(BUILDER_BUILD_DIR)/$(polarssl_dir)/Makefile:	$(BUILDER_SRC_DIR)/$(polarssl_dir)/Makefile
	mkdir -p $(BUILDER_BUILD_DIR)/$(polarssl_dir)
	cd $(BUILDER_BUILD_DIR)/$(polarssl_dir)/ && \
	rsync -rvLpgoc --exclude ".git" $(BUILDER_SRC_DIR)/$(polarssl_dir)/ .

build-polarssl: $(BUILDER_BUILD_DIR)/$(polarssl_dir)/Makefile
	host_alias=$(host) && . /$(BUILDER_SRC_DIR)/build/$(config_site) && \
	cd $(BUILDER_BUILD_DIR)/$(polarssl_dir) && make lib && make install DESTDIR=$(prefix)

clean-polarssl:
	-cd $(BUILDER_BUILD_DIR)/$(polarssl_dir) && make clean

veryclean-polarssl: 
	-rm -rf $(BUILDER_BUILD_DIR)/$(polarssl_dir)

clean-makefile-polarssl: veryclean-polarssl

