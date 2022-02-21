/**
 * lib runtime exceptions
 */

#pragma once

#include <stdexcept>

/**
 * Runtime exception relative to `libav` functions
 */
class avException : public std::runtime_error {
  public:
	explicit avException(const std::string &arg) : runtime_error(arg) {}
};

/**
 * Runtime exception relative to file operation
 */
class fsException : public std::runtime_error {
  public:
	explicit fsException(const std::string &arg) : runtime_error(arg) {}
};

/**
 * Runtime exception relative to the Interface
 */
class uiException : public std::runtime_error {
  public:
	explicit uiException(const std::string &arg) : runtime_error(arg) {}
};
