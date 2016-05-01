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
#include <time.h>
#include "foundation/AString.h"
#include "foundation/ALooper.h"
#include "foundation/ADebug.h"
#include "rtsp/MyRTSPHandler.h"
#include "rtsp/ARTPSource.h"
#include "rtsp/ARTPConnection.h"
#include "com_liuuu_ipcamera_IPCAM.h"
#include "stage_utils.h"
#define LOG_TAG "IPCAM"

#if 1


/* This is a trivial JNI example where we use a native method
 * to return a new VM String. See the corresponding Java source
 * file located at:
 *
 *   apps/samples/hello-jni/project/src/com/example/hellojni/HelloJni.java
 */
#define BUFLEN (1024*500)
int getBoxLen(int localSocket);
//./openRTSP.exe -u 123 123 rtsp://192.168.1.107:5544/ch0/live
ARTPSource mysource(5,50000); 

inline int recvN(int localSocket,char* buf,uint32_t N)
{
	uint32_t receivedLen = 0;
	int ret = 0;
	uint32_t tmpLen = N;
	while(1)
	{
		ret = recv(localSocket,buf + receivedLen,tmpLen,0);
		if(ret < 0)
			{
				LOGE(LOG_TAG,"%s",strerror(errno));
				return -1;
			}
		receivedLen += ret;
		if(receivedLen == N)break;
		else tmpLen -= ret;
	}
	return receivedLen;
}

