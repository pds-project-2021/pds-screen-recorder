#include <iostream>
#include "Interface.h"

int main(int argc, char **argv) {
	int result;

	try{
		result = launchUI(argc, argv);
	}catch(std::runtime_error &_e){
		std::cerr << _e.what() << std::endl;
		exit(-1);
	}

	return result;
}

