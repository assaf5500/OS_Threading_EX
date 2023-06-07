# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -Wall -pthread

# the build target executable:
TARGET1 = os4
NAME=OS
all:	clean	$(TARGET1)
$(TARGET1): $(TARGET1).c
	$(CC) $(CFLAGS) -o $(NAME) $(TARGET1).c

clean:
	$(RM) $(TARGET)

