#include <string>
#include <sstream>
#include <iostream>
#include "parse.hpp"

int scan_int(){
	int value;
	std::string read_line;
	std::getline(std::cin, read_line);
	std::stringstream in_stream(read_line);
	if(!(in_stream >> value)){
		throw std::invalid_argument("Read a non-integer value from std::cin.");
	}
	return value;
}