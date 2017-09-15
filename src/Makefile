CC = g++
CXXFLAGS = 
LDFLAGS =
NAME= guitarmidi
CPPFILES= $(NAME).c
$(NAME):
	$(CC) -shared -o $(NAME).so -fPIC $(CPPFILES)

clear:
	rm -f *.o
	rm -f *.so
