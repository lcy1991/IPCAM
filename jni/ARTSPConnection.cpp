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
#define LOG_TAG "IPCAM-ARTSPConnection"
#include "rtsp/ARTSPConnection.h"
#include "rtsp/base64.h"

#include "foundation/ABuffer.h"
#include "foundation/ADebug.h"
#include "foundation/AMessage.h"
#include "stage_utils.h"

#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
//#include <openssl/md5.h>
#include <sys/socket.h>
#include <errno.h>

// static
const int64_t ARTSPConnection::kSelectTimeoutUs = 1000ll;

ARTSPConnection::ARTSPConnection()
    : mState(CONNECTED),
      mAuthType(NONE),
      mSocket(-1),
      mReceiveRequestEventPending(false) {
}

ARTSPConnection::~ARTSPConnection() {
    if (mSocket >= 0) {
        LOGE(LOG_TAG,"Connection is still open, closing the socket.");
        close(mSocket);
        mSocket = -1;
    }
}

void ARTSPConnection::disconnect(const sp<AMessage> &reply) {
    sp<AMessage> msg = new AMessage(kWhatDisconnect, id());
    msg->setMessage("reply", reply);
    msg->post();
}

//liuchangyou
void ARTSPConnection::sendResponse(
        const char *response) {
    sp<AMessage> msg = new AMessage(kWhatSendResponse, id());
    msg->setString("response", response);
    msg->post();
}


void ARTSPConnection::observeBinaryData(const sp<AMessage> &reply) {
    sp<AMessage> msg = new AMessage(kWhatObserveBinaryData, id());
    msg->setMessage("reply", reply);
    msg->post();
}

void ARTSPConnection::onMessageReceived(const sp<AMessage> &msg) {
    switch (msg->what()) {

        case kWhatDisconnect:
            onDisconnect(msg);
            break;

        case kWhatCompleteConnection:
            onCompleteConnection(msg);
            break;
			
        case kWhatReceiveRequest:
            onReceiveRequest();
            break;

        case kWhatSendResponse:
            onSendResponse(msg);
            break;			

        case kWhatObserveBinaryData:
        {
            CHECK(msg->findMessage("reply", &mObserveBinaryMessage));
            break;
        }

        default:
            TRESPASS();
            break;
    }
}

#if 0
static void MakeSocketBlocking(int s, bool blocking) {
    // Make socket non-blocking.
    int flags = fcntl(s, F_GETFL, 0);
    CHECK_NE(flags, -1);

    if (blocking) {
        flags &= ~O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }

    CHECK_NE(fcntl(s, F_SETFL, flags), -1);
}
#endif
void ARTSPConnection::StartListen(int socket,handler_id handlerID,uint32_t mtempSessionID)
{
	mSocket = socket;
	mhandlerID = handlerID;
	mSessionID = mtempSessionID;
	LOGI(LOG_TAG,"StartListen mSessionID[%u]\n",mSessionID);
    sp<AMessage> reply = new AMessage(kWhatReceiveRequest, id());
	reply->post();
}

void ARTSPConnection::onDisconnect(const sp<AMessage> &msg) {
    if (mState == CONNECTED || mState == CONNECTING) {
        close(mSocket);
        mSocket = -1;

//        flushPendingRequests();
    }

    sp<AMessage> reply;
    CHECK(msg->findMessage("reply", &reply));

    reply->setInt32("result", OK);
    mState = DISCONNECTED;

    mUser.clear();
    mPass.clear();
    mAuthType = NONE;
    mNonce.clear();

    reply->post();
}

void ARTSPConnection::onCompleteConnection(const sp<AMessage> &msg) {
    sp<AMessage> reply;
    CHECK(msg->findMessage("reply", &reply));

    int32_t connectionID;
    CHECK(msg->findInt32("connection-id", &connectionID));

    if (mState != CONNECTING) {
        // While we were attempting to connect, the attempt was
        // cancelled.
        reply->setInt32("result", -ECONNABORTED);
        reply->post();
        return;
    }

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = kSelectTimeoutUs;

    fd_set ws;
    FD_ZERO(&ws);
    FD_SET(mSocket, &ws);

    int res = select(mSocket + 1, NULL, &ws, NULL, &tv);
    CHECK_GE(res, 0);

    if (res == 0) {
        // Timed out. Not yet connected.

        msg->post();
        return;
    }

    int err;
    socklen_t optionLen = sizeof(err);
    CHECK_EQ(getsockopt(mSocket, SOL_SOCKET, SO_ERROR, &err, &optionLen), 0);
    CHECK_EQ(optionLen, (socklen_t)sizeof(err));

    if (err != 0) {
        LOGE(LOG_TAG,"err = %d (%s)", err, strerror(err));

        reply->setInt32("result", -err);

        mState = DISCONNECTED;
        close(mSocket);
        mSocket = -1;
    } else {
        reply->setInt32("result", OK);
        mState = CONNECTED;

    }

    reply->post();
}

//liluchangyou
void ARTSPConnection::onSendResponse(const sp<AMessage> &msg) {

    AString response;
	sp<AMessage> notify;
    CHECK(msg->findString("response", &response));

    // Find the boundary between headers and the body.
    ssize_t i = response.find("\r\n\r\n");
    CHECK_GE(i, 0);

    LOGI(LOG_TAG,"response:\n%s", response.c_str());

    size_t numBytesSent = 0;
    while (numBytesSent < response.size()) {
        ssize_t n =
            send(mSocket, response.c_str() + numBytesSent,
                 response.size() - numBytesSent, 0);

        if (n == 0) {
            // Client closed the connection.
            LOGE(LOG_TAG,"Client unexpectedly closed the connection.");
			mState = DISCONNECTED;
			notify = new AMessage(kwhatCloseSession,mhandlerID);
			notify->setInt32("result",-1);
			notify->setInt32("sessionID",mSessionID);
			notify->post();		
            return;
        } else if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
			mState = DISCONNECTED;
            LOGE(LOG_TAG,"Error sending rtsp response.");
			notify = new AMessage(kwhatCloseSession,mhandlerID);
			notify->setInt32("result",errno );
			notify->setInt32("sessionID",mSessionID);
			notify->post();		
            return;
        }

        numBytesSent += (size_t)n;
    }
}

