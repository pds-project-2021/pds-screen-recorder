//
// Created by gabriele on 31/10/21.
//

#pragma once

#include "ptr_wrapper.h"

class Frame: public ptr_wrapper<AVFrame>{
  public:
	Frame();
	Frame(int width, int height, AVPixelFormat format, int align);
	Frame(int nb_samples, AVSampleFormat format, uint64_t channel_layout, int align);
};

