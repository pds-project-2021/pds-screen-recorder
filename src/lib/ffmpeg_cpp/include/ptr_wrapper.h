#pragma once

template<typename T>
class ptr_wrapper {
  protected:
	T *inner = nullptr;

  public:
	virtual void unref() = 0;
	T *into() { return inner; };
};
