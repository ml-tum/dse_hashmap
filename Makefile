all:
	g++ -std=c++17 server.cpp -o server
	g++ -std=c++17 client.cpp -o client
	g++ -std=c++17 -O3 stress_test.cpp -o stress_test

