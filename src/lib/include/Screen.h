//
// Created by gabriele on 29/01/22.
//

#pragma once

#include <filesystem>
#include <string>

// import top level exceptions
#include "exceptions.h"
#include "utils.h"

/**
 * Useful wrapper for screen parameters
 */
class Screen {
	int width = 0;
	int height = 0;
	int offset_x = 0;
	int offset_y = 0;

	bool show_region = true;
  public:

	Screen() = default;

	[[maybe_unused]] Screen(int width, int height, int offset_x, int offset_y);
	~Screen() = default;

	[[maybe_unused]] void set_dimension(const std::string &dim);
	template<typename T>
	void set_dimension(T w, T h);

	[[maybe_unused]] void set_offset(const std::string &offset);
	template<typename T>
	void set_offset(T x, T y);

	[[maybe_unused]] void set_show_region(bool val);

	[[nodiscard]] bool fullscreen() const;
	[[nodiscard]] std::string get_offset_x() const;
	[[nodiscard]] std::string get_offset_y() const;
	[[nodiscard]] std::string get_offset_str() const;
	[[nodiscard]] std::string get_width() const;
	[[nodiscard]] std::string get_height() const;
	[[nodiscard]] std::string get_video_size() const;
	[[nodiscard]] std::string get_show_region() const;
};

/**
 * Set capture window offset position
 *
 * @param x Position relative of the x axis
 * @param y Position relative of the y axis
 */
template<typename T>
void Screen::set_offset(T x, T y) {
	offset_x = set_even(x);
	offset_y = set_even(y);
}

/**
 * Set capture window dimension
 *
 * @param w Width of the window
 * @param h Height of the window
 */
template<typename T>
void Screen::set_dimension(T w, T h) {
	width = set_even(w);
	height = set_even(h);
}