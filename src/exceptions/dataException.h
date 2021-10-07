//
// Created by gabriele on 07/10/21.
//

#ifndef PDS_SCREEN_RECORDER_DATAEXCEPTION_H
#define PDS_SCREEN_RECORDER_DATAEXCEPTION_H

#include <stdexcept>

class dataException : public std::runtime_error{
public:
    explicit dataException(const std::string &arg) : runtime_error(arg) {}
};

#endif // PDS_SCREEN_RECORDER_DATAEXCEPTION_H
