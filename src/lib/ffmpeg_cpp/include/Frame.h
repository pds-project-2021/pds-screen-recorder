
#pragma once

#include "ffmpegc.h"
#include "ptr_wrapper.h"
#include "exceptions.h"

class Frame : public ptr_wrapper<AVFrame> {
  public:
	Frame();
	Frame(int width, int height, AVPixelFormat format, int align);
	Frame(int nb_samples, AVSampleFormat format, uint64_t channel_layout, int align);
	~Frame();

	void unref() override;
};

