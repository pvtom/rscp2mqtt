CXX=g++
TARGET=rscp2mqtt

all: $(TARGET)

$(TARGET): clean
ifeq ($(WITH_INFLUXDB), yes)
	$(CXX) -O3 RscpMqttMain.cpp RscpProtocol.cpp AES.cpp SocketConnection.cpp -pthread -lmosquitto -lcurl -o $@ -DINFLUXDB
else
	$(CXX) -O3 RscpMqttMain.cpp RscpProtocol.cpp AES.cpp SocketConnection.cpp -pthread -lmosquitto -o $@
endif

clean:
	-rm $(TARGET)
