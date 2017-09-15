
CC = g++
CXXFLAGS = 
LDFLAGS =
NAME= guitarmidi
SRC= src
LV2DIR=~/.lv2
LV2URI=http://github.com/geraldmwangi/GuitarMidi-LV2
all:
	$(MAKE) -C $(SRC)/ $(NAME)
	mkdir -p $(LV2DIR)/$(NAME).lv2
	cp $(SRC)/$(NAME).so $(LV2DIR)/$(NAME).lv2/
	cp manifest.ttl.in $(LV2DIR)/$(NAME).lv2/manifest.ttl
	cp $(NAME).ttl $(LV2DIR)/$(NAME).lv2/$(NAME).ttl
clear:
	$(MAKE) -C $(SRC)/ clear
	rm -r $(LV2DIR)/$(NAME).lv2
test:
	jalv.qt5 $(LV2URI)
