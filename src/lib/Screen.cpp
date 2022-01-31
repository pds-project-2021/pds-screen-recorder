//
// Created by gabriele on 29/01/22.
//

#include "Screen.h"

Screen::Screen(int width, int height, int offset_x, int offset_y) {
	// NOTE: screen parameters must be even
	this->width = set_even(width);
	this->height = set_even(height);
	this->offset_x = set_even(offset_x);
	this->offset_y = set_even(offset_y);
}

/**
 * Set capture window dimension in format `widthxheigth`,
 * for example, in a FHD screen, dim will be `1920x1080`
 *
 * @param dim string containing dimensions
 */
void Screen::set_dimension(const std::string &dim) {
	width = set_even(stoi(dim.substr(0, dim.find('x'))));
	height = set_even(stoi(dim.substr( dim.find('x')+1, dim.length())));
}

/**
 * Set capture window dimension
 *
 * @param w Width of the window
 * @param h Height of the window
 */
void Screen::set_dimension(int w, int h) {
	width = set_even(w);
	height = set_even(h);
}

/**
 * Set capture window offset position in format `posXxposY`,
 * for example, in a FHD screen, the top left angle will be `0x0` and the top right angle will be `1920x0`
 *
 * @param offset string containing offset positions
 */
void Screen::set_offset(const std::string &offset) {
	offset_x = set_even(stoi(offset.substr(0, offset.find('x'))));
	offset_y = set_even(stoi(offset.substr( offset.find('x')+1, offset.length())));
}

/**
 * Set capture window offset position
 *
 * @param x Position relative of the x axis
 * @param y Position relative of the y axis
 */
void Screen::set_offset(int x, int y) {
	offset_x = set_even(x);
	offset_y = set_even(y);
}

/**
 * View the record area into a rect
 * @param val
 */
void Screen::set_show_region(bool val) {
	show_region = val;
}

std::string Screen::get_offset_x() const {
	return std::to_string(offset_x);
}

std::string Screen::get_offset_y() const {
	return std::to_string(offset_y);
}

std::string Screen::get_offset_str() const {
	auto offx = std::to_string(offset_x);
	auto offy = std::to_string(offset_y);

	// In full screen capture the capture area can't be outside the screen size
	if (this->fullscreen()){
		offx = offy = "0";
	}

	return "+" + offx + "," + offy;
}

std::string Screen::get_width() const {
	return std::to_string(width);
}

std::string Screen::get_height() const {
	return std::to_string(height);
}

std::string Screen::get_video_size() const {
	return std::to_string(width) + "x" + std::to_string(height);
}

std::string Screen::get_show_region() const {
	return std::to_string(show_region);
}

bool Screen::fullscreen() const {
	return this->get_video_size() == "0x0";
}
