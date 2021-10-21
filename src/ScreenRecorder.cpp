#include "ScreenRecorder.h"

// ScreenRecorder::ScreenRecorder() {
//  thread_exit = 0;
//
//  avformat_network_init();
//  pFormatCtx = avformat_alloc_context();
//  // Open File
//  // char filepath[]="src01_480x272_22.h265";
//  // avformat_open_input(&pFormatCtx,filepath,nullptr,nullptr)
//
//  // Register Device
//  avdevice_register_all();
//
//  // Windows
//#ifdef _WIN32
//  // Use gdigrab
//  options = nullptr;
//  // Set some options
//  // grabbing frame rate
//  // av_dict_set(&options,"framerate","5",0);
//  // The distance from the left edge of the screen or desktop
//  // av_dict_set(&options,"offset_x","20",0);
//  // The distance from the top edge of the screen or desktop
//  // av_dict_set(&options,"offset_y","40",0);
//  // Video frame size. The default is to capture the full screen
//  // av_dict_set(&options,"video_size","640x480",0);
//  ifmt = av_find_input_format("gdigrab");
//  if (avformat_open_input(&pFormatCtx, "desktop", ifmt, &options) != 0) {
//    throw avException("Couldn't open input stream");
//  }
//#elif defined linux
//  // Linux
//  options = nullptr;
//  // Set some options
//  // grabbing frame rate
//  // av_dict_set(&options,"framerate","5",0);
//  // Make the grabbed area follow the mouse
//  // av_dict_set(&options,"follow_mouse","centered",0);
//  // Video frame size. The default is to capture the full screen
//  // av_dict_set(&options,"video_size","640x480",0);
//  ifmt = av_find_input_format("x11grab");
//  // Grab at position 10,20
//  if (avformat_open_input(&pFormatCtx, ":0.0+0.0", ifmt, &options) != 0) {
//    throw avException("Couldn't open input stream");
//  }
//#else
//  show_avfoundation_device();
//  // Mac
//  ifmt = av_find_input_format("avfoundation");
//  // Avfoundation
//  //[video]:[audio]
//  if (avformat_open_input(&pFormatCtx, "1", ifmt, nullptr) != 0) {
//    throw avException("Couldn't open input stream");
//  }
//#endif
//}
using namespace std;


static void fill_yuv_image(uint8_t *data[4], int linesize[4],
                           int width, int height, int frame_index)
{
    int x, y;

    /* Y */
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            data[0][y * linesize[0] + x] = x + y + frame_index * 3;

    /* Cb and Cr */
    for (y = 0; y < height / 2; y++) {
        for (x = 0; x < width / 2; x++) {
            data[1][y * linesize[1] + x] = 128 + y + frame_index * 2;
            data[2][y * linesize[2] + x] = 64 + x + frame_index * 5;
        }
    }
}

int decode(AVCodecContext *avctx, AVFrame *frame, int *got_frame, AVPacket *pkt)
{
  int ret;
  *got_frame = 0;

  if (pkt) {
    ret = avcodec_send_packet(avctx, pkt);
    // In particular, we don't expect AVERROR(EAGAIN), because we read all
    // decoded frames with avcodec_receive_frame() until done.
    if (ret < 0)
      return ret == AVERROR_EOF ? 0 : ret;
  }

  ret = avcodec_receive_frame(avctx, frame);
  if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
    return ret;
  if (ret >= 0)
    *got_frame = 1;

  return 0;
}

int encode(AVCodecContext *avctx, AVPacket *pkt, int *got_packet, AVFrame *frame)
{
    int ret;

    *got_packet = 0;

    ret = avcodec_send_frame(avctx, frame);
    if (ret < 0)
        return ret;

   ret = avcodec_receive_packet(avctx, pkt);
    if (!ret)
       *got_packet = 1;
    if (ret == AVERROR(EAGAIN))
        return 0;

    return ret;
}

/* initialize the resources*/
ScreenRecorder::ScreenRecorder() {
  av_register_all();
  avcodec_register_all();
  avdevice_register_all();
}

ScreenRecorder::~ScreenRecorder() {
  avformat_close_input(&inputFormatContext);
  if (inputFormatContext) {
    throw avException("Unable to close input");
  }

  avformat_free_context(inputFormatContext);
  if (inputFormatContext) {
    throw avException("Unable to free avformat context");
  }

  cout << "clean all" << endl;
}

