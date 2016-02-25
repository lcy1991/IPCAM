#include "handler_1.h"
#include "foundation/ADebug.h"
#include "foundation/StrongPointer.h"
#define LOG_TAG __FILE__


handler_1::handler_1()
{
	
}

handler_1::~handler_1()
{
	
}
void handler_1::setTarget(handler_id Target)
{
	mTarget = Target;
}

void handler_1::start(uint32_t mWhat,uint32_t startNum)
{
	
	sp<AMessage> msg = new AMessage(mWhat, mTarget);
	msg->setInt32("hello",startNum);
	msg->post();
}

void handler_1::onMessageReceived(const sp<AMessage> &msg)
{
	int32_t value;
	int32_t what;
	what = msg->what();
	if(what == 0)
		{
			if(msg->findInt32("hello",&value))
				{
					if(value == 100)return;
					LOGI(LOG_TAG,"received the num %d message hello %d\n",what,value);
					msg->setInt32("hello",value+1);
					msg->setTarget(mTarget);
					msg->post(10);
				}
		}
}



