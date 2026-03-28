#include "st_lib.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Reads a 32-bit integer from userspace and prints it");

void set_core_name(char * buf, int value)
{
    buf[0] = 'c';
    buf[1] = 'o';
    buf[2] = 'r';
    buf[3] = 'e';
    int offset = 4;
    if(value > 99)
    {
        buf[offset] = value / 100 + '0';
        value = value % 100;
        ++offset;
    }
    if(value > 9)
    {
        buf[offset] = value / 10 + '0';
        value = value % 10;
        ++offset;
    }

    buf[offset] = value + '0';
    ++offset;

    buf[offset] = '\0';

}