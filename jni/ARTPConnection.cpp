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
#define LOG_TAG "ARTPConnection"


#include "rtsp/ARTPConnection.h"

#include "rtsp/ARTPSource.h"
//#include "ASessionDescription.h"

#include <foundation/ABuffer.h>
#include <foundation/ADebug.h>
#include <foundation/AMessage.h>
#include <foundation/AString.h>


#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>


const uint8_t ff_ue_golomb_vlc_code[512]=
{ 32,32,32,32,32,32,32,32,31,32,32,32,32,32,32,32,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,
   7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,14,
   3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
   5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
   2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };


static const size_t UDP_MAX_SIZE = 1500;

static uint16_t u16at(const uint8_t *data) {
    return data[0] << 8 | data[1];
}

static uint32_t u32at(const uint8_t *data) {
    return u16at(data) << 16 | u16at(&data[2]);
}

static uint64_t u64at(const uint8_t *data) {
    return (uint64_t)(u32at(data)) << 32 | u32at(&data[4]);
}

// static
const int64_t ARTPConnection::kSelectTimeoutUs = 1000ll;



ARTPConnection::ARTPConnection()
    : 
      mPollEventPending(false),
      mThreadRunFlag(false),
      mLastReceiverReportTimeUs(-1) {
      timeStamp = 0;
	  seqNum = 0;
}

ARTPConnection::~ARTPConnection() {
}

void ARTPConnection::addStream(
        int rtpSocket, int rtcpSocket,
        size_t index,const char* address,int remoutPort) 
{
	LOGI(LOG_TAG,"add Stream index %d",index);
    sp<AMessage> msg = new AMessage(kWhatAddStream, id());
    msg->setInt32("rtp-socket", rtpSocket);
    msg->setInt32("rtcp-socket", rtcpSocket);
	msg->setInt32("RemoteRTPPort", remoutPort);
    msg->setInt32("index", index);
	msg->setString("RemoteRTPAddr",address,strlen(address));
    msg->post();
}

void ARTPConnection::removeStream(int index) {
	LOGI(LOG_TAG,"remove Stream index %d",index);
    sp<AMessage> msg = new AMessage(kWhatRemoveStream, id());
    msg->setInt32("index", index);
    msg->post();
}


void ARTPConnection::onMessageReceived(const sp<AMessage> &msg) {
    switch (msg->what()) {
        case kWhatAddStream:
        {
            onAddStream(msg);
            break;
        }

        case kWhatRemoveStream:
        {
            onRemoveStream(msg);
            break;
        }
        default:
        {
            TRESPASS();
            break;
        }
    }
}

void ARTPConnection::onAddStream(const sp<AMessage> &msg) {
	Mutex::Autolock autoLock(mStreamLock);
    mStreams.push_back(StreamInfo());
    StreamInfo* info = &*--mStreams.end();
    int32_t s;
	AString ipaddr;
	struct sockaddr_in tmpaddr;
	bzero(&tmpaddr,sizeof(tmpaddr));	
	tmpaddr.sin_family=AF_INET;  
    CHECK(msg->findInt32("rtp-socket", &s));
    info->mRTPSocket = s;
    CHECK(msg->findInt32("rtcp-socket", &s));
    info->mRTCPSocket = s;
	CHECK(msg->findInt32("index", &s));
    info->mIndex = s;
	CHECK(msg->findString("RemoteRTPAddr", &ipaddr));
	tmpaddr.sin_addr.s_addr=inet_addr(ipaddr.c_str());
	CHECK(msg->findInt32("RemoteRTPPort", &s));	
	tmpaddr.sin_port=htons(s); 	
	info->mRemoteRTPAddr = tmpaddr;
	
	if(mStreams.size()==1)
		{
			mThreadRunFlag = true;
			pthread_t id;
			pthread_create(&id,NULL,threadloop,(void*)this);
			LOGI(LOG_TAG,"Create RTP transthread");
		}
	LOGI(LOG_TAG,"rtp connection add stream:%d ip:port %s:%d",info->mIndex,inet_ntoa(info->mRemoteRTPAddr.sin_addr),ntohs(info->mRemoteRTPAddr.sin_port));
}

void ARTPConnection::onRemoveStream(const sp<AMessage> &msg) {
    int32_t index;
    CHECK(msg->findInt32("index", &index));
	Mutex::Autolock autoLock(mStreamLock);
    list<StreamInfo>::iterator it = mStreams.begin();
    while (it != mStreams.end()
           && (it->mIndex != index)) {
        ++it;
    }

    if (it == mStreams.end()) {
        TRESPASS();
    }

    mStreams.erase(it);
	if(mStreams.size()==0)
		mThreadRunFlag = false;
}



