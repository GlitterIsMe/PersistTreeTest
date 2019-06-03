//
// Created by 张艺文 on 19.6.3.
//

#ifndef PERSISTTREETEST_ERROR_H
#define PERSISTTREETEST_ERROR_H

#include <exception>
using std::exception;

namespace rocksdb {
class AllocatorException : public exception {
    const char *what() const noexcept {
        return "Allocator Exception";
    }
};

class AllocatorMapFailed : public AllocatorException {
    const char *what() const noexcept {
        return "NVM file mapped failed";
    }
};

class AllocatorNotPM : public AllocatorException {
    const char *what() const noexcept {
        return "Using addr not a persistent memory";
    }
};

class AllocatorSpaceRunOut : public AllocatorException {
    const char *what() const noexcept {
        return "NVM space has run out";
    }
};
}

#endif //PERSISTTREETEST_ERROR_H
