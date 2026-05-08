#include "uart_channel.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <stdexcept>
#include <cerrno>
#include <cstring>

using namespace serialipmi::uart;

Channel::Channel(const std::string& devicePath_, uint32_t baud)
    : devicePath(devicePath_)
{
    fd = ::open(devicePath.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        throw std::runtime_error("Failed to open device: " + devicePath);
    }

    struct termios tio{};
    if (tcgetattr(fd, &tio) != 0) {
        ::close(fd);
        throw std::runtime_error("Failed to get termios for device: " + devicePath);
    }

    cfmakeraw(&tio);
    tio.c_cflag |= (CLOCAL | CREAD);
    tio.c_cflag &= ~PARENB;
    tio.c_cflag &= ~CSTOPB;
    tio.c_cflag &= ~CSIZE;
    tio.c_cflag |= CS8;

    int baudConst = baudRateToConstant(baud);
    if (baudConst <= 0) {
        ::close(fd);
        throw std::runtime_error("Unsupported baud rate for device: " + devicePath);
    }
    cfsetispeed(&tio, baudConst);
    cfsetospeed(&tio, baudConst);

    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tio) != 0) {
        ::close(fd);
        throw std::runtime_error("Failed to set termios for device: " + devicePath);
    }

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
    }
}

Channel::~Channel()
{
    if (fd >= 0) {
        ::close(fd);
    }
    fd = -1;
}

std::size_t Channel::write(const ByteVec& in)
{
    if (fd < 0) {
        throw std::runtime_error("UART stream not open: " + devicePath);
    }
    ssize_t written = ::write(fd, in.data(), in.size());
    if (written < 0) {
        throw std::runtime_error("UART write error: " + std::string(strerror(errno)));
    }
    return static_cast<std::size_t>(written);
}

int Channel::getFd() const
{
    return fd;
}

void Channel::flush()
{
    if (fd >= 0)
    {
        ::tcflush(fd, TCIOFLUSH);
    }
}

void Channel::sendBreak()
{
#ifdef TIOCSBRK
    if (::ioctl(fd, TIOCSBRK, 0) == 0) {
        usleep(250000);
        ::ioctl(fd, TIOCCBRK, 0);
    }
#endif
}

int Channel::baudRateToConstant(uint32_t baud)
{
    switch (baud) {
        case 2400: return B2400;
        case 9600: return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
        case 57600: return B57600;
        case 115200: return B115200;
        case 230400: return B230400;
        case 460800:
#ifdef B460800
            return B460800;
#else
            return -1;
#endif
        default: return -1;
    }
}