void ARTPConnection::makeRTPHeader(RTPHeader* h,uint8_t out[12])
{
	uint16_t tmp16;
	uint32_t tmp32;
	out[0] = h->v << 6 | h->p << 5 | h->x << 4 | h->cc;
	out[1] = h->m << 7 | h->pt;
	tmp16 = htons(h->seq);
	out[2] = tmp16 >> 8;
	out[3] = tmp16;
	tmp32 = htonl(h->timestamp);
	out[4] = tmp32 >> 24;
	out[5] = tmp32 >> 16;
	out[6] = tmp32 >> 8;
	out[7] = tmp32;
	tmp32 = htonl(h->ssrc);
	out[8] = tmp32 >> 24;
	out[9] = tmp32 >> 16;
	out[10] = tmp32 >> 8;
	out[11] = tmp32;	
	
}
/*
nal < MUT 1460
nal > MUT

*/

bool ARTPConnection::isFirstNalu(uint8_t* data,size_t length) {

	uint8_t vlu;
	uint32_t BUF[4];
	uint32_t code;
	if(length < 5)
		{
			LOGE(LOG_TAG,"golomb decode error! buf length < 5");
			return false;
		}
	BUF[0] = data[1];
	BUF[1] = data[2];
	BUF[2] = data[3];
	BUF[3] = data[4];
	code = BUF[0]<<24 | BUF[1]<<16 | BUF[2]<<8 | BUF[3];
	if(code >= (1<<27)){                    //�ñ���Ϊ9bit����
		code >>= 32 - 9;                    //��buf����23bit�õ����ش�
		vlu = ff_ue_golomb_vlc_code[code];
	}
	else LOGE(LOG_TAG,"golomb decode error!");
	if(vlu == 0) return true;
	else return false;

}


void ARTPConnection::setSource(ARTPSource* src)
{
	mRTPSource = src;
}

