//
// Created by gabriele on 23/12/21.
//

#pragma once

#include <filesystem>
#include <string>

#include "exceptions.h"

using namespace std;
namespace fs = filesystem;

bool is_file(char *url);
bool is_file_str(const string &str);
void move_file(const string& source, const string &dest);