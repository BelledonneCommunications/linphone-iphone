polarssl_dir?=externals/polarssl

update-tree:  $(BUILDER_SRC_DIR)/$(polarssl_dir)/Makefile
	mkdir -p $(BUILDER_BUILD_DIR)/$(polarssl_dir)
	cd $(BUILDER_BUILD_DIR)/$(polarssl_dir)/ && \
	rsync -rvLpgoc --exclude ".git" $(BUILDER_SRC_DIR)/$(polarssl_dir)/ .

build-polarssl: update-tree
	host_alias=$(host) && . /$(BUILDER_SRC_DIR)/build/$(config_site) && \
	cd $(BUILDER_BUILD_DIR)/$(polarssl_dir) && make CC="$$CC" AR="$$AR" CPPFLAGS="$$CPPFLAGS" lib && make install DESTDIR=$(prefix)

clean-polarssl:
	-cd $(BUILDER_BUILD_DIR)/$(polarssl_dir) && make clean

veryclean-polarssl: 
	-rm -rf $(BUILDER_BUILD_DIR)/$(polarssl_dir)

clean-makefile-polarssl: veryclean-polarssl

