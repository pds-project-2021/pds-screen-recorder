//
// Created by gabriele on 23/12/21.
//

#include "utils.h"

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