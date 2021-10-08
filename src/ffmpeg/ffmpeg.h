//
// Created by gabriele on 07/10/21.
//

#ifndef PDS_SCREEN_RECORDER_FFMPEG_H
#define PDS_SCREEN_RECORDER_FFMPEG_H

#ifdef _WIN32
// Windows
extern "C" {
#include "SDL2/SDL.h"
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}
#else
// Linux and MacOS
#ifdef __cplusplus
extern "C" {
#endif
#include <SDL2/SDL.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
}
#endif
#endif

#endif // PDS_SCREEN_RECORDER_FFMPEG_H
