//
// Created by gabriele on 07/10/21.
//

/**
 * lib wrapper with cpp class and utilities
 */

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <string>
#include <cmath>
#include <cstring>
#include <memory>
#include <thread>
#include <atomic>


// import top level exceptions
#include "exceptions.h"

// import ffmpeg C headers
#include "ffmpegc.h"

#include "platform.h"
#include "Codec.h"
#include "Format.h"
#include "Dictionary.h"
#include "Stream.h"
#include "Rescaler.h"
#include "Frame.h"
#include "Packet.h"
#include "Screen.h"

/* Very low level C-like functions */

int decode(AVCodecContext *avctx, AVPacket *pkt, AVFrame *frame, int *got_frame);
int encode(AVCodecContext *avctx, AVPacket *pkt, AVFrame *frame, int *got_packet);


void writeFrameToOutput(AVFormatContext *outputFormatContext,
                        AVPacket *outPacket,
                        std::mutex *wR,
                        std::condition_variable *writeFrame);
// Video functions

int convertAndWriteVideoFrame(SwsContext *swsContext,
                              AVCodecContext *outputCodecContext,
                              AVCodecContext *inputCodecContext,
                              AVStream *videoStream,
                              AVFormatContext *outputFormatContext,
                              AVFrame *frame,
                              const int64_t *pts_p,
                              std::mutex *wR,
                              std::condition_variable *writeFrame);

int convertAndWriteDelayedVideoFrames(AVCodecContext *outputCodecContext,
                                      AVStream *videoStream,
                                      AVFormatContext *outputFormatContext,
                                      std::mutex *wR,
                                      std::condition_variable *writeFrame);

// Audio functions

int convertAndWriteAudioFrames(SwrContext *swrContext,
                                      AVCodecContext *outputCodecContext,
                                      AVCodecContext *inputCodecContext,
                                      AVStream *audioStream,
                                      AVFormatContext *outputFormatContext,
                                      AVFrame *audioFrame,
                                      int64_t *pts_p,
                                      std::mutex *wR,
                                      std::condition_variable *writeFrame);

int convertAndWriteLastAudioFrames(SwrContext *swrContext,
                                   AVCodecContext *outputCodecContext,
                                   AVCodecContext *inputCodecContext,
                                   AVStream *audioStream,
                                   AVFormatContext *outputFormatContext,
                                   int64_t *pts_p,
                                   std::mutex *wR,
                                   std::condition_variable *writeFrame);


int convertAndWriteDelayedAudioFrames(AVCodecContext *inputCodecContext,
                                      AVCodecContext *outputCodecContext,
                                      AVStream *audioStream,
                                      AVFormatContext *outputFormatContext,
                                      int finalSize,
                                      std::mutex *wR,
                                      std::condition_variable *writeFrame);

