//
// Created by gabriele on 23/12/21.
//

#pragma once

#include <filesystem>
#include <string>
#include <iostream>

#include "exceptions.h"

enum AudioLayout: unsigned int {
	MONO=1,
	STEREO=2
};

bool is_file(char *url);
bool is_file_str(const std::string &str);
void move_file(const std::string& source, const std::string &dest);
void delete_file(const std::string&);
std::string get_default_path();
void log_info(const std::string& str);

/* generic functions */

template<typename T> int set_even(T num){
	int i = static_cast<int>(num);
	return i % 2 ? i - 1: i;
}
