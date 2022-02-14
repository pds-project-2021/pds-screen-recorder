
/**
 * lib runtime exceptions
 */

#pragma once

#include <stdexcept>

class avException : public std::runtime_error {
  public:
	explicit avException(const std::string &arg) : runtime_error(arg) {}
};

class dataException : public std::runtime_error {
  public:
	explicit dataException(const std::string &arg) : runtime_error(arg) {}
};

class fsException : public std::runtime_error {
  public:
	explicit fsException(const std::string &arg) : runtime_error(arg) {}
};

class uiException : public std::runtime_error {
  public:
	explicit uiException(const std::string &arg) : runtime_error(arg) {}
};
