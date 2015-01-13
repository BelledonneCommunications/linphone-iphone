
# msilbc  plugin

msilbc_dir?=msilbc

MSILBC_SRC_DIR:=$(BUILDER_SRC_DIR)/$(msilbc_dir)
MSILBC_BUILD_DIR:=$(BUILDER_BUILD_DIR)/$(msilbc_dir)

$(MSILBC_SRC_DIR)/configure:
	cd $(MSILBC_SRC_DIR) && ./autogen.sh

$(MSILBC_BUILD_DIR)/Makefile: $(MSILBC_SRC_DIR)/configure
	mkdir -p $(MSILBC_BUILD_DIR)
	cd $(MSILBC_BUILD_DIR) && \
	PKG_CONFIG_LIBDIR=$(prefix)/lib/pkgconfig CONFIG_SITE=$(BUILDER_SRC_DIR)/build/$(config_site) \
	$(MSILBC_SRC_DIR)/configure -prefix=$(prefix) --host=$(host) $(library_mode)

build-msilbc: build-libilbc $(MSILBC_BUILD_DIR)/Makefile
	cd $(MSILBC_BUILD_DIR) && make  && make install

clean-msilbc:
	cd  $(MSILBC_BUILD_DIR) && make  clean

veryclean-msilbc:
#	-cd $(MSILBC_BUILD_DIR) && make distclean
	-cd $(MSILBC_SRC_DIR) && rm configure

clean-makefile-msilbc:
	cd $(MSILBC_BUILD_DIR) && rm -f Makefile