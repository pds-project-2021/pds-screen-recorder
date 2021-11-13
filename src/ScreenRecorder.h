#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include<string>
#include <math.h>
#include <string.h>
#include <memory>
#include <atomic>
#include <vector>
#include <future>
extern "C" {
#include <libavutil/audio_fifo.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libswresample/swresample.h>
}
#include "exceptions/avException.h"
#include "exceptions/dataException.h"
#include "exceptions/fsException.h"
#include <thread>

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
// Windows
#include <dshow.h>
#include <windows.h>
#include <dshow.h>
#pragma comment(lib, "strmiids.lib")
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
};
#else
// Linux and MacOS
#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libavcodec/avfft.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include "libavformat/avformat.h"
#include "libavformat/avio.h"

// libav resample

#include "libavutil/channel_layout.h"
#include "libavutil/common.h"
#include "libavutil/file.h"
#include "libavutil/imgutils.h"
#include "libavutil/mathematics.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "libavutil/samplefmt.h"
#ifdef __cplusplus
};
#endif
#endif

// Refresh Event

class ScreenRecorder {
  private:
	//  AVFormatContext *pFormatCtx;
	//  AVCodecContext *pCodecCtx;
	//  AVCodecParameters *pCodecPar;
	//  AVCodec *pCodec;
	//  AVInputFormat *ifmt;
	//  AVDictionary *options;

	//  int i, video_index;

	//  -----------------------
	std::unique_ptr<AVInputFormat> inputFormat = nullptr;
    std::unique_ptr<AVInputFormat> audioInputFormat = nullptr;
    std::unique_ptr<AVOutputFormat> outputFormat = nullptr;
	AVOutputFormat *audioOutputFormat = nullptr;
	AVCodecContext *inputCodecContext = nullptr;
	AVCodecContext *audioInputCodecContext = nullptr;
	AVCodecContext *outputCodecContext = nullptr;
	AVCodecContext *audioOutputCodecContext = nullptr;
	AVFormatContext *inputFormatContext = nullptr;
	AVFormatContext *audioInputFormatContext = nullptr;
	AVFormatContext *outputFormatContext = nullptr;
	AVFormatContext *audioOutputFormatContext = nullptr;
	AVCodecParameters *inputCodecPar = nullptr;
	AVCodecParameters *audioInputCodecPar = nullptr;
	AVCodecParameters *outputCodecPar = nullptr;
	AVCodecParameters *audioOutputCodecPar = nullptr;

//	AVCodec *inputCodec = nullptr;
//	AVCodec *audioInputCodec = nullptr;
//	AVCodec *outputCodec = nullptr;
//	AVCodec *audioOutputCodec = nullptr;
	AVDictionary *options = nullptr;
    AVDictionary *audioOptions = nullptr;
	AVStream *videoStream = nullptr;
	AVStream *audioStream = nullptr;
	SwsContext *swsContext = nullptr;
	SwrContext *swrContext = nullptr;
	std::thread *video;
	std::thread *audio;
    std::thread *videoDemux;
    std::thread *videoConvert;
    std::thread *audioDemux;
    std::thread *audioConvert;
//    std::atomic_bool *paused;
    std::atomic_bool *pausedVideo;
    std::atomic_bool *pausedAudio;
    std::atomic_bool *stopped;
    std::mutex *vD;
    std::mutex *aD;
    std::mutex *vP;
    std::mutex *aP;
    std::mutex *wR;
    std::condition_variable *videoCnv;
    std::condition_variable *audioCnv;
    std::condition_variable *videoDmx;
    std::condition_variable *audioDmx;
    std::condition_variable *writeFrame;
    bool finishedVideoDemux;
	bool finishedAudioDemux;
    bool finishedAudioConversion;
	bool recordVideo;
	const char *output_file = nullptr;
    int frameCount;
    std::vector<std::string> *audio_devices;
	double video_pts;

	int out_size;
	int codec_id;
	void DemuxVideoInput();
    void ConvertVideoFrames();
    void CaptureVideoFrames();
    void CaptureAudioFrames();
    void DemuxAudioInput();
    void ConvertAudioFrames();
    void WriteAudioOutput(AVFormatContext*, AVRational, AVRational);
	int initThreads();
    int CloseMediaFile();

  public:
	ScreenRecorder();
	~ScreenRecorder();

	/* function to initiate communication with display library */
	int init();
	int init_outputfile();
	int CaptureStart();
    void PauseCapture();
    void ResumeCapture();
    bool isPaused();
    void close();

	//  ----------------

	//  static int sfp_refresh_thread(void *opaque);
	//  void show_avfoundation_device();
	//  int start_recording();
};
