#ifndef _HANDLER_1_H
#define _HANDLER_1_H

#include "foundation/AHandler.h"
struct handler_1 : AHandler
{
	handler_1();
	~handler_1();
	void start(uint32_t mWhat,uint32_t startNum);
	void setTarget(handler_id Target);
protected:
    virtual void onMessageReceived(const sp<AMessage> &msg);
	uint32_t mhello_count;
	handler_id mTarget;
};
#endif
