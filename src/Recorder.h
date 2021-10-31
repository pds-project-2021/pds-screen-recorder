//
// Created by gabriele on 31/10/21.
//

#pragma once

#include "ffmpeg/include/ffmpeg.h"

using namespace std;

class Recorder {
  public:
	Format format;
	Codec codec;

	wrapper<AVStream> stream;

//	AVStream *videoStream = nullptr;
//	AVStream *audioStream = nullptr;


	bool record_video;
	int frame_count;

	AVDictionary *options = nullptr;


	SwsContext *swsContext = nullptr;
	SwrContext *swrContext = nullptr;

	thread *video;
	thread *audio;
    thread *audioDemux;
    thread *audioConvert;
    thread *audioWrite;
	bool finishedAudioDemux;
    bool finishedAudioConversion;
	bool recordVideo;
	const char *output_file = nullptr;
    int frameCount;

	double video_pts;

	int out_size;
	int codec_id;
	void VideoDemuxing();
	int initThreads();

  public:
	Recorder();
    ~Recorder() = default;

	void init();
	void init_output_file();
	void close_media_file();
	void capture_video_frame_thread();
	void capture_auto_frame_thread();
	void demux_audio_input();
	void convert_audio_frame();
	void write_audio_output(...);
	bool capture_starte();
};

