#pragma once

#include "Tracker.h"

template<typename T>
class ptr_wrapper : public Tracker<ptr_wrapper<T>> {
  protected:
	T *inner = nullptr;

  public:
	virtual void unref() = 0;
	T *into() { return inner; };
};
