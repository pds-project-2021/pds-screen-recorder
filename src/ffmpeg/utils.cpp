//
// Created by gabriele on 23/12/21.
//

#include "include/utils.h"

using namespace std;


Screen::Screen(int width, int height, int offset_x, int offset_y) {
	// NOTE: screen parameters must be even
	this->width = width % 2 ? width + 1 : width;
	this->height = height % 2 ? height + 1 : height;
	this->offset_x = offset_x % 2 ? offset_x + 1 : offset_x;
	this->offset_y = offset_y % 2 ? offset_y + 1 : offset_y;
}

/**
 * Set capture window dimension in format `widthxheigth`,
 * for example, in a FHD screen, dim will be `1920x1080`
 *
 * @param dim string containing dimensions
 */
void Screen::set_dimension(const string &dim) {
	width = stoi(dim.substr(0, dim.find('x')));
	height = stoi(dim.substr( dim.find('x')+1, dim.length()));
}

/**
 * Set capture window dimension
 *
 * @param w Width of the window
 * @param h Height of the window
 */
void Screen::set_dimension(int w, int h) {
	width = w;
	height = h;
}

/**
 * Set capture window offset position in format `posXxposY`,
 * for example, in a FHD screen, the top left angle will be `0x0` and the top right angle will be `1920x0`
 *
 * @param offset string containing offset positions
 */
void Screen::set_offset(const string &offset) {
	offset_x = stoi(offset.substr(0, offset.find('x')));
	offset_y = stoi(offset.substr( offset.find('x')+1, offset.length()));
}

/**
 * Set capture window offset position
 *
 * @param x Position relative of the x axis
 * @param y Position relative of the y axis
 */
void Screen::set_offset(int x, int y) {
	offset_x = x;
	offset_y = y;
}

void Screen::set_show_region(bool val) {
	show_region = val;
}

string Screen::get_offset_x() const {
	return to_string(offset_x);
}

string Screen::get_offset_y() const {
	return to_string(offset_y);
}

string Screen::get_offset_str() const {
	return "+" + to_string(offset_x) + "," + to_string(offset_y);
}

string Screen::get_width() const {
    return to_string(width);
}

string Screen::get_height() const {
    return to_string(height);
}

string Screen::get_video_size() const {
	return to_string(width) + "x" + to_string(height);
}

string Screen::get_show_region() const {
	return to_string(show_region);
}

/* General utils functions */

bool is_file(char *url){
	auto str = string {url};
	return str.find(".mp4") != string::npos;
}

bool is_file_str(const string &str){
	return str.find(".mp4") != string::npos;
}

void move_file(const string& source, const string& dest){
	try {
		auto dest_path = fs::path(dest);
		auto dest_folder = is_file_str(dest)? dest_path.parent_path(): dest_path;
		auto dest_file = is_file_str(dest_path.string())? dest_path: dest_path/ fs::path(source).filename();

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

    } catch (runtime_error& _e) {
		throw fsException("Unable to move " + source + " to " + dest);
	}
}