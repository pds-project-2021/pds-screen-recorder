//
// Created by gabriele on 07/10/21.
//

/**
 * ffmpeg wrapper with cpp class and utilities
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

#include "ffmpegc.h"

#include "platform.h"
#include "Codec.h"
#include "Format.h"
#include "Dictionary.h"
#include "Stream.h"
#include "Rescaler.h"
#include "Frame.h"
#include "Packet.h"

/* Very low level C-like fuctions */

int decode(AVCodecContext *avctx, AVPacket *pkt, AVFrame *frame, int *got_frame);
int encode(AVCodecContext *avctx, AVPacket *pkt, AVFrame *frame, int *got_packet);

void writeFrameToOutput(AVFormatContext *outputFormatContext,
                        AVPacket *outPacket,
                        mutex *wR,
                        condition_variable *writeFrame);

int convertAndWriteVideoFrame(SwsContext *swsContext,
                              AVCodecContext *outputCodecContext,
                              AVCodecContext *inputCodecContext,
                              AVStream *videoStream,
                              AVFormatContext *outputFormatContext,
                              AVFrame *frame,
                              const int64_t *pts_p,
                              mutex *wR,
                              condition_variable *writeFrame);

int convertAndWriteDelayedVideoFrames(AVCodecContext *outputCodecContext,
                                      AVStream *videoStream,
                                      AVFormatContext *outputFormatContext,
                                      mutex *wR,
                                      condition_variable *writeFrame);

int convertAndWriteDelayedAudioFrames(AVCodecContext *inputCodecContext,
                                             AVCodecContext *outputCodecContext,
                                             AVStream *audioStream,
                                             AVFormatContext *outputFormatContext,
                                             int finalSize,
                                             mutex *wR,
                                             condition_variable *writeFrame);

int convertAndWriteAudioFrames(SwrContext *swrContext,
                                      AVCodecContext *outputCodecContext,
                                      AVCodecContext *inputCodecContext,
                                      AVStream *audioStream,
                                      AVFormatContext *outputFormatContext,
                                      AVFrame *audioFrame,
                                      int64_t *pts_p,
                                      mutex *wR,
                                      condition_variable *writeFrame);

int convertAndWriteDelayedAudioFrames(AVCodecContext *inputCodecContext,
                                             AVCodecContext *outputCodecContext,
                                             AVStream *audioStream,
                                             AVFormatContext *outputFormatContext,
                                             int finalSize,
                                             mutex *wR,
                                             condition_variable *writeFrame);

int convertAndWriteLastAudioFrames(SwrContext *swrContext,
                                   AVCodecContext *outputCodecContext,
                                   AVCodecContext *inputCodecContext,
                                   AVStream *audioStream,
                                   AVFormatContext *outputFormatContext,
                                   int64_t *pts_p,
                                   mutex *wR,
                                   condition_variable *writeFrame);