status_t ARTPConnection::RTPPacket(sp<ABuffer> buf)
{

	static uint8_t sendbuf[UDP_MAX_SIZE];
	uint32_t bytes; 
	uint8_t* nalu_payload;
	if(isFirstNalu(buf->data(),buf->size()))
		timeStamp+=(unsigned int)(90000.0 / mRTPSource->mFramerate);
	NALU_t nalu;
	RTP_FIXED_HEADER* rtp_hdr;
	NALU_HEADER* nalu_hdr;
	FU_INDICATOR* fu_ind;
	FU_HEADER* fu_hdr;
	
	memset(&nalu,0,sizeof(NALU_t));
	GetAnnexbNALU(&nalu,buf);//ÿִ��һ�Σ��ļ���ָ��ָ�򱾴��ҵ���NALU��ĩβ����һ��λ�ü�Ϊ�¸�NALU����ʼ��0x000001
	dump(&nalu);//���NALU���Ⱥ�TYPE

	//memset(sendbuf,0,UDP_MAX_SIZE);//���sendbuf����ʱ�Ὣ�ϴε�ʱ�����գ������Ҫts_current�������ϴε�ʱ���ֵ
//	LOGI(LOG_TAG,"RTP TimeStamp %d  seq %d",timeStamp,seqNum);
	//rtp�̶���ͷ��Ϊ12�ֽ�,�þ佫sendbuf[0]�ĵ�ַ����rtp_hdr���Ժ��rtp_hdr��д�������ֱ��д��sendbuf��
	rtp_hdr =(RTP_FIXED_HEADER*)&sendbuf[0]; 
	
	//����RTP HEADER��
	rtp_hdr->version = 2;	//�汾�ţ��˰汾�̶�Ϊ2
	rtp_hdr->marker  = 0;	//��־λ���ɾ���Э��涨��ֵ��
	rtp_hdr->payload = 96;//H264;//�������ͺţ�
	rtp_hdr->ssrc	 = htonl(10);//���ָ��Ϊ10�������ڱ�RTP�Ự��ȫ��Ψһ

	//��һ��NALUС��1400�ֽڵ�ʱ�򣬲���һ����RTP������
	if(nalu.len <= UDP_MAX_SIZE){
		LOGI(LOG_TAG,"nalu length < UDP_MAX_SIZE");
		//����rtp M λ��
		rtp_hdr->marker=1;
		rtp_hdr->seq_no = htons(seqNum++); //���кţ�ÿ����һ��RTP����1
//		LOGI(LOG_TAG,"ts %d",seqNum);
		rtp_hdr->timestamp=htonl(timeStamp);
		//����NALU HEADER,�������HEADER����sendbuf[12]
		nalu_hdr =(NALU_HEADER*)&sendbuf[12]; //��sendbuf[12]�ĵ�ַ����nalu_hdr��֮���nalu_hdr��д��ͽ�д��sendbuf�У�
		nalu_hdr->F=nalu.forbidden_bit;
		nalu_hdr->NRI=nalu.nal_reference_idc>>5;//��Ч������n->nal_reference_idc�ĵ�6��7λ����Ҫ����5λ���ܽ���ֵ����nalu_hdr->NRI��
		nalu_hdr->TYPE=nalu.nal_unit_type;

		nalu_payload=&sendbuf[13];//ͬ��sendbuf[13]����nalu_payload
		memcpy(nalu_payload,nalu.buf+1,nalu.len-1);//ȥ��naluͷ��naluʣ������д��sendbuf[13]��ʼ���ַ�����

		//timeStamp = timeStamp + timestamp_increse;
		
		bytes = nalu.len + 12 ; //���sendbuf�ĳ���,Ϊnalu�ĳ��ȣ�����NALUͷ����ȥ��ʼǰ׺������rtp_header�Ĺ̶�����12�ֽ�
		LOGI(LOG_TAG,"socket send RTP packet seqnum:%d timeStamp: %d",seqNum,timeStamp);
		sendRTPPacket(sendbuf,bytes);//send(socket1,sendbuf,bytes,0);//����RTP��

	}else{
		int packetNum = nalu.len/UDP_MAX_SIZE;
		if (nalu.len%UDP_MAX_SIZE != 0)
			packetNum ++;

		int lastPackSize = nalu.len - (packetNum-1)*UDP_MAX_SIZE;
		int packetIndex = 1 ;

		rtp_hdr->timestamp = htonl(timeStamp);

		//���͵�һ����FU��S=1��E=0��R=0
		rtp_hdr->seq_no = htons(seqNum++); //���кţ�ÿ����һ��RTP����1  
//		LOGI(LOG_TAG,"ts %d",seqNum);
		//����rtp M λ��
		rtp_hdr->marker=0;
		//����FU INDICATOR,�������HEADER����sendbuf[12]
		fu_ind =(FU_INDICATOR*)&sendbuf[12]; //��sendbuf[12]�ĵ�ַ����fu_ind��֮���fu_ind��д��ͽ�д��sendbuf�У�
		fu_ind->F=nalu.forbidden_bit;
		fu_ind->NRI=nalu.nal_reference_idc>>5;
		fu_ind->TYPE=28;

		//����FU HEADER,�������HEADER����sendbuf[13]
		fu_hdr =(FU_HEADER*)&sendbuf[13];
		fu_hdr->S=1;
		fu_hdr->E=0;
		fu_hdr->R=0;
		fu_hdr->TYPE=nalu.nal_unit_type;

		nalu_payload=&sendbuf[14];//ͬ��sendbuf[14]����nalu_payload
		memcpy(nalu_payload,nalu.buf+1,UDP_MAX_SIZE);//ȥ��NALUͷ
		bytes=UDP_MAX_SIZE+14;//���sendbuf�ĳ���,Ϊnalu�ĳ��ȣ���ȥ��ʼǰ׺��NALUͷ������rtp_header��fu_ind��fu_hdr�Ĺ̶�����14�ֽ�
		LOGI(LOG_TAG,"socket send RTP packet seqnum:%d timeStamp: %d",seqNum,timeStamp);
		sendRTPPacket(sendbuf,bytes);//send( socket1, sendbuf, bytes, 0 );//����RTP��

		//�����м��FU��S=0��E=0��R=0
		for(packetIndex=2;packetIndex<packetNum;packetIndex++)
		{
			rtp_hdr->seq_no = htons(seqNum++); //���кţ�ÿ����һ��RTP����1
//	 		LOGI(LOG_TAG,"ts %d",seqNum);
			//����rtp M λ��
			rtp_hdr->marker=0;
			//����FU INDICATOR,�������HEADER����sendbuf[12]
			fu_ind =(FU_INDICATOR*)&sendbuf[12]; //��sendbuf[12]�ĵ�ַ����fu_ind��֮���fu_ind��д��ͽ�д��sendbuf�У�
			fu_ind->F=nalu.forbidden_bit;
			fu_ind->NRI=nalu.nal_reference_idc>>5;
			fu_ind->TYPE=28;

			//����FU HEADER,�������HEADER����sendbuf[13]
			fu_hdr =(FU_HEADER*)&sendbuf[13];
			fu_hdr->S=0;
			fu_hdr->E=0;
			fu_hdr->R=0;
			fu_hdr->TYPE=nalu.nal_unit_type;

			nalu_payload=&sendbuf[14];//ͬ��sendbuf[14]�ĵ�ַ����nalu_payload
			memcpy(nalu_payload,nalu.buf+(packetIndex-1)*UDP_MAX_SIZE+1,UDP_MAX_SIZE);//ȥ����ʼǰ׺��naluʣ������д��sendbuf[14]��ʼ���ַ�����
			bytes=UDP_MAX_SIZE+14;//���sendbuf�ĳ���,Ϊnalu�ĳ��ȣ���ȥԭNALUͷ������rtp_header��fu_ind��fu_hdr�Ĺ̶�����14�ֽ�
			LOGI(LOG_TAG,"socket send RTP packet seqnum:%d timeStamp: %d",seqNum,timeStamp);
			sendRTPPacket(sendbuf,bytes);//send( socket1, sendbuf, bytes, 0 );//����rtp��				
		}

		//�������һ����FU��S=0��E=1��R=0
	
		rtp_hdr->seq_no = htons(seqNum ++);
		LOGI(LOG_TAG,"ts %d",seqNum);
		//����rtp M λ����ǰ����������һ����Ƭʱ��λ��1		
		rtp_hdr->marker=1;
		//����FU INDICATOR,�������HEADER����sendbuf[12]
		fu_ind =(FU_INDICATOR*)&sendbuf[12]; //��sendbuf[12]�ĵ�ַ����fu_ind��֮���fu_ind��д��ͽ�д��sendbuf�У�
		fu_ind->F=nalu.forbidden_bit;
		fu_ind->NRI=nalu.nal_reference_idc>>5;
		fu_ind->TYPE=28;

		//����FU HEADER,�������HEADER����sendbuf[13]
		fu_hdr =(FU_HEADER*)&sendbuf[13];
		fu_hdr->S=0;
		fu_hdr->E=1;
		fu_hdr->R=0;
		fu_hdr->TYPE=nalu.nal_unit_type;

		nalu_payload=&sendbuf[14];//ͬ��sendbuf[14]�ĵ�ַ����nalu_payload
		memcpy(nalu_payload,nalu.buf+(packetIndex-1)*UDP_MAX_SIZE+1,lastPackSize-1);//��nalu���ʣ���-1(ȥ����һ���ֽڵ�NALUͷ)�ֽ�����д��sendbuf[14]��ʼ���ַ�����
		bytes=lastPackSize-1+14;//���sendbuf�ĳ���,Ϊʣ��nalu�ĳ���l-1����rtp_header��FU_INDICATOR,FU_HEADER������ͷ��14�ֽ�
		LOGI(LOG_TAG,"socket send RTP packet seqnum:%d timeStamp: %d",seqNum,timeStamp);
		sendRTPPacket(sendbuf,bytes);//send( socket1, sendbuf, bytes, 0 );//����rtp��		
	}
	return 0;


}

