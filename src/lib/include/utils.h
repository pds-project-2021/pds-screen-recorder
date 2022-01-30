//
// Created by gabriele on 23/12/21.
//

#pragma once

#include <filesystem>
#include <string>

#include "exceptions.h"

//using namespace std;
namespace fs = std::filesystem;

bool is_file(char *url);
bool is_file_str(const std::string &str);
void move_file(const std::string& source, const std::string &dest);