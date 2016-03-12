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

#include "foundation/ABuffer.h"
#include "foundation/ADebug.h"
#include "foundation/ALooper.h"
#include "foundation/AMessage.h"
#define LOG_TAG "IPCAM-ABuffer"

ABuffer::ABuffer(size_t capacity)
    : mData(malloc(capacity)),
      mCapacity(capacity),
      mRangeOffset(0),
      mRangeLength(capacity),
      mInt32Data(0),
      mOwnsData(true) {
}

ABuffer::ABuffer(void *data, size_t capacity)
    : mData(data),
      mCapacity(capacity),
      mRangeOffset(0),
      mRangeLength(capacity),
      mInt32Data(0),
      mOwnsData(false) {
}

ABuffer::~ABuffer() {
    if (mOwnsData) {
        if (mData != NULL) {
            free(mData);
			//LOGI(LOG_TAG,"~ABuffer data is free");
            mData = NULL;
        }
    }

    if (mFarewell != NULL) {
        mFarewell->post();
    }
}

void ABuffer::setRange(size_t offset, size_t size) {
	//LOGI(LOG_TAG,"offset %d,mCapacity %d,offset + size %d",offset,mCapacity,offset + size);
    CHECK_LE(offset, mCapacity);
    CHECK_LE(offset + size, mCapacity);

    mRangeOffset = offset;
    mRangeLength = size;
}

void ABuffer::setFarewellMessage(AMessage* msg) {
    mFarewell = msg;
}

AMessage* ABuffer::meta() {
    if (mMeta == NULL) {
        mMeta = new AMessage;
    }
    return mMeta;
}


