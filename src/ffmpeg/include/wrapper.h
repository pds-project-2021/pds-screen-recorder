//
// Created by gabriele on 31/10/21.
//

#pragma once

#include <memory>
#include <iostream>

#include "ffmpegc.h"
#include "Tracker.h"

using namespace std;

template <typename T>
class wrapper: public Tracker<wrapper<T>> {
  private:
	T* audio = nullptr;
	T* video = nullptr;

  public:
	wrapper();
	~wrapper();

	T* get_audio();
	T* get_video();

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
T* wrapper<T>::get_audio() {
	return audio;
}

/**
 * Get video instance
 * @return a pointer to the video instance of T
 */
template<typename T>
T* wrapper<T>::get_video() {
	return video;
}

/**
 * Set video instance
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
 * Destructor of `AVFormatContext`
 */
template<> inline
wrapper<AVFormatContext>::~wrapper() {
	if (audio != nullptr){
		avformat_close_input(&audio);
		avformat_free_context(audio);
	}

	if(video != nullptr){
		avformat_close_input(&video);
		avformat_free_context(video);
	}
}

/**
 * Destructor of `AVCodecContext`
 */
template<> inline
wrapper<AVCodecContext>::~wrapper() {
	cout << "delete codec context" << endl;
	if(audio != nullptr){
		avcodec_free_context(&audio);
	}

	if(video != nullptr){
		avcodec_free_context(&video);
	}
}

/**
 * Destructor of `AVDictionary`
 */
template<> inline
wrapper<AVDictionary>::~wrapper() {
	cout << "delete codec context" << endl;
	if(audio != nullptr){
		av_dict_free(&audio);
	}

	if(video != nullptr){
		av_dict_free(&video);
	}
}







