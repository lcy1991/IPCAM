#include "rtsp/ARTPSource.h"
#define LOG_TAG "IPCAM-ARTPSource"

#define IF 0
MyQueue::MyQueue(uint32_t MaxBufNum,uint32_t bufSize)
{
	mEmptyBufSize = 0;
	mMaxBufSize = MaxBufNum;
	int i;
	for(i = 0; i < mMaxBufSize;i++)
	{
		sp<ABuffer> buf1 = new ABuffer(bufSize);
		buf1->setRange(0,0);
		push(buf1);
	}
}
MyQueue::~MyQueue()
{
	while(mBufQue.size())
		{
			mBufQue.pop();
			//LOGI("~MyQueue");
		}
		
}


int MyQueue::push(const sp<ABuffer> &buf)
{
	if(mBufQue.size() == mMaxBufSize)
		{
			LOG_IF(IF,"MyQueue is full!");
			return -1;
		}
	if(buf->size()==0)
			mEmptyBufSize++;
	mBufQue.push(buf);
	LOG_IF(IF,"mEmptyBufSize %d",mEmptyBufSize);
	return 0;
}
int	MyQueue::pop(sp<ABuffer> &buf)
{
	if(mBufQue.size()==0)
		{
			LOG_IF(IF,"MyQueue is empty!");
			return -1;
		}

	buf = mBufQue.front();
	if(buf->size()==0)
		mEmptyBufSize--;	
	mBufQue.pop();
	return 0;

}
int MyQueue::getQSize()
{
	return mMaxBufSize;//mBufQue.size();
}
int MyQueue::getEmptyBufSize()
{
	return mEmptyBufSize;
}


int ARTPSource::inputQPop(sp<ABuffer> &buf)
{
	LOG_IF(IF,"try to get mInputQLock");
	mInputQLock.lock();
	LOG_IF(IF,"mInputQ is Lock");
	if(pInInputQ->getEmptyBufSize()==0)//input queue is full, we need empty queue
		{
			mInputQLock.unlock();
			return -1;
#if 0			
			LOGI("inputQPop input queue is full,waiting for exchange");
			mInputQLock.unlock();
			LOGI("mInputQ is unLock");
			mOutputQLock.lock();//get the output queue
			if(pOutOutputQ->getEmptyBufSize()!=0)//if output queue is not empty, don't change
				{
					mOutputQLock.unlock();
					return -1;
				}
			pOutOutputQ = (pOutOutputQ == mOutputQueue)?mInputQueue:mOutputQueue;
			pInInputQ = (pInInputQ == mInputQueue)?mOutputQueue:mInputQueue;

			pInInputQ->pop(buf);
			mOutputQLock.unlock();
			LOGI("mOutputQ is unLock--------------------------");
#endif			
		}
	else 
		{
			LOG_IF(IF,"pop from input queue %p",pInInputQ);
			if(pInInputQ==NULL)LOG_IF(IF,"pInInputQ is null");
			
			if(pInInputQ->pop(buf)!=0)
				{
					LOG_IF(IF,"pop error");
					return -1;
				}
				
		}
	
}
int ARTPSource::inputQPush(const sp<ABuffer> &buf)
{
	LOG_IF(IF,"push to input queue %p",pInInputQ);
	pInInputQ->push(buf);
	mInputQLock.unlock();
	LOG_IF(IF,"mInputQ is unLock");
	return 0;
}
int ARTPSource::outputQPop(sp<ABuffer> &buf)
{
	MyQueue* tmpPtr;
	//mOutputQLock.lock();//第一次进入需要锁住剩下的在交换时解锁和上锁

	if(pOutOutputQ->getEmptyBufSize() == pOutOutputQ->getQSize())//queue is empty
		{

			LOG_IF(IF,"output queue is empty ,waiting for exchange");
			//mOutputQLock.unlock();
			//LOG_IF(IF,"mOutputQ is unlock");
			mInputQLock.lock();
			LOG_IF(IF,"mInputQ is lock");
			if(pInInputQ->getEmptyBufSize()== pInInputQ->getQSize())//intput queue is not empty, we change in and out
				{
					mInputQLock.unlock();
					//LOG_IF(IF,"mInputQ is unlock");
					//mOutputQLock.unlock();
					//LOG_IF(IF,"mOutputQ is unlock");
					//LOG_IF(IF,"input Q is also empty");
					return -1;
				}
			tmpPtr = pOutOutputQ;
			pOutOutputQ = pInInputQ;
			pInInputQ = tmpPtr;
			//pOutOutputQ = (pOutOutputQ == mOutputQueue)?mInputQueue:mOutputQueue;
			//pInInputQ = (pInInputQ == mInputQueue)?mOutputQueue:mInputQueue;
			LOG_IF(IF,"Exchange +++++++++++++++++++++++++++IN[%p] OUT[%p]",pInInputQ,pOutOutputQ);
			pOutOutputQ->pop(buf);
			LOG_IF(IF,"pop from output queue %p",pOutOutputQ);

			if(buf->size()==0)//buf is empty
				{
					pOutOutputQ->push(buf);
					mInputQLock.unlock();
					return -1;
				}			
			mInputQLock.unlock();
		}
	else
		{
			LOG_IF(IF,"pop from output queue %p",pOutOutputQ);
			pOutOutputQ->pop(buf);
			if(buf->size()==0)//ABuffer is empty
				{
					pOutOutputQ->push(buf);
					return -1;
				}
		}
	return 0;
}
int ARTPSource::outputQPush(const sp<ABuffer> &buf)
{
	LOG_IF(IF,"push to output queue");
	pOutOutputQ->push(buf);
	//if(pOutOutputQ->getEmptyBufSize() == pOutOutputQ->getQSize())
	//	{
	//mOutputQLock.unlock();
	//		LOG_IF(IF,"mOutputQ is unlock");
	//	}
	return 0;
}


ARTPSource::ARTPSource(uint32_t bufNum, uint32_t bufSize)
	: mBufSize(bufSize),
	  mBufNum(bufNum),
	  mFramerate(29)
{

	mInputQueue = new MyQueue(bufNum,bufSize);
	mOutputQueue = new MyQueue(bufNum,bufSize);	
	pInInputQ = mInputQueue;
	pOutOutputQ = mOutputQueue;

}

ARTPSource::~ARTPSource()
{

}




