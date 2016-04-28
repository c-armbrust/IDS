all: camtest
camtest: camtest.o
	g++ -std=c++14 camtest.o -lueye_api -o camtest
camtest.o:
	g++ -std=c++14 -c camtest.cpp

clean: 
	rm camtest  camtest.o
