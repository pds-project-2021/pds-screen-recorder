//
// Created by gabriele on 21/11/21.
//

#pragma once

#include "ffmpegc.h"
#include "Tracker.h"
#include "Codec.h"

class Rescaler: public Tracker<Rescaler> {
  protected:
	SwsContext* swsCtx = nullptr;
	SwrContext* swrCtx = nullptr;


  public:
	Rescaler();
	~Rescaler();

	void scale_video(const Codec& codec);
	void scale_audio(const Codec& codec);

};



