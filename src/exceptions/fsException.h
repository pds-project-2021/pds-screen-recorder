//
// Created by gabriele on 07/10/21.
//

#ifndef PDS_SCREEN_RECORDER_FSEXCEPTION_H
#define PDS_SCREEN_RECORDER_FSEXCEPTION_H

#include <stdexcept>

class fsException: public std::runtime_error{
public:
    explicit fsException(const std::string &arg) : runtime_error(arg){}
};

#endif // PDS_SCREEN_RECORDER_FSEXCEPTION_H
