CXX=g++
ROOT_VALUE=rscp2mqtt

all: $(ROOT_VALUE)

$(ROOT_VALUE): clean
	$(CXX) -O3 RscpMqttMain.cpp RscpProtocol.cpp AES.cpp SocketConnection.cpp -pthread -lmosquitto -o $@


clean:
	-rm $(ROOT_VALUE) $(VECTOR)
