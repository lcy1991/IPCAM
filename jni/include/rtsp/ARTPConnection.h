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

#ifndef A_RTP_CONNECTION_H_

#define A_RTP_CONNECTION_H_

#include "foundation/AHandler.h"
#include "rtsp/ARTPSource.h"
#include<list>
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace std;

struct ABuffer;
struct ARTPSource;
//struct ASessionDescription;

struct StreamInfo {
	int mRTPSocket;
	int mRTCPSocket;
	int mIndex;
	sp<AMessage> mNotifyMsg;
	struct sockaddr_in mRemoteRTPAddr;
};
enum {
	kWhatAddStream,
	kWhatRemoveStream,
};

struct ARTPConnection : public AHandler {
    enum Flags {
        kFakeTimestamps      = 1,
        kRegularlyRequestFIR = 2,
    };
	
	typedef struct RTPHeader
	{
		uint8_t v;	 //2bit
		uint8_t p;	 //1bit
		uint8_t x;	 //1bit
		uint8_t cc;  //4bit
		uint8_t m;	 //1bit
		uint8_t pt;  //7bit
		uint16_t seq;///16bit
		uint32_t timestamp;//32bit
		uint32_t ssrc;	   //32bit
		
	}RTPHeader;
	
	/******************************************************************
	RTP_FIXED_HEADER
	0					1					2					3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|V=2|P|X|  CC	|M| 	PT		|		sequence number 		|
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|							timestamp							|
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|			synchronization source (SSRC) identifier			|
	+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
	|			 contributing source (CSRC) identifiers 			|
	|							  ....								|
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	
	******************************************************************/
	typedef struct 
	{
		/* byte 0 */
		unsigned char csrc_len:4; /* CC expect 0 */
		unsigned char extension:1;/* X	expect 1, see RTP_OP below */
		unsigned char padding:1;  /* P	expect 0 */
		unsigned char version:2;  /* V	expect 2 */
		/* byte 1 */
		unsigned char payload:7; /* PT	RTP_PAYLOAD_RTSP */
		unsigned char marker:1;  /* M	expect 1 */
		/* byte 2,3 */
		unsigned short seq_no;	 /*sequence number*/
		/* byte 4-7 */
		unsigned  long timestamp;
		/* byte 8-11 */
		unsigned long ssrc; /* stream number is used here. */
	} RTP_FIXED_HEADER;/*12 bytes*/
	
	/******************************************************************
	NALU_HEADER
	+---------------+
	|0|1|2|3|4|5|6|7|
	+-+-+-+-+-+-+-+-+
	|F|NRI|  Type	|
	+---------------+
	******************************************************************/
	typedef struct {
		//byte 0
		unsigned char TYPE:5;
		unsigned char NRI:2;
		unsigned char F:1;
	} NALU_HEADER; /* 1 byte */
	
	
	/******************************************************************
	FU_INDICATOR
	+---------------+
	|0|1|2|3|4|5|6|7|
	+-+-+-+-+-+-+-+-+
	|F|NRI|  Type	|
	+---------------+
	******************************************************************/
	typedef struct {
		//byte 0
		unsigned char TYPE:5;
		unsigned char NRI:2; 
		unsigned char F:1;		   
	} FU_INDICATOR; /*1 byte */
	
	
	/******************************************************************
	FU_HEADER
	+---------------+
	|0|1|2|3|4|5|6|7|
	+-+-+-+-+-+-+-+-+
	|S|E|R|  Type	|
	+---------------+
	******************************************************************/
	typedef struct {
		//byte 0
		unsigned char TYPE:5;
		unsigned char R:1;
		unsigned char E:1;
		unsigned char S:1;	  
	} FU_HEADER; /* 1 byte */
	
	typedef struct
	{
		unsigned len;				  //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
		unsigned max_size;			  //! Nal Unit Buffer size
		int forbidden_bit;			  //! should be always FALSE
		int nal_reference_idc;		  //! NALU_PRIORITY_xxxx
		int nal_unit_type;			  //! NALU_TYPE_xxxx	
		uint8_t *buf;					  //! contains the first byte followed by the EBSP
		unsigned short lost_packets;  //! true, if packet loss is detected
	} NALU_t;
	



    ARTPConnection();
    virtual ~ARTPConnection();

    void addStream(
        int rtpSocket, int rtcpSocket,
        size_t index,const char* address,int remoutPort);

    void removeStream(int index);

    void injectPacket(int index, const sp<ABuffer> &buffer);

    void fakeTimestamps();

	void setSource(ARTPSource*  src);

	bool getStatus();

protected:

    virtual void onMessageReceived(const sp<AMessage> &msg);

private:

	
	uint32_t timeStamp;
	uint16_t seqNum;

    static const int64_t kSelectTimeoutUs;
	uint32_t mRtpPort;
    uint32_t mFlags;
	uint32_t mTimestamp;//RTP timestamp
	RTPHeader mRTPHeader;
	ARTPSource* mRTPSource;
    list<StreamInfo> mStreams;
	Mutex mStreamLock;
    bool mPollEventPending;
    int64_t mLastReceiverReportTimeUs;
	bool mThreadRunFlag;
    void onAddStream(const sp<AMessage> &msg);
    void onRemoveStream(const sp<AMessage> &msg);
    void onPollStreams();
    void onInjectPacket(const sp<AMessage> &msg);
	void onSendPacket(const sp<AMessage> &msg);
	void makeRTPHeader(RTPHeader* h,uint8_t out[12]);
	int GetAnnexbNALU (NALU_t *nalu,const sp<ABuffer> &buf);
	void dump(NALU_t *n);
	void sendRTPPacket(const uint8_t* buf ,size_t bytes);

static	void* threadloop(void* arg);

//    status_t receive(StreamInfo *info, bool receiveRTP);

//    status_t parseRTP(StreamInfo *info, const sp<ABuffer> &buffer);
//    status_t parseRTCP(StreamInfo *info, const sp<ABuffer> &buffer);
//    status_t parseSR(StreamInfo *info, const uint8_t *data, size_t size);
//    status_t parseBYE(StreamInfo *info, const uint8_t *data, size_t size);
	bool isFirstNalu(uint8_t* data,size_t length);
	status_t RTPPacket(sp<ABuffer> buf);

    sp<ARTPSource> findSource(StreamInfo *info, uint32_t id);

    void postPollEvent();

    DISALLOW_EVIL_CONSTRUCTORS(ARTPConnection);
};



#endif  // A_RTP_CONNECTION_H_
