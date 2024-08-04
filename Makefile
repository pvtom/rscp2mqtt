CXX=g++
FLAGS_MACOS=-O3 -std=c++17
FLAGS_LINUX=-O3
OS_NAME := $(shell uname -s)
CXXFLAGS=

ifeq ($(OS_NAME), Linux)
    CXXFLAGS = $(FLAGS_LINUX)
endif

ifeq ($(OS_NAME), Darwin)
    CXXFLAGS = $(FLAGS_MACOS)
endif

TARGET=rscp2mqtt

all: $(TARGET)

$(TARGET): clean
ifeq ($(WITH_INFLUXDB), yes)
	$(CXX) $(CXXFLAGS) RscpMqttMain.cpp RscpProtocol.cpp AES.cpp SocketConnection.cpp -pthread -lmosquitto -lcurl -o $@ -DINFLUXDB
else
	$(CXX) $(CXXFLAGS) RscpMqttMain.cpp RscpProtocol.cpp AES.cpp SocketConnection.cpp -pthread -lmosquitto -o $@
endif

clean:
	-rm $(TARGET)

.PHONY: all clean

