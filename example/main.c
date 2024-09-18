
#include <stdio.h>
#include <unistd.h>

#include "sip.h"
#include "log.h"

int main(int argc, char *argv[])
{
    log_init("./gb28181.log", 512*1024, 3, 3);
    SipInit();

    while (1){
        sleep(1);
    }

    SipUnInit();

    return 0;
}
