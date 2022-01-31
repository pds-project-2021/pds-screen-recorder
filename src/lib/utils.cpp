//
// Created by gabriele on 23/12/21.
//

#include "utils.h"

namespace fs = std::filesystem;

bool is_file(char *url){
	auto str = std::string {url};
	return str.find(".mp4") != std::string::npos;
}

bool is_file_str(const std::string &str){
	return str.find(".mp4") != std::string::npos;
}

void move_file(const std::string& source, const std::string& dest){
	try {
		auto dest_path = fs::path(dest);
		auto dest_folder = is_file_str(dest)? dest_path.parent_path(): dest_path;
		auto dest_file = is_file_str(dest)? dest_path: dest_path/ fs::path(source).filename();

		// create parent folder tree
		if(!exists(dest_folder)){
			fs::create_directories(dest_folder);
		}

		// overwrite destination file
		if(exists(dest_file)){
			fs::remove(dest_file);
		}

		fs::copy_file(source, dest_file);
		fs::remove(source);

    } catch (std::runtime_error& _e) {
		throw fsException("Unable to move " + source + " to " + dest);
	}
}

void delete_file(const std::string& filename) {
    try {
        auto dest_path = fs::path(filename);

        // delete file
        if(exists(dest_path)){
            fs::remove(dest_path);
        }

    } catch (std::runtime_error& _e) {
        throw fsException("Unable to delete " + filename);
    }
}

std::string get_default_path() {
	return (fs::temp_directory_path()/ "output.mp4").string();
}

int set_even(int num) {
	return num % 2 ? num - 1: num;
}

void log(const std::string& str) {
	std::cout << "[INFO] " + str << std::endl;
}
