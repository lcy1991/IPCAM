#ifndef A_RTP_SOURCE_H_

#define A_RTP_SOURCE_H_

#include <queue>
#include "foundation/ABuffer.h"
#include "foundation/Mutex.h"
#include "foundation/ADebug.h"

using namespace std;
struct MyQueue
{
	MyQueue(uint32_t MaxBufNum,uint32_t bufSize);
	~MyQueue();
	int push(const sp<ABuffer> &buf);
	int pop(sp<ABuffer> &buf);
	int getQSize();
	int getEmptyBufSize();

private:
	queue<sp<ABuffer> > mBufQue;
	uint32_t mEmptyBufSize;
	uint32_t mMaxBufSize;
};




struct ARTPSource
{
	ARTPSource(uint32_t bufNum, uint32_t bufSize);
	~ARTPSource();
	int inputQPop(sp<ABuffer> &buf);
	int inputQPush(const sp<ABuffer> &buf);
	int outputQPop(sp<ABuffer> &buf);
	int outputQPush(const sp<ABuffer> &buf);
	uint8_t mFramerate;
private:
	uint32_t mBufSize;
	uint32_t mBufNum;
	MyQueue* mInputQueue;
	MyQueue* mOutputQueue;
	MyQueue* pInInputQ;
	MyQueue* pOutOutputQ;	
	Mutex mOutputQLock;
	Mutex mInputQLock;
	
};


#endif
