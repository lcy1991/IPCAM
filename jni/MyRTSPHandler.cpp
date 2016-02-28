#include "rtsp/MyRTSPHandler.h"
#include "our_md5.h"
#include "rtsp/uuid.h"
#include "rtsp/base64.h"
#include "stage_utils.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>


#define RTSP_PORT 5544
#define LOG_TAG  "IPCAM-RTSP"

const char* passwd = "123";
const char* user = "123";
const char* realm = "android";
const char* uri="/ch0/live";
//coolpad720*480
uint8_t spset[13] = {0x67 ,0x42, 0x00, 0x0d, 0x96, 0x54, 0x05, 0xA1, 0xe8, 0x80 ,0x01, 0x00, 0x04};   //coolpad
  //uint8_t spset[16] = {0x67, 0x42, 0x80, 0x1E, 0xDA, 0x02, 0x80, 0xF6, 0x80, 0x6D, 0x0A, 0x13, 0x50, 0x01, 0x00, 0x04};//vivo x710
uint8_t ppset[12] = {0x68 ,0xce ,0x38 ,0x80 ,0x00 ,0x00 ,0x00 ,0x10 ,0x70 ,0x61 ,0x73 ,0x70  };//coolpad
  //uint8_t ppset[12] = {0x68, 0xCE, 0x06, 0xE2, 0x00, 0x00, 0x00, 0x10, 0x70, 0x61, 0x73, 0x70};//vivo x710

void bumpSocketBufferSize(int s) {
		int size = 256 * 1024;
		CHECK_EQ(setsockopt(s, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)), 0);
}

