#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>

#include "foundation/ADebug.h"
#define LOG_TAG "IPCAM-stage-util"

void MakeSocketBlocking(int s, bool blocking) {
    // Make socket non-blocking.
    int flags = fcntl(s, F_GETFL, 0);
	if(flags < 0)
		LOGE(LOG_TAG,"%s",strerror(errno));
	
    if (blocking) {
        flags &= ~O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }
	flags = fcntl(s, F_SETFL, flags);
	if(flags < 0)
		LOGE(LOG_TAG,"%s",strerror(errno));	
}

