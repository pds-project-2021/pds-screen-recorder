//
// Created by gabriele on 23/12/21.
//

#pragma once

#include <filesystem>
#include <string>
#include <iostream>

#include "exceptions.h"

bool is_file(char *url);
bool is_file_str(const std::string &str);
void move_file(const std::string& source, const std::string &dest);
void delete_file(const std::string&);
std::string get_default_path();
int set_even(int num);
void log_info(const std::string& str);