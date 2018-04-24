#ifndef _TERMIOS_UTIL_HPP_
#define _TERMIOS_UTIL_HPP_

#include <unistd.h>
#include <errno.h>
#include <termios.h>

#ifdef __cplusplus

namespace irods
{
    // class termiosUtil - at this time, this class encapsulates the work
    // which has to be done in order to turn echo on or off for a specifc
    // file descriptor.  The call can be expanded to encapsulate other
    // functionality of termios(3).

    class termiosUtil {
        public:
            // Class has to be instantiated with a file descriptor.
            // The default constructor is invalid.
            explicit termiosUtil(int fd) :
                valid_(false),
                errno_copy_(0),
                fd_(fd),
                save_c_lflag_(0)
            {
                memset(&tty_, 0, sizeof(tty_));
                if (isatty(fd_))
                {
                    if (tcgetattr( fd_, &tty_ ) == 0)
                    {
                        save_c_lflag_ = tty_.c_lflag;
                        valid_ = true;
                    }
                    else
                    {
                        errno_copy_ = errno;
                        valid_ = false;
                    }
                }
                else
                {
                    errno_copy_ = errno;
                    valid_ = false;
                }
            };

        public:
            virtual ~termiosUtil(void) = default;
            int getError(void) const     { return errno_copy_; };
            bool getValid(void) const     { return valid_; };

            bool echoOff(void)
            {
                if (!valid_)
                {
                    return false;
                }
                tty_.c_lflag &= ~ECHO;
                if (tcsetattr( fd_, TCSANOW, &tty_ ) == 0)
                {
                    return true;
                }
                errno_copy_ = errno;
                valid_ = false;
                return false;
            }

            bool echoOn(void)
            {
                if (!valid_)
                {
                    return false;
                }
                tty_.c_lflag = save_c_lflag_;
                if (tcsetattr( fd_, TCSANOW, &tty_ ) == 0)
                {
                    return true;
                }
                errno_copy_ = errno;
                valid_ = false;
                return false;
            }

       private:
            bool valid_;            // fd_ >= 0, etc
            int errno_copy_;        // if fd_ is not a tty, or other termios error, this has the error
            int fd_;                // File descriptor for this tty_ structure
            struct termios tty_;
            tcflag_t save_c_lflag_;
    };
}; // namespace irods

#endif // __cplusplus
#endif // _TERMIOS_UTIL_HPP_
