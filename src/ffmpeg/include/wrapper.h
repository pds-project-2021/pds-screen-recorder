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
	shared_ptr<T*> audio = nullptr;
	shared_ptr<T*> video = nullptr;

  public:
	wrapper();
	~wrapper();

	shared_ptr<T*> get_audio();
	shared_ptr<T*> get_video();

	void set_audio(shared_ptr<T*> ptr);
	void set_video(shared_ptr<T*> ptr);

	void set_audio(T* prt);
	void set_video(T* prt);

};

/* General template functions */

template<typename T>
wrapper<T>::wrapper() = default;

template<typename T>
wrapper<T>::~wrapper() = default;



/**
 * Get audio instance
 * @return an instance of audio T
 */
template<typename T>
shared_ptr<T*> wrapper<T>::get_audio() {
	return audio;
}

/**
 * Get video instance
 * @return an instance of video T
 */
template<typename T>
shared_ptr<T*> wrapper<T>::get_video() {
	return video;
}

template<typename T>
void wrapper<T>::set_audio(shared_ptr<T *> ptr) {
	audio = ptr;
}

template<typename T>
void wrapper<T>::set_video(shared_ptr<T *> ptr) {
	video = ptr;
}

template<typename T>
void wrapper<T>::set_audio(T* ptr) {
	audio = make_shared<T*>(ptr);
}

template<typename T>
void wrapper<T>::set_video(T* ptr) {
	video = make_shared<T*>(ptr);
}


/* Specialized template functions */

/**
 * Constructor of `AVFormatContext`
 */
template<> inline
wrapper<AVFormatContext>::wrapper() {
	video = make_shared<AVFormatContext*>(avformat_alloc_context());
	audio = make_shared<AVFormatContext*>(avformat_alloc_context());
}

/**
 * Destructor of `AVFormatContext`
 */
template<> inline
wrapper<AVFormatContext>::~wrapper() {
	avformat_close_input(video.get());
	avformat_free_context(*video);
	avformat_close_input(audio.get());
	avformat_free_context(*audio);
}






