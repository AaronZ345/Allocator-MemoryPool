vector_test.out:main.o int_test.out int_test2.out
	g++ -o vector_test.out main.o
int_test.out:main1.o
	g++ -o int_test.out main1.o
int_test2.out:main2.o
	g++ -o int_test2.out main2.o
main.o:main.cpp
	g++ -c main.cpp -std=c++17
main1.o:main1.cpp
	g++ -c main1.cpp -std=c++17	
main2.o:main2.cpp
	g++ -c main2.cpp -std=c++17
