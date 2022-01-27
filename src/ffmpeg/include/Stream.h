//
// Created by gabriele on 31/10/21.
//

#pragma once

#include "wrapper.h"
#include "Format.h"
#include "Codec.h"

class Stream: public wrapper<AVStream> {
  public:
	Stream()=default;
	Stream(const Format& format, const Codec& codec);
	~Stream()=default;
};



