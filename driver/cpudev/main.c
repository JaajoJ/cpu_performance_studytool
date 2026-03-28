#include <linux/module.h>
#include <linux/kernel.h>
#include "../src/st_lib.h"

static int __init hello_init(void)
{
    st_setup();
    return 0;
}

static void __exit hello_exit(void)
{
    st_destroy();
}

module_init(hello_init);
module_exit(hello_exit);