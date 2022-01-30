//
// Created by gabriele on 29/01/22.
//

#pragma once

#include <filesystem>
#include <string>

// import top level exceptions
#include "exceptions.h"

/**
 * Useful wrapper for screen parameters
 */
class Screen{
	int width = 0.0;
	int height = 0.0;
	int offset_x = 0.0;
	int offset_y = 0.0;

	bool show_region = true;
  public:

	Screen() = default;
	Screen(int width, int height, int offset_x, int offset_y);
	~Screen() = default;

	void set_dimension(const std::string &dim);
	void set_dimension(int w, int h);
	void set_offset(const std::string &offset);
	void set_offset(int x, int y);
	void set_show_region(bool val);

	[[nodiscard]] std::string get_offset_x() const;
	[[nodiscard]] std::string get_offset_y() const;
	[[nodiscard]] std::string get_offset_str() const;
	[[nodiscard]] std::string get_width() const;
	[[nodiscard]] std::string get_height() const;
	[[nodiscard]] std::string get_video_size() const;
	[[nodiscard]] std::string get_show_region() const;
};



