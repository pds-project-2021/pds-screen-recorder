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

	// output codec
	string audio_codec = DEFAULT_AUDIO_CODEC;
	string video_codec = DEFAULT_VIDEO_CODEC;

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

	atomic<bool> capturing = false;

	mutex vD;
	mutex aD;
	mutex wR;

	condition_variable videoCnv;
	condition_variable audioCnv;
	condition_variable writeFrame;

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

	// recorder parameters functions
	[[maybe_unused]] string get_audio_codec();
	[[maybe_unused]] string get_video_codec();
	[[maybe_unused]] void set_audio_codec(const string &cod);
	[[maybe_unused]] void set_video_codec(const string &cod);

	// recorder factions
	void init(Screen params);
	void capture();
	void pause();
	void resume();
	void terminate();
	bool is_paused();
	bool is_capturing();

	[[maybe_unused]] void set_threads(unsigned int th);

	// log functions
	[[maybe_unused]] void print_source_info();
	[[maybe_unused]] void print_destination_info(const string& dest) const;
};