int ScreenRecorder::init() {
  inputFormatContext = avformat_alloc_context(); // Allocate an AVFormatContext.

  options = nullptr;
  av_dict_set(&options, "framerate", "30", 0);
  av_dict_set(&options, "preset", "medium", 0);
  av_dict_set(&options, "offset_x", "0", 0);
  av_dict_set(&options, "offset_y", "0", 0);
  //av_dict_set(&options, "video_size", "1920x1080", 0);
  av_dict_set(&options, "show_region", "1", 0);

#ifdef _WIN32
#if USE_DSHOW
    options = nullptr;
    inputFormat = av_find_input_format("dshow");
    if (avformat_open_input(&inputFormatContext, "video=screen-capture-recorder", inputFormat, &options) != 0) {
        throw avException("Couldn't open input stream");
    }
#else
    options = nullptr;
    inputFormat = av_find_input_format("gdigrab");
    if (avformat_open_input(&inputFormatContext, "desktop", inputFormat, &options) != 0) {
        throw avException("Couldn't open input stream");
        }
#endif
#elif defined linux
    options = nullptr;
    inputFormat = av_find_input_format("x11grab");
    if (avformat_open_input(&inputFormatContext, ":0.0+0,0", inputFormat, &options) != 0) {
        throw avException("Couldn't open input stream");
      }
#else
    show_avfoundation_device();
    inputFormat = av_find_input_format("avfoundation");
    if (avformat_open_input(&inputFormatContext, "1", inputFormat, nullptr) != 0) {
        throw avException("Couldn't open input stream");
      }
#endif
  auto ret = avformat_find_stream_info(inputFormatContext, &options);
  if (ret < 0) {
    throw avException("Unable to find the stream information");
  }

  auto index = av_find_best_stream(inputFormatContext, AVMEDIA_TYPE_VIDEO, -1,-1, nullptr, 0);

  if (index == -1) {
    throw avException("Unable to find the video stream index. (-1)");
  }

  inputCodecPar = inputFormatContext->streams[index]->codecpar;
  inputCodecPar->format = AV_PIX_FMT_YUV444P;

  inputCodec = avcodec_find_decoder(inputCodecPar->codec_id);
  if (inputCodec == nullptr) {
    throw avException("Unable to find the decoder");
  }

  inputCodecContext = avcodec_alloc_context3(inputCodec);
  if (inputCodecContext == nullptr) {
    throw avException("Unable to get input codec context");
  }
  avcodec_parameters_to_context(inputCodecContext, inputCodecPar);

  ret = avcodec_open2(inputCodecContext, inputCodec,nullptr); // Initialize the AVCodecContext to use the given AVCodec.
  if (ret < 0) {
    throw avException("Unable to open the av codec");
  }
#ifdef _WIN32
    av_dump_format(inputFormatContext, 0, "desktop", 0);
#elif defined linux
    av_dump_format(inputFormatContext, 0, ":0.0+0,0", 0);
#else
    av_dump_format(inputFormatContext, 0, "1", 0);
#endif
}

