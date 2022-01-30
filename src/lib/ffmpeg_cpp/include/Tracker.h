//
// Created by gabriele on 31/10/21.
//

#pragma once

#include <iostream>

//using namespace std;

template <class T>
class Tracker {
	static inline unsigned int count = 0;
	friend T;

  public:
	Tracker(){count++;}
	~Tracker(){count--;}

	static void print_count(){
		std::cout << typeid(T).name() << ": " << count << std::endl;
	}
};


