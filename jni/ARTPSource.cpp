#include "rtsp/ARTPSource.h"
#define LOG_TAG "ARTPSource"

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
			//LOGI(LOG_TAG,"~MyQueue");
		}
		
}


int MyQueue::push(const sp<ABuffer> &buf)
{
	if(mBufQue.size() == mMaxBufSize)
		{
			LOG_IF(IF,LOG_TAG,"MyQueue is full!");
			return -1;
		}
	if(buf->size()==0)
			mEmptyBufSize++;
	mBufQue.push(buf);
	LOG_IF(IF,LOG_TAG,"mEmptyBufSize %d",mEmptyBufSize);
	return 0;
}
int	MyQueue::pop(sp<ABuffer> &buf)
{
	if(mBufQue.size()==0)
		{
			LOG_IF(IF,LOG_TAG,"MyQueue is empty!");
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
	mInputQLock.lock();
	LOG_IF(IF,LOG_TAG,"mInputQ is Lock");
	if(pInInputQ->getEmptyBufSize()==0)//input queue is full, we need empty queue
		{
			mInputQLock.unlock();
			return -1;
#if 0			
			LOGI(LOG_TAG,"inputQPop input queue is full,waiting for exchange");
			mInputQLock.unlock();
			LOGI(LOG_TAG,"mInputQ is unLock");
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
			LOGI(LOG_TAG,"mOutputQ is unLock--------------------------");
#endif			
		}
	else 
		{
			LOG_IF(IF,LOG_TAG,"pop from input queue4 %p",pInInputQ);
			if(pInInputQ==NULL)LOG_IF(IF,LOG_TAG,"pInInputQ is null");
			
			if(pInInputQ->pop(buf)!=0)
				{
					LOG_IF(IF,LOG_TAG,"pop error");
					return -1;
				}
				
		}
	
}
int ARTPSource::inputQPush(const sp<ABuffer> &buf)
{
	LOG_IF(IF,LOG_TAG,"push to input queue %d",pInInputQ);
	pInInputQ->push(buf);
	mInputQLock.unlock();
	LOG_IF(IF,LOG_TAG,"mInputQ is unLock");
	return 0;
}
int ARTPSource::outputQPop(sp<ABuffer> &buf)
{

	mOutputQLock.lock();

	if(pOutOutputQ->getEmptyBufSize() == pOutOutputQ->getQSize())//queue is empty
		{
			
			LOG_IF(IF,LOG_TAG,"output queue is empty ,waiting for exchange");
			mOutputQLock.unlock();
			LOG_IF(IF,LOG_TAG,"mOutputQ is unlock");
			mInputQLock.lock();
			LOG_IF(IF,LOG_TAG,"mInputQ is lock");
			if(pInInputQ->getEmptyBufSize()!=0)//intput queue is not empty, we change in and out
				{
					mInputQLock.unlock();
					LOG_IF(IF,LOG_TAG,"mInputQ is unlock");
					mOutputQLock.unlock();
					LOG_IF(IF,LOG_TAG,"mOutputQ is unlock");
					LOG_IF(IF,LOG_TAG,"input Q is also empty");
					return -1;
				}
			pOutOutputQ = (pOutOutputQ == mOutputQueue)?mInputQueue:mOutputQueue;
			pInInputQ = (pInInputQ == mInputQueue)?mOutputQueue:mInputQueue;
			pOutOutputQ->pop(buf);
			if(buf->size()==0)//buf is empty
				{
					pOutOutputQ->push(buf);
					return -1;
				}			
			mInputQLock.unlock();
			LOG_IF(IF,LOG_TAG,"mInputQ is unlock +++++++++++++++++++++++++++");
		}
	else
		{
			LOG_IF(IF,LOG_TAG,"pop from output queue %d",pOutOutputQ);
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
	LOG_IF(IF,LOG_TAG,"push to output queue");
	pOutOutputQ->push(buf);
	//if(pOutOutputQ->getEmptyBufSize() == pOutOutputQ->getQSize())
	//	{
	mOutputQLock.unlock();
			LOG_IF(IF,LOG_TAG,"mOutputQ is unlock");
	//	}
	return 0;
}


ARTPSource::ARTPSource(uint32_t bufNum, uint32_t bufSize)
	: mBufSize(bufSize),
	  mBufNum(bufNum),
	  mFramerate(15)
{

	mInputQueue = new MyQueue(bufNum,bufSize);
	mOutputQueue = new MyQueue(bufNum,bufSize);	
	pInInputQ = mInputQueue;
	pOutOutputQ = mOutputQueue;

}

ARTPSource::~ARTPSource()
{

}