void ARTSPConnection::onReceiveRequest() {  //Server receives clients' requests
	mReceiveRequestEventPending = false;
	
	 if (mState != CONNECTED) {
		 return;
	 }
	
	 struct timeval tv;
	 tv.tv_sec = 0;
	 tv.tv_usec = kSelectTimeoutUs;
	
	 fd_set rs;
	 FD_ZERO(&rs);
	 FD_SET(mSocket, &rs);
	
	 int res = select(mSocket + 1, &rs, NULL, NULL, &tv);
	 CHECK_GE(res, 0);
	
	 if (res == 1) {
		 MakeSocketBlocking(mSocket, true);
	
		 bool success = receiveRTSPRequest();
	
		 MakeSocketBlocking(mSocket, false);
	
		 if (!success) {
			 // Something horrible, irreparable has happened.
			 //flushPendingRequests();
			 //return;
		 }
	 }
	
	 postReceiveRequestEvent();

}

//liuchangyou
void ARTSPConnection::postReceiveRequestEvent() {
    if (mReceiveRequestEventPending) {
        return;
    }
    sp<AMessage> msg = new AMessage(kWhatReceiveRequest, id());
    msg->post();

    mReceiveRequestEventPending = true;
}

status_t ARTSPConnection::receive(void *data, size_t size) {
    size_t offset = 0;
	sp<AMessage> notify;
    while (offset < size) {
        ssize_t n = recv(mSocket, (uint8_t *)data + offset, size - offset, 0);
//		LOGE(LOG_TAG,"recv %d byte %c\n",n,*(uint8_t *)data);//lcy sdebug
        if (n == 0) {
            // Server closed the connection.
            LOGE(LOG_TAG,"Client unexpectedly closed the connection.");
			mState = DISCONNECTED;
			notify = new AMessage(kwhatCloseSession,mhandlerID);
			notify->setInt32("result",-1);
			notify->setInt32("sessionID",mSessionID);
			notify->post();			
			
            return -1;
        } else if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
			mState = DISCONNECTED;
            LOGE(LOG_TAG,"Error reading rtsp response.");
			notify = new AMessage(kwhatCloseSession,mhandlerID);
			notify->setInt32("result",-1);
			notify->setInt32("sessionID",mSessionID);
			notify->post();	
            return -errno;
        }

        offset += (size_t)n;
    }

    return OK;
}

bool ARTSPConnection::receiveLine(AString *line) {
    line->clear();

    bool sawCR = false;
    for (;;) {
        char c;
        if (receive(&c, 1) != OK) {
            return false;
        }

        if (sawCR && c == '\n') {
            line->erase(line->size() - 1, 1);
			//LOGI(LOG_TAG,"receive line OK \n%s\n",line->c_str());
            return true;
        }

        line->append(&c, 1);

        if (c == '$' && line->size() == 1) {
            // Special-case for interleaved binary data.
            return true;
        }

        sawCR = (c == '\r');
    }
}

bool ARTSPConnection::receiveRTSPRequest() {
    AString requestLine;
	LOGI(LOG_TAG,"start receiveLine ......\n");
    if (!receiveLine(&requestLine)) {
        return false;
    }
//	LOGI(LOG_TAG,"receiveLine OK\n");
    sp<AMessage>  request = new AMessage(kWhatRequest,mhandlerID);
	request->setInt32("SessionID",mSessionID);
	LOGI(LOG_TAG,"request->setInt32 SessionID %d\n",mSessionID);
    LOGI(LOG_TAG,"request: %s\n", requestLine.c_str());

    ssize_t space1 = requestLine.find(" ");//寻找空格
    if (space1 < 0) {
        return false;
    }
    ssize_t space2 = requestLine.find(" ", space1 + 1);
    if (space2 < 0) {
        return false;
    }
	LOGI(LOG_TAG,"SPACE1 %d SPACE2 %d",space1,space2);

    AString Method(requestLine.c_str(), space1);
	request->setString("Method",Method.c_str());
	AString URI(requestLine,space1+1,space2-space1-1);
	request->setString("URI",URI.c_str());
	LOGI(LOG_TAG,"SPACE1 %s SPACE2 %s",Method.c_str(),URI.c_str());
    AString line;
    for (;;) {
        if (!receiveLine(&line)) {
            break;
        }

        if (line.empty()) {
            break;
        }

        ssize_t colonPos = line.find(":");
        if (colonPos < 0) {
            // Malformed header line.
            return false;
        }

        AString key(line, 0, colonPos);
        key.trim();
       // key.tolower();

        line.erase(0, colonPos + 1);
        line.trim();
		
        LOGI(LOG_TAG,"line: %s:%s\n", key.c_str(),line.c_str());

	    request->setString(key.c_str(),line.c_str());	
    }
	LOGI(LOG_TAG,"Post the request to handler\n");
    request->post();//将请求消息发送给uplayer 处理
	return OK;
}


void ARTSPConnection::setNonce(const char * nonce)
{
	mNonce.setTo(nonce);
}

const char* ARTSPConnection::getNonce()
{
	return mNonce.c_str();
}


