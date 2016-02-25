/*
 * Copyright (C) 2009 The Android Open Source Project
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
 *
 */
#include <string.h>
#include <jni.h>
#include <android/log.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "foundation/AString.h"
#include "foundation/ALooper.h"
#include "foundation/ADebug.h"
#include "rtsp/MyRTSPHandler.h"
#include "rtsp/ARTPSource.h"
#include "rtsp/ARTPConnection.h"
#include "com_liuuu_ipcamera_IPCAM.h"
#define LOG_TAG "Native"


#if 1


/* This is a trivial JNI example where we use a native method
 * to return a new VM String. See the corresponding Java source
 * file located at:
 *
 *   apps/samples/hello-jni/project/src/com/example/hellojni/HelloJni.java
 */
#define BUFLEN (1024*500)

ARTPSource mysource(5,50000); 

inline int recvN(int localSocket,char* buf,uint32_t N)
{
	uint32_t receivedLen = 0;
	int ret = 0;
	uint32_t tmpLen = N;
	while(1)
	{
		ret = recv(localSocket,buf + receivedLen,tmpLen,0);
		if(ret < 0)return -1;
		receivedLen += ret;
		if(receivedLen == N)break;
		else tmpLen -= ret;
	}
	return receivedLen;
}


void* start(void*){
	int localsocket, len;
	int recv_size=0;
	int err;
	socklen_t optlen;
	struct sockaddr_un remote;

	ALooper looper1;
	MyRTSPHandler handler_rtsp;
	ARTPConnection handler_rtp;
	
	if ((localsocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
	    return 0;
	}
	char *name="VideoCamera";
	remote.sun_path[0] = '\0';  /* abstract namespace */
	strcpy(remote.sun_path+1, name);
	remote.sun_family = AF_UNIX;
	int nameLen = strlen(name);
	len = 1 + nameLen + offsetof(struct sockaddr_un, sun_path);

	//系统默认socket缓冲区大小
	optlen = sizeof(recv_size);
	err = getsockopt(localsocket, SOL_SOCKET, SO_RCVBUF, &recv_size, &optlen);
	if(err<0)
	{
		__android_log_print(ANDROID_LOG_DEBUG,"LCY DEBUG TAG","get recv buffer length error\n");
    }
	else
	__android_log_print(ANDROID_LOG_DEBUG,"LCY DEBUG TAG","recv buffer length = %d",recv_size);
	//设置新的缓冲区大小
	recv_size = 500*1024;    /* 接收缓冲区大小为500K */
	optlen = sizeof(recv_size);
	err = setsockopt(localsocket,SOL_SOCKET,SO_RCVBUF, (char *)&recv_size, optlen);
	if(err<0)
	{
		__android_log_print(ANDROID_LOG_DEBUG,"LCY DEBUG TAG","set recv buffer length error\n");
	}
	//获取新的缓冲区大小
	optlen = sizeof(recv_size);
	err = getsockopt(localsocket, SOL_SOCKET, SO_RCVBUF, &recv_size, &optlen);
	if(err<0)
	{
		__android_log_print(ANDROID_LOG_DEBUG,"LCY DEBUG TAG","get NEW recv buffer length error\n");
    }
	else
	__android_log_print(ANDROID_LOG_DEBUG,"LCY DEBUG TAG","NEW recv buffer length = %d",recv_size);
	//连接服务器
	if (connect(localsocket, (struct sockaddr *)&remote, len) == -1)
	{
		__android_log_print(ANDROID_LOG_DEBUG,"LCY DEBUG TAG","connect error~~~") ;
	    return 0;
	}


	handler_rtp.setSource(&mysource);
	handler_rtsp.setRTPConnection(&handler_rtp);
	looper1.registerHandler(&handler_rtsp);
	looper1.registerHandler(&handler_rtp);
	looper1.start();
	handler_rtsp.StartServer();

	sp<ABuffer> tmpbuf;
	int Len,ret;
	int i=0;
	char LenBuf[4];
	char Signal;
	bool pre_status;
	bool cur_status;
	cur_status = pre_status = handler_rtp.getStatus();
	while(1)
	{
		pre_status = cur_status;
		cur_status = handler_rtp.getStatus();
		
		if(cur_status)
			{
				if(pre_status==false)
					{
						Signal=0x06;
						send(localsocket,&Signal,1,0);					
					}
				if(mysource.inputQPop(tmpbuf)>=0)
					{
						ret = recv(localsocket,LenBuf,4,MSG_WAITALL);
						Len = LenBuf[0]<<24 | LenBuf[1]<<16 | LenBuf[2]<<8 | LenBuf[3];
						ret = recvN(localsocket,(char*)tmpbuf->data(),Len);
						tmpbuf->setRange(0,ret);
						LOGI(LOG_TAG,"get a NALU length:%d NUM:%d\n",ret,i++);
						mysource.inputQPush(tmpbuf);
					}				
			}
		else
			{
				if(pre_status==true)
					{
						Signal=0x05;
						send(localsocket,&Signal,1,0);					
					}
				usleep(10000);
			}
	}

	return 0;
}

#endif
JNIEXPORT jstring JNICALL Java_com_liuuu_ipcamera_IPCAM_helloWorld(JNIEnv *env, jobject obj,jstring inputStr)
{
	const char *str =
	(const char *)env->GetStringUTFChars(inputStr, JNI_FALSE );
	env->ReleaseStringUTFChars(inputStr, (const char *)str );
	return env->NewStringUTF("Hello World! I am Native interface");
}
JNIEXPORT jstring JNICALL Java_com_liuuu_ipcamera_IPCAM_stringFromJNI
  (JNIEnv * env, jobject job)
{
	pthread_t tid;
	int ret=0;
	ret = pthread_create(&tid, NULL, start, NULL);
	if(!ret)__android_log_print(ANDROID_LOG_DEBUG,"IPCAM","pthread_create success!") ;
    return env->NewStringUTF("Hello from JNI !");
}


JNIEXPORT jint JNICALL JNI_OnLoad (JavaVM *vm, void *reserved)
{
	__android_log_print(ANDROID_LOG_DEBUG,"IPCAM","JNI_OnLoad !! \n");
	return JNI_VERSION_1_6;
}