#define IP_LENGTH  40
void* start(void*){

//*********************************
	FILE* bits;
	bits = fopen("test.264","r");

//*********************************
	int localsocket, len;
	int recv_size=0;
	int err;
	socklen_t optlen;
	struct sockaddr_un remote;

	ALooper looper1;
	MyRTSPHandler handler_rtsp;
	ARTPConnection handler_rtp;
	
	char localIP[IP_LENGTH];
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
		LOGE(LOG_TAG,"get recv buffer length error\n");
	else
		LOGE(LOG_TAG,"recv buffer length = %d",recv_size);
	//设置新的缓冲区大小
	recv_size = 8*1024;    /* 接收缓冲区大小为500K */
	optlen = sizeof(recv_size);
	err = setsockopt(localsocket,SOL_SOCKET,SO_RCVBUF, (char *)&recv_size, optlen);
	if(err<0)
		LOGE(LOG_TAG,"set recv buffer length error\n");
	//获取新的缓冲区大小
	optlen = sizeof(recv_size);
	err = getsockopt(localsocket, SOL_SOCKET, SO_RCVBUF, &recv_size, &optlen);
	if(err<0)
		LOGE(LOG_TAG,"get NEW recv buffer length error\n");
	else
		LOGE(LOG_TAG,"NEW recv buffer length = %d",recv_size);

	MakeSocketBlocking(localsocket,false);
	//连接服务器
	while(connect(localsocket, (struct sockaddr *)&remote, len))
	{
		usleep(10000);
	}
	LOGI(LOG_TAG,"connected") ;
	MakeSocketBlocking(localsocket,true);


	if(recv(localsocket,localIP,IP_LENGTH,0) < 0)
		LOGE(LOG_TAG,"Receive IP addr error");
	else
		LOGI(LOG_TAG,"Receive IP addr:%s",localIP);

	//getBoxLen(localsocket);
	//return NULL;

	handler_rtsp.setHostIP(localIP);
	handler_rtp.setSource(&mysource);
	handler_rtsp.setRTPConnection(&handler_rtp);
	looper1.registerHandler(&handler_rtsp);
	looper1.registerHandler(&handler_rtp);
	looper1.start();
	handler_rtsp.StartServer();

	//coolpad720*480
	uint8_t coolpad720x480spset[13] = {0x67 ,0x42, 0x00, 0x0d, 0x96, 0x54, 0x05, 0xA1, 0xe8, 0x80 ,0x01, 0x00, 0x04};   //coolpad
	  //uint8_t spset[16] = {0x67, 0x42, 0x80, 0x1E, 0xDA, 0x02, 0x80, 0xF6, 0x80, 0x6D, 0x0A, 0x13, 0x50, 0x01, 0x00, 0x04};//vivo x710
	uint8_t coolpad720x480ppset[12] = {0x68 ,0xce ,0x38 ,0x80 ,0x00 ,0x00 ,0x00 ,0x10 ,0x70 ,0x61 ,0x73 ,0x70  };//coolpad
	  //uint8_t ppset[12] = {0x68, 0xCE, 0x06, 0xE2, 0x00, 0x00, 0x00, 0x10, 0x70, 0x61, 0x73, 0x70};//vivo x710


	sp<ABuffer> tmpbuf;
	uint32_t Len;
	int i=0,ret;
	uint8_t LenBuf[4];
	uint8_t Buf[100];
	char Signal;
	bool pre_status;
	bool cur_status;
	timespec cur_tv;
	cur_status = pre_status = handler_rtp.getStatus();
	while(1)
	{
		pre_status = cur_status;
		cur_status = handler_rtp.getStatus();
		//LOGI(LOG_TAG,"cur_status:%d",cur_status);
		if(cur_status)
			{
				if(pre_status==false)
					{
						Signal=0x06;
						send(localsocket,&Signal,1,0);
						ret = recv(localsocket,Buf,52,MSG_WAITALL);//"52" it's different between devices!!!!!!!!!!!!!!!
						if(ret < 0)
							LOGE(LOG_TAG,"%s",strerror(errno));
						else LOGI(LOG_TAG,"Receive 1st %d bytes",ret);
						if(mysource.inputQPop(tmpbuf)>=0)
							{
								memcpy((char*)tmpbuf->data(),coolpad720x480spset,sizeof(coolpad720x480spset));
								tmpbuf->setRange(RTP_HEADER_SIZE,sizeof(coolpad720x480spset));
								clock_gettime(CLOCK_REALTIME, &cur_tv);	
								tmpbuf->setTimeStamp(cur_tv.tv_sec*1000 + cur_tv.tv_nsec/1000000);//ms
								mysource.inputQPush(tmpbuf);
							}
						if(mysource.inputQPop(tmpbuf)>=0)
							{
								memcpy((char*)tmpbuf->data(),coolpad720x480ppset,sizeof(coolpad720x480ppset));
								tmpbuf->setRange(RTP_HEADER_SIZE,sizeof(coolpad720x480ppset));
								clock_gettime(CLOCK_REALTIME, &cur_tv);	
								tmpbuf->setTimeStamp(cur_tv.tv_sec*1000 + cur_tv.tv_nsec/1000000);//ms								
								mysource.inputQPush(tmpbuf);
							}

					}
				if(mysource.inputQPop(tmpbuf)>=0)
					{
						ret = recv(localsocket,LenBuf,4,MSG_WAITALL);
						if(ret < 0)	
							{
								LOGE(LOG_TAG,"%s",strerror(errno));
								tmpbuf->setRange(RTP_HEADER_SIZE,0);
							}
						else
							{
								Len = LenBuf[0]<<24 | LenBuf[1]<<16 | LenBuf[2]<<8 | LenBuf[3];
								LOGI(LOG_TAG,"get len %d",Len);
								ret = recvN(localsocket,(char*)tmpbuf->data(),Len);
								if(ret >= 0) 
									{
										tmpbuf->setRange(RTP_HEADER_SIZE,ret);
										clock_gettime(CLOCK_REALTIME, &cur_tv);	
										tmpbuf->setTimeStamp(cur_tv.tv_sec*1000 + cur_tv.tv_nsec/1000000);//ms										
										LOGI(LOG_TAG,"get a NALU length:%d NUM:%d\n",ret,i++);
									}
								else tmpbuf->setRange(RTP_HEADER_SIZE,0);
							}
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


int getBoxLen(int localSocket)
{
	char Buf[BUFLEN];
	int ret,i;
	char header[4] = {0x00,0x00,0x00,0x01};
	uint32_t length;
	FILE *fp;
	fp=fopen("/sdcard/test.264","w+");
	if(fp==NULL) return -1;
	
	char Signal=0x06;
	send(localSocket,&Signal,1,0);	


	ret = recv(localSocket,Buf,52,MSG_WAITALL);
	if(ret < 0)return -1;
	LOGI(LOG_TAG,"receive %d 1st bytes",ret);
	//fwrite(Buf,1,ret,fp);
	memset(Buf,52,0);
	for(i=0;i<1000;i++)
	{
		ret = recv(localSocket,Buf,4,MSG_WAITALL);
		if(ret < 0)return -1;
		fwrite(Buf,1,ret,fp);
//		fwrite(header,1,4,fp);
		length = Buf[0]<<24 | Buf[1]<<16 | Buf[2]<<8 | Buf[3];
        LOGI(LOG_TAG,"recv frame 0x%x 0x%x 0x%x 0x%x bytes %d",Buf[0],Buf[1],Buf[2],Buf[3],length) ;
		ret = recvN(localSocket,Buf,length);
		if(ret<0)return -1;
		fwrite(Buf,1,ret,fp);
	}
	fclose(fp);
	Signal=0x05;
	send(localSocket,&Signal,1,0);	
	close(localSocket);
	return 0;

}

JNIEXPORT jstring JNICALL Java_com_liuuu_ipcamera_IPCAM_helloWorld(JNIEnv *env, jobject obj,jstring inputStr)
{
	const char *str =
	(const char *)env->GetStringUTFChars(inputStr, JNI_FALSE );
	env->ReleaseStringUTFChars(inputStr, (const char *)str );
	return env->NewStringUTF("Hello World! I am Native interface");
}
JNIEXPORT jint JNICALL Java_com_liuuu_ipcamera_IPCAM_startRTSPServer
  (JNIEnv * env, jobject job)
{
	pthread_t tid;
	int ret=0;
	ret = pthread_create(&tid, NULL, start, NULL);
	if(!ret)__android_log_print(ANDROID_LOG_DEBUG,"IPCAM","pthread_create server start success!") ;
    return ret;
}


JNIEXPORT jint JNICALL JNI_OnLoad (JavaVM *vm, void *reserved)
{
	__android_log_print(ANDROID_LOG_DEBUG,"IPCAM","JNI_OnLoad !! \n");
	return JNI_VERSION_1_6;
}


