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

#ifndef A_LOOPER_ROSTER_H_

#define A_LOOPER_ROSTER_H_


#include "foundation/ALooper.h"
#include "foundation/Mutex.h"
#include "foundation/StrongPointer.h"
#include <map>
using namespace std;


struct ALooperRoster {
    ALooperRoster();

    handler_id registerHandler(
            ALooper* looper,AHandler* handler);

    void unregisterHandler(handler_id handlerID);

    void postMessage(const sp<AMessage> &msg, int64_t delayUs = 0);
    void deliverMessage(const sp<AMessage> &msg);

    ALooper* findLooper(handler_id handlerID);

private:
    struct HandlerInfo {
        ALooper* mLooper;
        AHandler* mHandler;
    };

    Mutex mLock;
    map<handler_id, HandlerInfo> mHandlers;
    handler_id mNextHandlerID;

    DISALLOW_EVIL_CONSTRUCTORS(ALooperRoster);
};



#endif  // A_LOOPER_ROSTER_H_

