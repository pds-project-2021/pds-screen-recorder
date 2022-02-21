//
// Created by gabriele on 21/11/21.
//

#pragma once

#include "Tracker.h"
#include "Codec.h"

class Rescaler : public Tracker<Rescaler> {
  protected:
	SwsContext *swsCtx = nullptr;
	SwrContext *swrCtx = nullptr;

	AudioLayout audio_layout;

  public:
	Rescaler() = default;
	~Rescaler();

	void set_audio_layout(AudioLayout layout);

	void set_video_scaler(const Codec &codec);
	void set_audio_scaler(const Codec &codec);
    void reset();

	SwsContext *get_sws();
	SwrContext *get_swr();
};



