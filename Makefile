LIBS=-levent -levent_core

server: mytcpserver.cpp test.cpp ./pubkey/ecc.cpp ./bn/bn_boost.cpp
	g++ $^ -Wall -std=c++11 $(LIBS) -g -o $@