int ScreenRecorder::init_outputfile() {
  output_file = "../output.mp4";

  outputCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
  if (!outputCodec) {
    throw avException(
        "Error in finding the av codecs. try again with correct codec");
  }

  avformat_alloc_output_context2(&outputFormatContext, nullptr, nullptr, output_file);
  if (!outputFormatContext) {
    throw avException("Error in allocating av format output context");
  }

  videoStream = avformat_new_stream(outputFormatContext, outputCodec);
  if (!videoStream) {
    throw avException("Error in creating a av format new stream");
  }
  videoStream->time_base = {1, 30};

  /* Returns the output format in the list of registered output formats which
   * best matches the provided parameters, or returns nullptr if there is no
   * match.
   */
  outputFormat = av_guess_format(nullptr, output_file, nullptr);
  if (!outputFormat) {
    throw avException(
        "Error in guessing the video format. try with correct format");
  }

  outputCodecContext = avcodec_alloc_context3(outputCodec);
  if (!outputCodecContext) {
    throw avException("Error in allocating the codec context");
  }
  outputCodecContext->gop_size = 3;
  outputCodecContext->max_b_frames = 2;
  outputCodecContext->time_base = videoStream->time_base;

  /* set property of the video file */
  outputCodecPar = videoStream->codecpar;
  outputCodecPar->codec_id = AV_CODEC_ID_H264; // AV_CODEC_ID_MPEG4; AV_CODEC_ID_H264; AV_CODEC_ID_MPEG1VIDEO;
  outputCodecPar->codec_type = AVMEDIA_TYPE_VIDEO;
  outputCodecPar->format = AV_PIX_FMT_YUV420P;
  outputCodecPar->bit_rate = 800000; // 2500000
  outputCodecPar->width = inputCodecPar->width;
  outputCodecPar->height = inputCodecPar->height;

  auto ret = avcodec_parameters_to_context(outputCodecContext,outputCodecPar);
  if (ret < 0) {
    throw avException("Unable to get output codec context");
  }

  /* Some container formats (like MP4) require global headers to be present
 Mark the encoder so that it behaves accordingly. */

  if (outputFormatContext->oformat->flags & AVFMT_GLOBALHEADER) {
    outputCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }

  if (codec_id == AV_CODEC_ID_H264) {
    av_opt_set(outputCodecContext->priv_data, "preset", "slow", 0);
  }

  ret = avcodec_open2(outputCodecContext, outputCodec, nullptr);
  if (ret < 0) {
    throw avException("Error in opening the avcodec");
  }

  /* create empty video file */
  if (!(outputFormatContext->flags & AVFMT_NOFILE)) {
    ret = avio_open2(&(outputFormatContext->pb), output_file, AVIO_FLAG_WRITE,
                      nullptr, nullptr);
    if (ret < 0) {
        char buf[35];
        av_strerror(ret, buf, sizeof(buf));
        throw avException(buf);
    }
  }

//  if (!outputFormatContext->nb_streams) {
//    throw avException("Output file dose not contain any stream");
//  }

  swsContext = sws_getCachedContext(swsContext, inputCodecPar->width, inputCodecPar->height,
                              (AVPixelFormat)inputCodecPar->format,
                              outputCodecPar->width, outputCodecPar->height,
                              (AVPixelFormat)outputCodecPar->format,
                              SWS_BICUBIC, nullptr, nullptr, nullptr);
    if (!swsContext) {
        throw avException("Impossible to create scale context for the conversion");
    }
  /* imp: mp4 container or some advanced container file required header
   * information*/
  ret = avformat_write_header(outputFormatContext, &options);
  if (ret < 0) {
    throw avException("Error in writing the header context");
  }

  // file informations
  av_dump_format(outputFormatContext, 0, output_file, 1);
}

/* function to capture and store data in frames by allocating required memory
 * and auto deallocating the memory.   */
