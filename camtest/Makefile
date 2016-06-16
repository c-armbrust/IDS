all: camtest
camtest: camtest.o
	g++ -std=c++11 camtest.o -lueye_api -o camtest
camtest.o:
	g++ -std=c++11 -c camtest.cpp

clean: 
	rm camtest  camtest.o
