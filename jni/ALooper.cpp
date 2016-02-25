/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "ALooper"

#include <sys/time.h>

#include "foundation/ALooper.h"

#include "foundation/AHandler.h"
#include "foundation/ALooperRoster.h"
#include "foundation/AMessage.h"
#include "foundation/ADebug.h"


ALooperRoster gLooperRoster;

// static
int64_t ALooper::GetNowUs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (int64_t)tv.tv_sec * 1000000ll + tv.tv_usec;
}

ALooper::ALooper()
    : mIsRunning(false) {
}

ALooper::~ALooper() {
    stop();
}

void ALooper::setName(const char *name) {
    mName = name;
}

handler_id ALooper::registerHandler(AHandler* handler) {
    return gLooperRoster.registerHandler(this, handler);
}

void ALooper::unregisterHandler(handler_id handlerID) {
    gLooperRoster.unregisterHandler(handlerID);
}

status_t ALooper::start(){
	Mutex::Autolock autoLock(mLock);
	int32_t err;
	mIsRunning = true;
	err = pthread_cond_init(&mQueueChangedCondition,NULL);
    if ( 0 != err ) 
    { 
        LOGE(LOG_TAG,"can't init Condition:%s\n", strerror(err)); 
		return err;
    }	
	err = pthread_create(&mTID, NULL,loop,(void*)this); 
    if ( 0 != err ) 
    { 
        LOGE(LOG_TAG,"can't create thread:%s\n", strerror(err));
		return err;
    } 
	return OK;
}

status_t ALooper::stop(){
	mIsRunning = false;
	return OK;
}


void ALooper::post(const sp<AMessage> &msg, int64_t delayUs) {
    Mutex::Autolock autoLock(mLock);

    int64_t whenUs;
    if (delayUs > 0) {
        whenUs = GetNowUs() + delayUs;
    } else {
        whenUs = GetNowUs();
    }

    list<Event>::iterator it = mEventQueue.begin();
    while (it != mEventQueue.end() && (*it).mWhenUs <= whenUs) {
        ++it;
    }

    Event event;
    event.mWhenUs = whenUs;
    event.mMessage = msg;

    if (it == mEventQueue.begin()) {
//        mQueueChangedCondition.signal();
	   pthread_cond_signal(&mQueueChangedCondition);
    }
    mEventQueue.insert(it, event);
}


bool ALooper::threadloop(){
    Event event;
    {
		struct timespec timer;
        Mutex::Autolock autoLock(mLock);
        if (mIsRunning==false) {
            return false;
        }
        if (mEventQueue.empty()) {
            //mQueueChangedCondition.wait(mLock);
            pthread_cond_wait(&mQueueChangedCondition,&mLock.mMutex);
            return true;
        }
        int64_t whenUs = (*mEventQueue.begin()).mWhenUs;
        int64_t nowUs = GetNowUs();

        if (whenUs > nowUs) {
            int64_t delayUs = whenUs - nowUs;
    		timer.tv_sec = time(NULL);
   			timer.tv_nsec = delayUs * 1000ll;			
			pthread_cond_timedwait(&mQueueChangedCondition,&mLock.mMutex,&timer);
//            mQueueChangedCondition.waitRelative(mLock, delayUs * 1000ll);
            return true;
        }

        event = *mEventQueue.begin();
        mEventQueue.erase(mEventQueue.begin());
    }

    gLooperRoster.deliverMessage(event.mMessage);

    return true;	
}

void* ALooper::loop(void* arg) {
	ALooper* ptr = (ALooper*)arg;
	while(ptr->threadloop());
	pthread_cond_destroy(&ptr->mQueueChangedCondition);
}

