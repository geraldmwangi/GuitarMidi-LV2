CC = g++
CXXFLAGS = -g -O0
LDFLAGS =
NAME= testmidi
SRC= src
LV2DIR=~/.lv2
LV2URI=http://github.com/geraldmwangi/TestMidi
all:
	$(CC) -shared -o $(NAME).so $(CXXFLAGS) $(NAME).c
	mkdir -p $(LV2DIR)/$(NAME).lv2
	cp $(NAME).so $(LV2DIR)/$(NAME).lv2/
	cp manifest.ttl.in $(LV2DIR)/$(NAME).lv2/manifest.ttl
	cp $(NAME).ttl $(LV2DIR)/$(NAME).lv2/$(NAME).ttl
clear:
	$(MAKE) -C $(SRC)/ clear
	rm -r $(LV2DIR)/$(NAME).lv2
test:
	jalv $(LV2URI)