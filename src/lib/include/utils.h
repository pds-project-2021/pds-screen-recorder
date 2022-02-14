//
// Created by gabriele on 23/12/21.
//

#pragma once

#include <filesystem>
#include <string>
#include <iostream>

#include "config.h"
#include "exceptions.h"

enum AudioLayout : unsigned int {
	MONO = 1,
	STEREO = 2
};

static int LOGGING = 0;

bool is_file(char *url);
bool is_file_str(const std::string &str);
void move_file(const std::string &source, const std::string &dest);
void delete_file(const std::string &);
std::string get_default_path(const std::filesystem::path &path = std::filesystem::temp_directory_path());
std::string get_current_time_str();

void log_info(const std::string &str);
void log_debug(const std::string &str);
void log_error(const std::string &str);

void print_version();
void print_helper();

/* generic functions */

template<typename T>
int set_even(T num) {
	int i = static_cast<int>(num);
	return i % 2 ? i - 1 : i;
}
