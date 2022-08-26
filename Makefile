main: main.cc joycon.hpp
	g++ -o main main.cc -pthread -levdev -lboost_regex -std=c++20 -g