void ARTPConnection::sendRTPPacket(const uint8_t* buf ,size_t bytes)
{
	Mutex::Autolock autoLock(mStreamLock);
    list<StreamInfo>::iterator it = mStreams.begin();
    while (it != mStreams.end()) 
		{
			ssize_t n = sendto(
					it->mRTPSocket, buf, bytes, 0,
					(const struct sockaddr *)&it->mRemoteRTPAddr, sizeof(it->mRemoteRTPAddr));	
			//CHECK_EQ(n, (ssize_t)buffer->size());
			if(n!=bytes)LOGE(LOG_TAG,"socket send rtp error %d!",n);
	        ++it;
	    }
}



int ARTPConnection::GetAnnexbNALU (NALU_t *nalu,const sp<ABuffer> &buf)
{
	nalu->len = buf->size();
	nalu->buf = buf->data();
	nalu->forbidden_bit = nalu->buf[0] & 0x80;		//1 bit
	nalu->nal_reference_idc = nalu->buf[0] & 0x60;	//2 bit
	nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;	//5 bit
	return 0;
}

void ARTPConnection::dump(NALU_t *n)
{
	if (!n)return;
	LOGI(LOG_TAG,"nal length:%d nal_unit_type: %x\n", n->len,n->nal_unit_type);
}

bool ARTPConnection::getStatus()
{
	return mThreadRunFlag;
}


void* ARTPConnection::threadloop(void* arg)
{
	ARTPConnection* connptr = (ARTPConnection*)arg;
	sp<ABuffer> buf;
	while(connptr->mThreadRunFlag)
		{	
			if(connptr->mRTPSource->outputQPop(buf)>=0)
				{
					if(!connptr->RTPPacket(buf))
						{
						
						}
					buf->setRange(0,0);
					connptr->mRTPSource->outputQPush(buf);
					usleep(10000);
				}
			else LOGI(LOG_TAG,"RTP Source is empty");	
		}
}

