//
// Created by gabriele on 07/10/21.
//

#ifndef PDS_SCREEN_RECORDER_AVEXCEPTION_H
#define PDS_SCREEN_RECORDER_AVEXCEPTION_H

#include <stdexcept>

class avException : public std::runtime_error{
public:
    explicit avException(const std::string &arg) : runtime_error(arg) {}
};


#endif // PDS_SCREEN_RECORDER_AVEXCEPTION_H
