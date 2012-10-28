CXX=clang++
CFLAGS=-c -std=c++0x -g -Wall -I/usr/include/libusb-1.0
LDFLAGS=-g -lusb-1.0 -llog4cplus
EXECUTABLE=jambox
SOURCES=src/jambox.cpp src/usb.cpp
OBJECTS=$(SOURCES:.cpp=.o)

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

.cpp.o:
	$(CXX) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
