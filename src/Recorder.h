//
// Created by gabriele on 31/10/21.
//

#pragma once

#include "ffmpeg/include/ffmpeg.h"
#include "Screen.h"


using namespace std;

class Recorder {
  private:
	Codec codec;
	Format format;
	Dictionary options;
	Stream stream;
	Rescaler rescaler;

	thread th_audio_demux;
	thread th_audio_convert;
	thread th_video_demux;
	thread th_video_convert;
	thread th_video;

	int64_t ref_time = 0;

	// action variable for pause and stop
	atomic_bool pausedVideo = false;
	atomic_bool pausedAudio = false;
	atomic_bool stopped = false;
	atomic_bool finishedVideoDemux = false;
	atomic_bool finishedAudioDemux = false;

	mutex vD;
	mutex aD;
	mutex vP;
	mutex aP;
	mutex wR;

	condition_variable videoCnv;
    condition_variable audioCnv;
    condition_variable videoDmx;
    condition_variable audioDmx;

	condition_variable writeFrame;

	unsigned int num_core = 4; // todo: update with real number of core

	// private functions

	void DemuxVideoInput();
	void ConvertVideoFrames();
	void CaptureVideoFrames();
	void join_all();

  public:
	Recorder();
    ~Recorder() = default;

	void init(Screen params);
	void capture();
	void capture_blocking();
	void pause();
	void stop();



//	void init_output_file();
//	void close_media_file();
//	void capture_video_frame_thread();
//	void capture_auto_frame_thread();
//	void demux_audio_input();
//	void convert_audio_frame();
//	void write_audio_output(...);
//	bool capture_starte();

	void create_out_file(const string& dest) const;
	int64_t get_platform_ref_time() const;

	// log functions
	[[maybe_unused]] void print_source_info() const;
	[[maybe_unused]] void print_destination_info(const string& dest) const;
};

