#pragma once

#include "ffmpeg_cpp.h"

class Recorder {
  private:
	Codec codec;
	Dictionary options;
	Format format;
	Rescaler rescaler;
	Stream stream;

	// output codec
	enum AudioLayout audio_layout = MONO;
	std::string audio_codec = DEFAULT_AUDIO_CODEC;
	std::string video_codec = DEFAULT_VIDEO_CODEC;
	std::string destination_path = get_default_path();

	std::thread th_audio_demux;
	std::thread th_audio_convert;
	std::thread th_video_demux;
	std::thread th_video_convert;
	std::thread th_video;
	std::thread th_audio;

	int64_t ref_time = 0;
	unsigned int num_core = std::thread::hardware_concurrency();

	// action variable for pause and terminate
	std::atomic<bool> stopped = false;
	std::atomic<bool> pausedVideo = false;
	std::atomic<bool> pausedAudio = false;
	std::atomic<bool> finishedVideoDemux = false;
	std::atomic<bool> finishedAudioDemux = false;

	std::atomic<bool> capturing = false;

	std::mutex vD;
	std::mutex aD;
	std::mutex wR;

	std::condition_variable videoCnv;
	std::condition_variable audioCnv;
	std::condition_variable writeFrame;

	/* private functions */

	void join_all();
	void create_out_file(const std::string &dest) const;

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
	~Recorder();

	// recorder parameters functions
	[[maybe_unused]] enum AudioLayout get_audio_layout();
	[[maybe_unused]] void set_video_layout(enum AudioLayout layout);

	[[maybe_unused]] std::string get_audio_codec();
	[[maybe_unused]] std::string get_video_codec();
	[[maybe_unused]] void set_audio_codec(const std::string &cod);
	[[maybe_unused]] void set_video_codec(const std::string &cod);

	// recorder factions
	void init(Screen params);
	void capture();
	void pause();
	void resume();
	void terminate();
	bool is_paused();
	bool is_capturing();
	[[maybe_unused]] void set_destination(const std::string &);
	std::string get_destination();
	[[maybe_unused]] void set_threads(unsigned int th);

	// log functions
	[[maybe_unused]] void print_source_info();
	[[maybe_unused]] void print_destination_info(const std::string &dest) const;
};

