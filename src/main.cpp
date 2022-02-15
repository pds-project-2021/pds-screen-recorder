#include <iostream>
#include "Interface.h"


int main(int argc, char **argv) {
	if (argc == 2) {
		// set logging level to `info`
		if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--info") == 0) {
			LOGGING = 1;
			log_info("Logging level to `info`");
		}
		// set logging level to `debug`
		else if (strcmp(argv[1], "-w") == 0 || strcmp(argv[1], "--debug") == 0) {
			LOGGING = 2;
			log_info("Logging level to `debug`");
		}
		// print version and exit
		else if (strcmp(argv[1], "-V") == 0 || strcmp(argv[1], "--version") == 0) {
			print_version();
			return 0;
		}
		// print helper
		else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0){
			print_helper();
			return 0;
		}
	}else if (argc > 2){
		log_error("Too many arguments");
		print_helper();
		return 0;
	}

	try {
		return launchUI(argc, argv);
	} catch (std::runtime_error &_e) {
		std::cerr << _e.what() << std::endl;
		exit(-1);
	}
}

