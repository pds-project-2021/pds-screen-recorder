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
#include "Rescaler.h"
#include "Frame.h"
#include "Packet.h"
#include "Screen.h"

/* Very low level C-like functions */

int decode(AVCodecContext *avctx, AVPacket *pkt, AVFrame *frame, int *got_frame);
int encode(AVCodecContext *avctx, AVPacket *pkt, AVFrame *frame, int *got_packet);

void writeFrameToOutput(AVFormatContext *outputFormatContext,
                        AVPacket *outPacket,
                        std::mutex *wR);
// Video functions

int
convertAndWriteVideoFrame(SwsContext *swsContext, AVCodecContext *outputCodecContext, AVCodecContext *inputCodecContext,
                          AVStream *videoStream, AVFormatContext *outputFormatContext, AVFrame *frame,
                          int64_t *pts_p, std::mutex *wR, std::mutex *r, int64_t *mx_pts, int64_t *mn_pts,
                          std::atomic<bool> *paused, bool resync);

int convertAndWriteDelayedVideoFrames(AVCodecContext *outputCodecContext, AVStream *videoStream,
                                      AVFormatContext *outputFormatContext, std::mutex *wR);

// Audio functions

int convertAndWriteAudioFrames(SwrContext *swrContext, AVCodecContext *outputCodecContext,
                               AVCodecContext *inputCodecContext, AVStream *audioStream,
                               AVFormatContext *outputFormatContext, AVFrame *frame, int64_t *pts_p, std::mutex *wR,
                               std::mutex *r, int64_t *mx_pts, int64_t *mn_pts, std::atomic<bool> *paused, bool resync);

int convertAndWriteLastAudioFrames(SwrContext *swrContext, AVCodecContext *outputCodecContext,
                                   AVCodecContext *inputCodecContext, AVStream *audioStream,
                                   AVFormatContext *outputFormatContext, int64_t *pts_p, std::mutex *wR);

int convertAndWriteDelayedAudioFrames(AVCodecContext *inputCodecContext, AVCodecContext *outputCodecContext,
                                      AVStream *audioStream, AVFormatContext *outputFormatContext, int finalSize,
                                      std::mutex *wR);

