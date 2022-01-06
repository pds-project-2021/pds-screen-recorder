//
// Created by gabriele on 31/10/21.
//

#pragma once

#include "ffmpeg/include/ffmpeg.h"

using namespace std;

class Recorder {
  private:
	Codec codec;
	Dictionary options;
	Format format;
	Rescaler rescaler;
	Stream stream;

	thread th_audio_demux;
	thread th_audio_convert;
	thread th_video_demux;
	thread th_video_convert;
	thread th_video;
	thread th_audio;

	int64_t ref_time = 0;
	unsigned int num_core = thread::hardware_concurrency();

	// action variable for pause and terminate
	atomic<bool> stopped = false;
	atomic<bool> pausedVideo = false;
	atomic<bool> pausedAudio = false;
	atomic<bool> finishedVideoDemux = false;
	atomic<bool> finishedAudioDemux = false;

	mutex vD;
	mutex aD;
	mutex wR;
//	mutex vP;
//	mutex aP;

	condition_variable videoCnv;
	condition_variable audioCnv;
	condition_variable writeFrame;
//    condition_variable videoDmx;
//    condition_variable audioDmx;

	/* private functions */
	void join_all();
	void create_out_file(const string& dest) const;

	// single thread (de)muxing
	void CaptureAudioFrames();
	void CaptureVideoFrames();

	// multithread (de)muxing
	void ConvertAudioFrames();
	void DemuxAudioInput();
	void ConvertVideoFrames();
	void DemuxVideoInput();

  public:
	Recorder();
    ~Recorder() = default;

	void init(Screen params);
	void capture();
	void pause();
	void resume();
	void terminate();
	bool is_paused();

	[[maybe_unused]] void set_threads(unsigned int th);

	// log functions
	[[maybe_unused]] void print_source_info() const;
	[[maybe_unused]] void print_destination_info(const string& dest) const;
};

