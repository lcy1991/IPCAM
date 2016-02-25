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

#ifndef A_LOOPER_H_

#define A_LOOPER_H_
#include "foundation/AMessage.h"
#include "foundation/AHandler.h"
#include "foundation/StrongPointer.h"
#include "foundation/ABase.h"
#include "foundation/AString.h"
#include "foundation/Errors.h"
#include "foundation/Mutex.h"
#include <list>
#include <pthread.h>


using namespace std;

struct AHandler;//Ç°ÏòÉùÃ÷
struct AMessage;

typedef int32_t event_id;
typedef int32_t handler_id;


struct ALooper{
public:	

    ALooper();

    // Takes effect in a subsequent call to start().
    void setName(const char *name);

    handler_id registerHandler(AHandler* handler);
    void unregisterHandler(handler_id handlerID);

    status_t start();

    status_t stop();

    static int64_t GetNowUs();


    virtual ~ALooper();

private:
    friend struct ALooperRoster;

    struct Event {
        int64_t mWhenUs;
        sp<AMessage> mMessage;
    };

    Mutex mLock;
    pthread_t mTID;
    pthread_cond_t mQueueChangedCondition;

    AString mName;

    list<Event> mEventQueue;

	bool mIsRunning;

    void post(const sp<AMessage> &msg, int64_t delayUs);
	bool threadloop();
    static void* loop(void* arg);

DISALLOW_EVIL_CONSTRUCTORS(ALooper);

};



#endif  // A_LOOPER_H_

