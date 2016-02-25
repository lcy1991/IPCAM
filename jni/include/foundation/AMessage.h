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

#ifndef A_MESSAGE_H_

#define A_MESSAGE_H_

#include "foundation/ABase.h"
#include "foundation/ALooper.h"
#include "foundation/LightRefBase.h"
#include "foundation/StrongPointer.h"
#include "foundation/ABuffer.h"

#include <stdint.h>
#include <sys/types.h>
typedef int32_t handler_id;

struct AString;

struct AMessage :public LightRefBase<AMessage>{             
    AMessage (uint32_t what = 0, handler_id target = 0);

    void setWhat(uint32_t what);
    uint32_t what() const;

    void setTarget(handler_id target);
    handler_id target() const;

    void setInt32(const char *name, int32_t value);
    void setInt64(const char *name, int64_t value);
    void setSize(const char *name, size_t value);
    void setFloat(const char *name, float value);
    void setDouble(const char *name, double value);
    void setPointer(const char *name, void *value);
    void setString(const char *name, const char *s, ssize_t len = -1);
//    void setObject(const char *name, const sp<RefBase> &obj);
    void setMessage(const char *name, const sp<AMessage> &obj);
    void setBuffer(const char *name, const sp<ABuffer> &obj);
    bool findInt32(const char *name, int32_t *value) const;
    bool findInt64(const char *name, int64_t *value) const;
    bool findSize(const char *name, size_t *value) const;
    bool findFloat(const char *name, float *value) const;
    bool findDouble(const char *name, double *value) const;
    bool findPointer(const char *name, void **value) const;
    bool findString(const char *name, AString *value) const;
//    bool findObject(const char *name, sp<RefBase> *obj) const;
    bool findMessage(const char *name, sp<AMessage> *obj) const;
	bool findBuffer(const char *name, sp<ABuffer> *obj) const;

    void post(int64_t delayUs = 0);

    AMessage* dup() const;

    AString debugString(int32_t indent = 0) const;


//protected:
    virtual ~AMessage();

private:
    enum Type {
        kTypeInt32,
        kTypeInt64,
        kTypeSize,
        kTypeFloat,
        kTypeDouble,
        kTypePointer,
        kTypeString,
        kTypeObject,
        kTypeMessage,
        kTypeBuffer,
    };

    uint32_t mWhat;
    handler_id mTarget;

    struct Item {
        union {
            int32_t int32Value;
            int64_t int64Value;
            size_t sizeValue;
            float floatValue;
            double doubleValue;
            void *ptrValue;
            AString *stringValue;
			AMessage *msgValue;
			ABuffer *bufValue;
        } u;
        const char *mName;
        Type mType;
    };

    enum {
        kMaxNumItems = 16
    };
    Item mItems[kMaxNumItems];
    size_t mNumItems;

    void clear();
    Item *allocateItem(const char *name);
    void freeItem(Item *item);
    const Item *findItem(const char *name, Type type) const;
	DISALLOW_EVIL_CONSTRUCTORS(AMessage);

};


#endif  // A_MESSAGE_H_
