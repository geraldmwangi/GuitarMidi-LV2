
PREFIX ?= /usr
LV2DIR ?= $(PREFIX)/lib/lv2

PLUGIN = guitarmidi-lv2
BUNDLE = $(PLUGIN).lv2

INSTALL_DIR = $(DESTDIR)$(LV2DIR)/$(BUNDLE)
BUILD_DIR = build
SOURCE_DIR = ../../../
$(PLUGIN).so: 
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(INSTALL_DIR)  $(SOURCE_DIR)
	cd $(BUILD_DIR) && cmake --build .
install: $(PLUGIN).so
	cd $(BUILD_DIR) && cmake --install .

clean:
	rm -rf $(BUILD_DIR)
