CC = gcc
CFLAGS = -Wall -g `pkg-config --cflags gtk+-3.0`
LDFLAGS = `pkg-config --libs gtk+-3.0` -lrt -pthread

# Object files
OBJS = main.o model.o view.o controller.o

# Target executable
TARGET = my_shell

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

main.o: main.c model.h view.h controller.h
	$(CC) $(CFLAGS) -c main.c

model.o: model.c model.h
	$(CC) $(CFLAGS) -c model.c

view.o: view.c view.h controller.h
	$(CC) $(CFLAGS) -c view.c

controller.o: controller.c controller.h model.h view.h
	$(CC) $(CFLAGS) -c controller.c

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