unsigned MakePortPair(
		int *rtpSocket, int *rtcpSocket) 
{
	unsigned port ;
	struct sockaddr_in addr;
	*rtpSocket = socket(AF_INET, SOCK_DGRAM, 0);
	CHECK_GE(*rtpSocket, 0);

	bumpSocketBufferSize(*rtpSocket);

	*rtcpSocket = socket(AF_INET, SOCK_DGRAM, 0);
	CHECK_GE(*rtcpSocket, 0);

	bumpSocketBufferSize(*rtcpSocket);

	unsigned start = (rand() * 1000)/ RAND_MAX + 15550;
	start &= ~1;

	for (port = start; port < 65536; port += 2) {

		addr.sin_port = htons(port);

		if (bind(*rtpSocket,
				 (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
			continue;
		}

		addr.sin_port = htons(port + 1);

		if (bind(*rtcpSocket,
				 (const struct sockaddr *)&addr, sizeof(addr)) == 0) {
			break;//return port;
		}
	}

	addr.sin_port = htons(port);
	if (connect(*rtpSocket,(const struct sockaddr *)&addr, sizeof(addr)) < 0) {
		LOGE("MakePort","rtpSocket connect to client failed!");
		return -1;
	}
	
	addr.sin_port = htons(port + 1);
	if (connect(*rtcpSocket,(const struct sockaddr *)&addr, sizeof(addr)) < 0) {
		LOGE("MakePort","rtcpSocket connect to client failed!");
		return -1;
	}
	return port;// return rtp port and rtcp port

}


//向服务器发送心跳包
MyRTSPHandler::MyRTSPHandler()
	: mRunningFlag(false),
	  session_id(0),
	  mtempSessionID(0)
{

}
MyRTSPHandler::~MyRTSPHandler()
{
	close(mSocket);
}

void MyRTSPHandler::calcMD5part1()
{
	AString tmpStr;
	char hostip[40];
	getHostIP(hostip);
	mURI.append("rtsp://");
	mURI.append(hostip);
	mURI.append(":");
	mURI.append(RTSP_PORT);
	mURI.append(uri);
	LOGI(LOG_TAG,"uri--->%s",mURI.c_str());
	tmpStr.append(user);
	tmpStr.append(":");
	tmpStr.append(realm);
	tmpStr.append(":");
	tmpStr.append(passwd);
	//md5(username:realm:password)
	MD5_encode(tmpStr.c_str(),mMD5part1);	
}

void MyRTSPHandler::setRTPConnection(ARTPConnection* RTPConn)
{
	mRTPConnPt = RTPConn;
}

void MyRTSPHandler::onMessageReceived(const sp<AMessage> &msg)
{
	switch(msg->what())
		{
			case kwhatCloseSession:
				onCloseSession(msg);
				break;
			case kWhatRequest:
				LOGI(LOG_TAG,"MyRTSPHandler receive client request\n");
				onReceiveRequest(msg);
			    break;
		}
}
void MyRTSPHandler::onCloseSession(const sp<AMessage> &msg)
{
	int32_t sessionID = 0 ;
	int32_t result = 0 ;
	ARTSPConnection* Conn;
//	return;
	map<uint32_t,ARTSPConnection*>::iterator iter;
	msg->findInt32("sessionID",&sessionID);
	msg->findInt32("result",&result);
	iter = mSessions.find(sessionID);	
	if(iter != mSessions.end()){
		Conn = iter->second;
	    if (Conn != NULL) {
			mlooper.unregisterHandler(Conn->id());
			delete Conn;
	        LOGI(LOG_TAG,"Session %d is deleted ,result %d",sessionID,result);
	        mSessions.erase(iter);
		}
	}
	else LOGI(LOG_TAG,"Can not find the session %d!",sessionID);
}

void MyRTSPHandler::onReceiveRequest(const sp<AMessage> &msg)
{
	int32_t sessionID=0 ;
	ARTSPConnection* Conn;
	AString method;
	AString cseq;
	AString tmpStr;
	AString URI;
	int cseqNum;
	AString response;
	ReqMethod ReqMethodNum;
	uuid_t uuid;
	char randomID[33];
	map<uint32_t,ARTSPConnection*>::iterator iter;

	msg->findInt32("SessionID",&sessionID);
//	LOGW(LOG_TAG,"findInt32 SessionID %d\n",sessionID);
	iter = mSessions.find(sessionID);	
	if(iter != mSessions.end()){
		Conn = iter->second;
	    if (Conn == NULL) {
	        LOGW(LOG_TAG,"failed to find session %d object ,it is NULL\n",sessionID);
	        mSessions.erase(iter);
	        return;
		}
	}
	else{
		LOGW(LOG_TAG,"There is no session ID %d\n",sessionID);
    	return;
	}
	msg->findString("Method",&method);
	msg->findString("CSeq",&cseq);
	cseqNum = atoi(cseq.c_str());
	
	do{
		if(strcmp(method.c_str(),"DESCRIBE")==0)
			{
				ReqMethodNum = DESCRIBE;
				msg->findString("URI",&URI);
				LOGI(LOG_TAG,"findString Method %s CSeq %d URI:%s",method.c_str(),cseqNum,URI.c_str());
				if(passwd!=""&&user!="")
					{
						if(msg->findString("Authorization",&tmpStr)==false)//no Authorization
							{
								sendUnauthenticatedResponse(Conn,cseqNum);
							}
						else
							{
								if(!isAuthenticate(Conn->getNonce(),tmpStr,"DESCRIBE"))
									sendUnauthenticatedResponse(Conn,cseqNum);
								else{
								makeSDP(spset,13,ppset,12);
								response.append("RTSP/1.0 200 OK\r\n");
								response.append("CSeq: ");
								response.append(cseqNum);
								response.append("\r\nContent-Base: ");
								response.append(mURI.c_str());
								response.append("\r\nContent-Type: application/sdp\r\nContent-Length: ");
								response.append(mSDP.size());
								response.append("\r\n\r\n");
								response.append(mSDP);
								//LOGI(LOG_TAG,"%s",response.c_str());
								Conn->sendResponse(response.c_str());	
								mSDP.clear();
								}
							}					
					}

				break;
			}
		if (strcmp(method.c_str(),"OPTIONS")==0)
			{
				response.append("RTSP/1.0 200 OK\r\n");
				response.append("CSeq: ");
				response.append(cseqNum);
				response.append("\r\n");
				response.append("Public: DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE\r\n\r\n");
				LOGI(LOG_TAG,"%s",response.c_str());
				Conn->sendResponse(response.c_str());
				ReqMethodNum = OPTION;
				break;
			}

		if (strcmp(method.c_str(),"SETUP")==0)
			{
				ReqMethodNum = SETUP;
				if(passwd!=""&&user!="")
					{
						if(msg->findString("Authorization",&tmpStr)==false)//no Authorization
							{
								sendUnauthenticatedResponse(Conn,cseqNum);
							}
						else
							{
								if(!isAuthenticate(Conn->getNonce(),tmpStr,"SETUP"))
									sendUnauthenticatedResponse(Conn,cseqNum);
								else
									{
										LOGI(LOG_TAG,"%s","setup Authenticate pass\n");
										//find client rtp/rtcp port 
										//Transport: RTP/AVP;unicast;client_port=4588-4589\r\n									\r\n
										AString Transport;
										uint8_t fstConlon;
										uint8_t sndConlon;
										uint8_t part1;// =
										uint8_t part2;// -
										if(msg->findString("Transport",&Transport))
											{
												fstConlon = Transport.find(";",9);
												sndConlon = Transport.find(";",fstConlon+1);
												part1 = Transport.find("=");
												part2 = Transport.find("-",part1+1);
												AString rtpPort(Transport,part1+1,part2-part1-1);
												Conn->mRemoteRtpPort = atoi(rtpPort.c_str());
												LOGI(LOG_TAG,"client rtp port:%d ",Conn->mRemoteRtpPort);
												Conn->mLocalRtpPort = MakePortPair(&Conn->rtpSocket,&Conn->rtcpSocket);
												if(Conn->mLocalRtpPort > 0)
													{
														response.append("RTSP/1.0 200 OK\r\n");
														response.append("CSeq: ");
														response.append(cseqNum);
														response.append("\r\n");
														response.append("Session: ");
														response.append(Conn->mSessionID);
														response.append("\r\n");
														response.append("Transport: RTP/AVP;unicast;client_port=");
														response.append(Conn->mRemoteRtpPort);
														response.append("-");
														response.append(Conn->mRemoteRtpPort+1);
														response.append(";server_port=");
														response.append(Conn->mLocalRtpPort);
														response.append("-");
														response.append(Conn->mLocalRtpPort+1);
														response.append("\r\n\r\n");
														LOGI(LOG_TAG,"%s",response.c_str());
														Conn->sendResponse(response.c_str());
														ReqMethodNum = OPTION;
														break;
													}
												
											}
										else break;
									}
							}					
					}

				break;
			}
		if (strcmp(method.c_str(),"PLAY")==0)
			{
				ReqMethodNum = PLAY;
				if(passwd!=""&&user!="")
					{
						if(msg->findString("Authorization",&tmpStr)==false)//no Authorization
							{
								sendUnauthenticatedResponse(Conn,cseqNum);
							}
						else
							{
								if(!isAuthenticate(Conn->getNonce(),tmpStr,"PLAY"))
									sendUnauthenticatedResponse(Conn,cseqNum);
								else
									{
										AString range;
										char ipAddr[16];
									    msg->findString("Range",&range);
										LOGI(LOG_TAG,"%s","PLAY Authenticate pass\n");
										
										response.append("RTSP/1.0 200 OK\r\n");
										response.append("CSeq: ");
										response.append(cseqNum);
										response.append("\r\n");
										response.append("Range: ");
										response.append(range.c_str());
										response.append("\r\n\r\n");
										LOGI(LOG_TAG,"%s",response.c_str());
										Conn->sendResponse(response.c_str());//
										ReqMethodNum = PLAY;
										strcpy(ipAddr,inet_ntoa(Conn->mClient_addr.sin_addr));  
										mRTPConnPt->addStream(Conn->rtpSocket,Conn->rtcpSocket,Conn->mSessionID,ipAddr,Conn->mRemoteRtpPort);	//Conn->mRemoteRtpPort									
										break;

									}
							}					
					}				
				break;
			}
		if (strcmp(method.c_str(),"PAUSE")==0)
			{
				ReqMethodNum = PAUSE;
				break;
			}
		if (strcmp(method.c_str(),"TEARDOWN")==0)
			{
				ReqMethodNum = TEARDOWN;
				int32_t tmpID = Conn->mSessionID;
				mRTPConnPt->removeStream(Conn->mSessionID);
				response.append("RTSP/1.0 200 OK\r\n");
				response.append("CSeq: ");
				response.append(cseqNum);
				response.append("\r\n\r\n");
				msg->setWhat(kwhatCloseSession);
				msg->post();
				LOGI(LOG_TAG,"TEARDOWN session %d",tmpID);
				break;
			}
			
		if (strcmp(method.c_str(),"SET_PARAMETER")==0)
			{
				ReqMethodNum = SET_PARAMETER;
				break;
			}

	}while(0);

		
	
		
	
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
void MyRTSPHandler::StartServer()
{
	pthread_create(&mtempTID,NULL,ServerThread,(void *)this);		
	return;
}



void* MyRTSPHandler::ServerThread(void* arg)
{
	//mlooper.registerHandler(this);
	MyRTSPHandler* handpt = (MyRTSPHandler*)arg;
	handpt->mlooper.start();
    handpt->mSocket = socket(AF_INET, SOCK_STREAM, 0);
	handpt->calcMD5part1();
	if(handpt->mSocket < 0)
		LOGE(__FUNCTION__,"socket() %s",strerror(errno));
	handpt->mRunningFlag = true;
    MakeSocketBlocking(handpt->mSocket, true);
    struct sockaddr_in server_addr,peerAddr;
	struct sockaddr_in addr;
    socklen_t addrlen;
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY); ;
    server_addr.sin_port = htons(RTSP_PORT);
	LOGI(LOG_TAG,"rtsp StartServer\n");
	if(-1 == (bind(handpt->mSocket, (struct sockaddr*)&server_addr, sizeof(server_addr)))) 
	  { 
		LOGE(LOG_TAG,"Server Bind Failed:%s",strerror(errno)); 
		//exit(1); 
		return NULL;
	  } 
	// socket监听
	int err;
	err = listen(handpt->mSocket, 15);//使主动连接套接字变成监听套接字
	if(err < 0){
		LOGE(LOG_TAG,"Server listen Failed:%s",strerror(errno)); 
		return NULL;
	}
	LOGI(LOG_TAG,"rtsp Start listening\n");
	while(handpt->mRunningFlag)
		{
		   //mSocketAccept需要锁保护
		    //pthread_mutex_lock(&mMutex);
		    char ipAddr[16];
		    memset(&addr,0,sizeof(struct sockaddr));
			addrlen = sizeof(struct sockaddr);
		    LOGI(LOG_TAG,"rtsp Start accept\n");
			handpt->mSocketAccept = accept(handpt->mSocket,(struct sockaddr *)&addr,&addrlen);
			strcpy(ipAddr,inet_ntoa(addr.sin_addr));  
			LOGI(LOG_TAG,"connected peer address = %s:%d\n",ipAddr, ntohs(addr.sin_port));
			
			handpt->mtempSessionID++;
			
			ARTSPConnection* rtspConn = new ARTSPConnection();
			rtspConn->mClient_addr = addr;
			handpt->mSessions.insert(make_pair(handpt->mtempSessionID, rtspConn));
			handpt->mlooper.registerHandler(rtspConn);
			rtspConn->StartListen(handpt->mSocketAccept,handpt->id(),handpt->mtempSessionID);


			
			//pthread_create(&mtempTID,NULL,NewSession,(void *)this);		
			LOGE(LOG_TAG,"mtempSessionID[%d]\n",handpt->mtempSessionID);			
		}

}


/*
session_id
socket_fd
status
*/
void MyRTSPHandler::StopServer()
{
	mRunningFlag = false;
}

void* MyRTSPHandler::NewSession(void* arg)
{
	MyRTSPHandler* handlerPt = (MyRTSPHandler*)arg;
	ARTSPConnection* rtspConn = new ARTSPConnection();
	handlerPt->mSessions.insert(make_pair(handlerPt->mtempSessionID, rtspConn));
	handlerPt->mlooper.registerHandler(rtspConn);
	rtspConn->StartListen(handlerPt->mSocketAccept,handlerPt->id(),handlerPt->mtempSessionID);
	pthread_mutex_unlock(&handlerPt->mMutex);
	while(rtspConn->mState != DISCONNECTED)
		{
			sleep(1);
		}
	sp<AMessage> notify = new AMessage(kwhatCloseSession,handlerPt->id());
	notify->setInt32("sessionID",rtspConn->mSessionID);
	notify->post();
	LOGI(LOG_TAG,"session %d is closed\n",rtspConn->mSessionID);
}

// response= md5(md5(username:realm:password):nonce:md5(public_method:url));
void MyRTSPHandler::getDigest(const char* NONCE,const char* public_method,AString *result)
{
	AString tmpStr;
	AString part3;
	char part3_md5[33];
	char ret[33];
	tmpStr.append(mMD5part1);
	//LOGI(LOG_TAG,"mMD5part1:%s",mMD5part1);
	tmpStr.append(":");
	tmpStr.append(NONCE);
	//LOGI(LOG_TAG,"NONCE:%s",NONCE);
	tmpStr.append(":");
	part3.append(public_method);
	part3.append(":");
	part3.append(mURI);
	MD5_encode(part3.c_str(),part3_md5);
	//LOGI(LOG_TAG,"part3_md5:%s",part3_md5);
	
	tmpStr.append(part3_md5);
	//LOGI(LOG_TAG,"[response]:%s",tmpStr.c_str());
	MD5_encode(tmpStr.c_str(),ret);
	
	result->setTo(ret);
	LOGI(LOG_TAG,"Digest result:%s",ret);
}


//#ifdef ANDROID   
#if 1
int MyRTSPHandler::getHostIP (char addressBuffer[40]) 
{
	if(strlen(hostIP)==0)
		sprintf(addressBuffer,"%s","192.168.1.105");
	else 
	    sprintf(addressBuffer,"%s",hostIP);
	return 0;
}

void MyRTSPHandler::setHostIP (char* addr) 
{
	sprintf(hostIP,"%s",addr);
}


#else
#include <ifaddrs.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int MyRTSPHandler::getHostIP(char addressBuffer[40]) 
{

    struct ifaddrs * ifAddrStruct=NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);

    while (ifAddrStruct!=NULL) 
	{
        if (ifAddrStruct->ifa_addr->sa_family==AF_INET)
		{   // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
			if(strcmp(ifAddrStruct->ifa_name,"eth0")==0)
				{
					inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            		//printf("%s IPV4 Address %s\n", ifAddrStruct->ifa_name, addressBuffer); 
           			break;
				}

        }
		else if (ifAddrStruct->ifa_addr->sa_family==AF_INET6)
		{   // check it is IP6
            // is a valid IP6 Address
            //tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            //char addressBuffer[INET6_ADDRSTRLEN];
            //inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            //printf("%s IPV6 Address %s\n", ifAddrStruct->ifa_name, addressBuffer); 
        } 
        ifAddrStruct = ifAddrStruct->ifa_next;
    }
    return 0;
}
#endif


bool MyRTSPHandler::isAuthenticate(const char* NONCE,AString& tmpStr,const char* method)
{
	AString Digest;
	ssize_t resp;
	resp = tmpStr.find("response");
	
	AString response(tmpStr, resp + 10, 32);
	LOGI(LOG_TAG,"~~~Client's response %s",response.c_str());
	getDigest(NONCE,method,&Digest);
	if(Digest==response)
		{
			LOGI(LOG_TAG,"Authenticate passed !!!!");
			return true;
		}
	else return false;
}

void MyRTSPHandler::sendUnauthenticatedResponse(ARTSPConnection* Conn,int cseqNum)
{
	AString response;
	uuid_t uuid;
	char randomID[33];
	uuid_generate(uuid);
	sprintf(randomID,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		uuid[0],uuid[1],uuid[2],uuid[3],uuid[4],uuid[5],uuid[6],uuid[7],
		uuid[8],uuid[9],uuid[10],uuid[11],uuid[12],uuid[13],uuid[14],uuid[15]);
	Conn->setNonce(randomID);
	response.append("RTSP/1.0 401 Unauthorized\r\n");
	response.append("CSeq: ");
	response.append(cseqNum);
	response.append("\r\n");
	response.append("WWW-Authenticate: Digest realm=\"");
	response.append(realm);
	response.append("\",nonce=\"");
	response.append(randomID);
	response.append("\"\r\n\r\n");
	//LOGI(LOG_TAG,"%s",response.c_str());
	Conn->sendResponse(response.c_str());

}

void MyRTSPHandler::makeSDP(uint8_t* SPS,uint32_t SPS_len,uint8_t* PPS,uint32_t PPS_len)
{
	char profile_level_id[7];
	AString sps;
	AString pps;
    sprintf(profile_level_id,"%02X%02X%02X",SPS[1],SPS[2],SPS[3]);
	encodeBase64(SPS,SPS_len,&sps);
	encodeBase64(PPS,PPS_len,&pps);
	mSDP.append("v=0\r\n");
	mSDP.append("o=- 1439819707757719 1 IN IP4 192.168.1.107\r\n");
	mSDP.append("s=H.264 Video, streamed by the android media Server\r\n");
	mSDP.append("i=test.264\r\n");
	mSDP.append("t=0 0\r\n"
                "a=tool:android media Server 2015\r\n"
				"a=type:broadcast\r\n"
				"a=control:*\r\n"
				"a=range:npt=0-\r\n"
				"a=x-qt-text-nam:H.264 Video, streamed by the android media Server\r\n"
				"a=x-qt-text-inf:test.264\r\n"
				"m=video 0 RTP/AVP 96\r\n"
				"c=IN IP4 0.0.0.0\r\n"
				"b=AS:500\r\n"
				"a=rtpmap:96 H264/90000\r\n"
				"a=fmtp:96 packetization-mode=1;profile-level-id=");
	mSDP.append(profile_level_id);
	mSDP.append(";sprop-parameter-sets=");
	mSDP.append(sps);
	mSDP.append(",");
	mSDP.append(pps);
	mSDP.append("\r\n");
	mSDP.append("a=control:track1\r\n");

}
/*
SDP协议格式
v=0
o=- 1439819707757719 1 IN IP4 192.168.1.107
s=H.264 Video, streamed by the LIVE555 Media Server
i=test.264
t=0 0
a=tool:LIVE555 Streaming Media v2013.07.03
a=type:broadcast
a=control:*
a=range:npt=0-
a=x-qt-text-nam:H.264 Video, streamed by the LIVE555 Media Server
a=x-qt-text-inf:test.264
m=video 0 RTP/AVP 96
c=IN IP4 0.0.0.0
b=AS:500
a=rtpmap:96 H264/90000
a=fmtp:96 packetization-mode=1;profile-level-id=42001F;sprop-parameter-sets=Z0IAH5ZUBQHogAEABA==,aM44gAAAABBwYXNw
a=control:track1
                                                                                                                    Z0IgH5ZUBQHogAEgBA==,aM44gCAgIBBwYXNw
profile-level-id 的值 是从SPS的第二个字节开始的三个字节
sprop-parameter-sets SPS,PPS base64 编码



*/



