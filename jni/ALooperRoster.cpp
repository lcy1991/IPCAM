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
#define LOG_TAG "IPCAM-ALooperRoster"
#include "foundation/ALooperRoster.h"
#include "foundation/ADebug.h"
#include "foundation/AHandler.h"
#include "foundation/AMessage.h"
using namespace std;


ALooperRoster::ALooperRoster()
    : mNextHandlerID(1) {
}

    handler_id ALooperRoster::registerHandler(
         ALooper* looper,  AHandler* handler) {
    Mutex::Autolock autoLock(mLock);

    if (handler->id() != 0) {
        CHECK(!"A handler must only be registered once.");
        return INVALID_OPERATION;
    }

    HandlerInfo info;
    info.mLooper = looper;
    info.mHandler = handler;
    handler_id handlerID = mNextHandlerID++;
	mHandlers.insert(make_pair(handlerID, info));

    handler->setID(handlerID);

    return handlerID;
}

void ALooperRoster::unregisterHandler(handler_id handlerID) {
    Mutex::Autolock autoLock(mLock);

    std::map<handler_id, HandlerInfo>::iterator iter;
	iter = mHandlers.find(handlerID);
	if(iter != mHandlers.end()){
		AHandler* handler = iter->second.mHandler;
		if (handler != NULL)
    		handler->setID(0);
		mHandlers.erase(iter);
	}
}

void ALooperRoster::postMessage(
        const sp<AMessage> &msg, int64_t delayUs) {
	ALooper* looper;
    map<handler_id, HandlerInfo>::iterator iter;
{
    Mutex::Autolock autoLock(mLock);	
	iter = mHandlers.find(msg->target());
	if(iter != mHandlers.end()){
		looper = iter->second.mLooper;
	    if (looper == NULL) {
	        LOGW(LOG_TAG,"failed to post message. "
	             "Target handler %d still registered, but object gone.",
	             msg->target());
	        mHandlers.erase(iter);
	        return;
		}
	}
	else{
		LOGW(LOG_TAG,"failed to post message. Target handler not registered.");
    	return;
	}
}	
	looper->post(msg, delayUs);	
}

void ALooperRoster::deliverMessage(const sp<AMessage> &msg) {
    AHandler* handler;
    map<handler_id, HandlerInfo>::iterator iter;
{
	Mutex::Autolock autoLock(mLock);
	iter = mHandlers.find(msg->target());
	if(iter != mHandlers.end()){
		handler = iter->second.mHandler;
	    if (handler == NULL) {
            LOGW(LOG_TAG,"failed to deliver message. "
                 "Target handler %d registered, but object gone.",
	             msg->target());
	        mHandlers.erase(iter);
	        return;
		}
	}
	else{
		LOGW(LOG_TAG,"failed to deliver message. Target handler not registered.");
    	return;
	}
}	
	handler->onMessageReceived(msg);
}

ALooper* ALooperRoster::findLooper(handler_id handlerID) {
    Mutex::Autolock autoLock(mLock);
	ALooper* looper;
	map<handler_id,HandlerInfo>::iterator iter;
	iter = mHandlers.find(handlerID);
	if(iter!=mHandlers.end()){
		looper = iter->second.mLooper;
	    if (looper == NULL) {
	        mHandlers.erase(iter);
	        return NULL;
		}
		return looper;
	}
	else
        return NULL;

}