int ScreenRecorder::CaptureVideoFrames() {
  // decoder frame
  AVFrame *frame = av_frame_alloc();
  if (!frame) {
    throw avException("Unable to release the avframe resources");
  }
    frame->data[0] = nullptr;
    frame->width = inputCodecPar->width;
    frame->height = inputCodecPar->height;
    frame->format = inputCodecPar->format;
    frame->pts = 0;
    if (av_image_alloc(frame->data, frame->linesize,
                       inputCodecContext->width, inputCodecContext->height,
                       (AVPixelFormat)inputCodecPar->format, 32) < 0) {
        throw avException("Error in allocating frame data");
    }
  // encoder frame
  AVFrame *outputFrame = av_frame_alloc();
  if (!outputFrame) {
    throw avException(
        "Unable to release the avframe resources for outputFrame");
  }
  outputFrame->data[0] = nullptr;
  outputFrame->width = outputCodecPar->width;
  outputFrame->height = outputCodecPar->height;
  outputFrame->format = outputCodecPar->format;
  outputFrame->pts = 0;


if (av_image_alloc(outputFrame->data, outputFrame->linesize,
                   outputCodecContext->width, outputCodecContext->height,
                   (AVPixelFormat)outputFrame->format, 32) < 0) {
    throw avException("Error in allocating frame data");
}

  // Setup the data pointers and linesizes based on the specified image
  // parameters and the provided array.
  //  value = av_image_fill_arrays(
  //      outputFrame->data, outputFrame->linesize, video_outbuf,
  //      AV_PIX_FMT_YUV420P, outputCodecContext->width,
  //      outputCodecContext->height, 1); // returns : the size in bytes
  //      required for src
  //  if (value < 0) {
  //    throw avException("Error in filling image array");
  //  }

  //  SwsContext *swsCtx_;

  // Allocate and return swsContext.
  // a pointer to an allocated context, or nullptr in case of error
  // Deprecated : Use sws_getCachedContext() instead.
  //  swsCtx_ = sws_getContext(
  //      inputCodecContext->width, inputCodecContext->height,
  //      inputCodecContext->pix_fmt, outputCodecContext->width,
  //      outputCodecContext->height, outputCodecContext->pix_fmt, SWS_BICUBIC,
  //      nullptr, nullptr, nullptr);

  int count = 0;
  int frameCount = 300;

  int got_picture;

  int j = 0;
  int frameFinished;

  AVPacket *packet = av_packet_alloc();
  if (!packet) {
    throw avException("Error on packet initialization");
  }
  AVPacket *outPacket= av_packet_alloc();
  if (!outPacket) {
      throw avException("Error on packet initialization");
  }
  while (av_read_frame(inputFormatContext, packet) >= 0) {
    if (count++ == frameCount) {
    break;
    }

    if (packet->stream_index == videoStream->index) {
//      avcodec_decode_video2(inputCodecContext, frame, &frameFinished, packet);
        int ret;
        //= decode(inputCodecContext, frame, &frameFinished, packet);
        fill_yuv_image(frame->data, frame->linesize, inputCodecPar->width, inputCodecPar->height, count);
//      auto result = avcodec_send_packet(inputCodecContext, packet);
//      if (result == AVERROR(EAGAIN)) {}
//      else{
//        throw avException("Failed to send packet to decoder");
//      }
//      result = avcodec_receive_frame(inputCodecContext, frame);
  //    if (ret < 0) {
  //      throw avException("Unable to decode");
  //    }
      if(frameFinished=1){
        sws_scale(swsContext, frame->data, frame->linesize, 0,
                  inputCodecContext->height, (uint8_t * const *)outputFrame->data,
                 outputFrame->linesize);

        encode(outputCodecContext, outPacket, &got_picture, outputFrame);

//        got_picture = avcodec_encode_video2(outputCodecContext, &outPacket, outputFrame, &got_picture);
//        ret = avcodec_send_frame(outputCodecContext, outputFrame);
//        if (ret < 0) {
//          cout << ret << endl;
//          throw avException("Failed to send frame from encoder");
//        }
//
//        ret = avcodec_receive_packet(outputCodecContext, &outPacket);
//        if (ret < 0) {
//          char buff[100];
//          av_strerror(ret, buff, 100);
//          cout << buff << endl;
//          throw avException("Failed to receive packet from encoder");
//        }

        if(got_picture){
          if (outPacket->pts != AV_NOPTS_VALUE) {
            outPacket->pts =
                av_rescale_q(outPacket->pts, outputCodecContext->time_base,videoStream->time_base);
          }
          if (outPacket->dts != AV_NOPTS_VALUE) {
            outPacket->dts =
                av_rescale_q(outPacket->dts, outputCodecContext->time_base,videoStream->time_base);
          }

//          cout << "Write frame " << j++ << " (size= " << outPacket.size / 1000 << ")" << endl;

          ret = av_write_frame(outputFormatContext, outPacket);
          if (ret != 0) {
            throw avException("Error in writing video frame");
          }

        } // got_picture
          av_packet_unref(outPacket);

      } // frameFinished
    }
  } // End of while-loop

  auto ret = av_write_trailer(outputFormatContext);
  if (ret < 0) {
    throw avException("Error in writing av trailer");
  }

  // THIS WAS ADDED LATER
//    av_free(outBuffer);
  cout << "qua" << endl;

  av_dump_format(outputFormatContext, 0,"../output.txt", 1);
/*
    cout << "qua" << endl;
    FILE * f = fopen("../output.mp4", "a+");
    uint8_t outbuf[4];
    outbuf[0] = 0x00;
    outbuf[1] = 0x00;
    outbuf[2] = 0x01;
    outbuf[3] = 0xb7;
    fwrite(outbuf, 1, 4, f);
    fclose(f);
*/

}


// ------------------------------------

// int ScreenRecorder::sfp_refresh_thread(void *opaque) {
//   thread_exit = 0;
//   while (1) {
//     SDL_Event event;
//     event.type = SFM_REFRESH_EVENT;
//     SDL_PushEvent(&event);
//     SDL_Delay(40);
//   }
//   thread_exit = 0;
//  Break
//   SDL_Event event;
//   event.type = SFM_BREAK_EVENT;
//   SDL_PushEvent(&event);
//
//   return 0;
//}

