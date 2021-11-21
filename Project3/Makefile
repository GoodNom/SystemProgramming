CC = gcc
CFLAGS = -Wall -W -g
OBJECTS = 20171630.o shellcmd.o memcmd.o opcmd.o sicxecmd.o linkload.o
TARGET = 20171630.out

all: $(TARGET)

$(TARGET) : $(OBJECTS)
	      $(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

clean:
	rm -rf $(OBJECTS) $(TARGET) $(TARGET).dSYM
