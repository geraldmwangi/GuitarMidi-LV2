CC = g++
CXXFLAGS = -fPIC -DPIC
LDFLAGS =
NAME= guitarmidi
CPPFILES= $(NAME).c
$(NAME):
	$(CC) -shared -o $(NAME).so $(CXXFLAGS) $(CPPFILES)

clear:
	rm -f *.o
	rm -f *.so
