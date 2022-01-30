//
// Created by gabriele on 31/10/21.
//

#pragma once

// import library top level exceptions
#include "exceptions.h"
#include "utils.h"

#include "ffmpegc.h"
#include "Tracker.h"

//using namespace std;

template <typename T>
class wrapper: public Tracker<wrapper<T>> {
  protected:
	T* audio = nullptr;
	T* video = nullptr;

  public:
	wrapper();
	~wrapper();

	T* get_audio() const;
	T* get_video() const;

	void set_audio(T* ptr);
	void set_video(T* ptr);
};

/* General template functions */

template<typename T>
wrapper<T>::wrapper() = default;

template<typename T>
wrapper<T>::~wrapper() = default;

/**
 * Get audio instance
 * @return a pointer to the audio instance of T
 */
template<typename T>
T * wrapper<T>::get_audio() const {
	return audio;
}

/**
 * Get video instance
 * @return a pointer to the video instance of T
 */
template<typename T>
T * wrapper<T>::get_video() const {
	return video;
}

/**
 * Set audio instance
 * @param ptr pointer of the audio instance of T
 */
template<typename T>
void wrapper<T>::set_audio(T* ptr) {
	audio = ptr;
}

/**
 * Set video instance
 * @param ptr pointer of the video instance of T
 */
template<typename T>
void wrapper<T>::set_video(T* ptr) {
	video = ptr;
}

/* Specialized template functions */

/**
 * Get audio instance
 * @return a pointer to the audio instance of T or the video instance if is an output context
 */
template<> inline
AVFormatContext * wrapper<AVFormatContext>::get_audio() const {
	if(audio == nullptr && video != nullptr) { // for output there is only one context
		return video;
	}else{
		return audio;
	}
}

/**
 * Destructor of `AVFormatContext`
 */
template<>
inline
wrapper<AVFormatContext>::~wrapper() {
	if (audio != nullptr) {
		avformat_close_input(&audio);
		avformat_free_context(audio);
	}

	if (video != nullptr) {
		if (is_file(video->url) && !(video->flags & AVFMT_NOFILE)) {
			avio_close(video->pb);
		}else{
			avformat_close_input(&video);
		}
		avformat_free_context(video);
	}
}

/**
 * Destructor of `AVCodecContext`
 */
template<> inline
wrapper<AVCodecContext>::~wrapper() {
	if(audio != nullptr){
		avcodec_close(audio);
		avcodec_free_context(&audio);
	}

	if(video != nullptr){
		avcodec_close(video);
		avcodec_free_context(&video);
	}
}

/**
 * Destructor of `AVDictionary`
 */
template<> inline
wrapper<AVDictionary>::~wrapper() {
	if(audio != nullptr){
		av_dict_free(&audio);
	}

	if(video != nullptr){
		av_dict_free(&video);
	}
}


