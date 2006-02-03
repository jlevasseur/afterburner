#ifndef __AFTERBURN_WEDGE__INCLUDE__LINUX__HIOSTREAM_H__
#define __AFTERBURN_WEDGE__INCLUDE__LINUX__HIOSTREAM_H__

#include <hiostream.h>
#include <unistd.h>

class hiostream_linux_t : public hiostream_driver_t
{
public:
    virtual void print_char(char ch)
    { write(1, &ch, 1); }

    virtual char get_blocking_char()
    {  return 'a'; }
};


#endif	/* __AFTERBURN_WEDGE__INCLUDE__LINUX__HIOSTREAM_H__ */
