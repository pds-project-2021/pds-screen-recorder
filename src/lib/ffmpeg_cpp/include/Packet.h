
#pragma once

#include "ffmpegc.h"
#include "ptr_wrapper.h"
#include "exceptions.h"

class Packet : public ptr_wrapper<AVPacket> {
  public:
	Packet();
	~Packet();

	void unref() override;
};