// Show AVFoundation Device
// void ScreenRecorder::show_avfoundation_device() {
//  pFormatCtx = avformat_alloc_context();
//  options = nullptr;
//  av_dict_set(&options, "list_devices", "true", 0);
//  AVInputFormat *iformat = av_find_input_format("avfoundation");
//  printf("==AVFoundation Device Info===\n");
//  avformat_open_input(&pFormatCtx, "", iformat, &options);
//  printf("=============================\n");
//}
//
// int ScreenRecorder::start_recording() {
//  if (avformat_find_stream_info(pFormatCtx, nullptr) < 0) {
//    throw avException("Couldn't find stream information");
//  }
//  video_index = -1;
//  for (i = 0; i < pFormatCtx->nb_streams; i++)
//    if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
//      video_index = i;
//      break;
//    }
//  if (video_index == -1) {
//    throw avException("Didn't find a video stream");
//  }
//  pCodecPar = pFormatCtx->streams[video_index]->codecpar;
//  pCodec = avcodec_find_decoder(pCodecPar->codec_id);
//  auto pCodecCtx = pFormatCtx->streams[video_index]->codec;
//  if (pCodec == nullptr) {
//    throw avException("Codec not found");
//  }
//  if (avcodec_open2(pCodecCtx, pCodec, nullptr) < 0) {
//    throw avException("Could not open codec");
//  }
//  AVFrame *pFrame, *pFrameYUV;
//  pFrame = av_frame_alloc();
//  pFrameYUV = av_frame_alloc();
//  // unsigned char *out_buffer=(unsigned char
//  // *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width,
//  // pCodecCtx->height)); avpicture_fill((AVPicture *)pFrameYUV, out_buffer,
//  // AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
//  // SDL----------------------------
//  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
//    throw avException(std::string("Could not initialize SDL - ") +
//                          SDL_GetError());
//  }
//
//  int screen_w = 640, screen_h = 360;
//  SDL_DisplayMode *dm;
//  SDL_GetCurrentDisplayMode(0, dm);
//  // Half of the Desktop's width and height.
//  screen_w = dm->w / 2;
//  screen_h = dm->h / 2;
//  SDL_Window *screen;
//  screen = SDL_CreateWindow("Video Recording", 0, 0, screen_w, screen_h, 0);
//
//  if (!screen) {
//    throw avException(
//        std::string("SDL: could not set video mode - exiting:") +
//        SDL_GetError());
//  }
//  SDL_Renderer *renderer = SDL_CreateRenderer(screen, -1, 0);
//  if (!renderer) {
//    throw avException(
//        std::string("SDL: could not create renderer - exiting:") +
//        SDL_GetError());
//  }
//
//  SDL_Texture *txr = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12,
//                                       SDL_TEXTUREACCESS_STREAMING,
//                                       pCodecCtx->width, pCodecCtx->height);
//  SDL_Rect rect;
//  rect.x = 0;
//  rect.y = 0;
//  rect.w = screen_w;
//  rect.h = screen_h;
//  // SDL End------------------------
//  int ret, got_picture;
//
//  AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
//
//  struct SwsContext *img_convert_ctx;
//  img_convert_ctx =
//      sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
//                     pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P,
//                     SWS_BICUBIC, nullptr, nullptr, nullptr);
//  //------------------------------
//  SDL_Thread *video_tid =
//      SDL_CreateThread(sfp_refresh_thread, "video", nullptr);
//  // Event Loop
//  SDL_Event event;
//
//  for (;;) {
//    // Wait
//    SDL_WaitEvent(&event);
//    if (event.type == SFM_REFRESH_EVENT) {
//      //------------------------------
//      if (av_read_frame(pFormatCtx, packet) >= 0) {
//        if (packet->stream_index == video_index) {
//          // TODO update deprecated call
//          ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture,
//          packet); if (ret < 0) {
//            throw avException("Decode Error");
//          }
//          if (got_picture) {
//            //            SDL_LockYUVOverlay(bmp);
//            //            pFrameYUV->data[0] = bmp->pixels[0];
//            //            pFrameYUV->data[1] = bmp->pixels[2];
//            //            pFrameYUV->data[2] = bmp->pixels[1];
//            //            pFrameYUV->linesize[0] = bmp->pitches[0];
//            //            pFrameYUV->linesize[1] = bmp->pitches[2];
//            //            pFrameYUV->linesize[2] = bmp->pitches[1];
//            //            sws_scale(img_convert_ctx,
//            //                      (const unsigned char *const
//            *)pFrame->data,
//            //                      pFrame->linesize, 0, pCodecCtx->height,
//            //                      pFrameYUV->data, pFrameYUV->linesize);
//            //
//            //            SDL_UnlockYUVOverlay(bmp);
//            //
//            //            SDL_DisplayYUVOverlay(bmp, &rect);
//          }
//        }
//        av_packet_unref(packet);
//      } else {
//        // Exit Thread
//        //        thread_exit = 1;
//      }
//    } else if (event.type == SDL_QUIT) {
//      //      thread_exit = 1;
//    } else if (event.type == SFM_BREAK_EVENT) {
//      break;
//    }
//  }
//
//  sws_freeContext(img_convert_ctx);
//  SDL_Quit();
//
//  av_free(pFrameYUV);
//  avcodec_close(pCodecCtx);
//  avformat_close_input(&pFormatCtx);
//  return 0;
//}
