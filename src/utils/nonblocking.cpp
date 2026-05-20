#include "utils/nonblocking.hpp"

#include <fcntl.h>
#include <unistd.h>

namespace utils {

bool set_nonblocking(const int fd) {
    int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }
    flags = ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    return flags >= 0;
}

} // namespace utils
