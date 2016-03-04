#include "handler_1.h"
#include "foundation/AString.h"
#include "foundation/ALooper.h"
#include <unistd.h>
#include "foundation/ADebug.h"
#include "rtsp/MyRTSPHandler.h"
#include "rtsp/ARTPSource.h"
#include "rtsp/ARTPConnection.h"


ARTPSource mysource(5,50000);
int GetAnnexbNALU (sp<ABuffer> nalu);
void* start__(void*);

void* sendbuf(void* agr)
{
	int i;
	sp<ABuffer> buf;
	char neirong[20];
	for(i=0;i<100;i++)
		{
			sprintf(neirong,"neirong %d",i);
			if(mysource.inputQPop(buf)>=0)
				{
					memcpy(buf->data(),neirong,strlen(neirong));
					buf->setRange(0,strlen(neirong));
					mysource.inputQPush(buf);				
				}
			else i--;
		}
}

void* getbuf(void* arg)
{
	int i;
	sp<ABuffer> buf;
	char neirong[20];
	for(i=0;i<200;i++)
		{
			//sprintf(neirong,"neirong %d",i);
			if(mysource.outputQPop(buf)>=0)
				{			
					memcpy(neirong,buf->data(),buf->size());
					printf("%s\n",neirong);
					buf->setRange(0,0);
					mysource.outputQPush(buf);
				}
		}	
}
FILE* bits;
int main()
{
	bits = fopen("test.264","r");
	pthread_t p_t;
	int ret;
	void* status;
	ret=pthread_create(&p_t,NULL,start__,NULL);
	pthread_join(p_t,&status);
	return 0;
}


void* start__(void*){

//*********************************
	FILE* bits;
	bits = fopen("test.264","r");
	if(bits<0)
		LOGE(LOG_TAG,"open file fail");

//*********************************
	int localsocket, len;
	int err;
	char localIP[] = "127.0.0.1";//"192.168.1.101";
	ALooper looper1;
	MyRTSPHandler handler_rtsp;
	ARTPConnection handler_rtp;

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
	int ret;
	uint32_t Len;
	int i=0;
	uint8_t LenBuf[4];
	uint8_t Buf[100];
	char Signal;
	bool pre_status;
	bool cur_status;
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
						if(mysource.inputQPop(tmpbuf)>=0)
							{
								LOGI(LOG_TAG,"SEND SPS");
								memcpy((char*)tmpbuf->data(),coolpad720x480spset,sizeof(coolpad720x480spset));
								tmpbuf->setRange(0,sizeof(coolpad720x480spset));
								mysource.inputQPush(tmpbuf);
							}
						if(mysource.inputQPop(tmpbuf)>=0)
							{
								LOGI(LOG_TAG,"SEND PPS");
								memcpy((char*)tmpbuf->data(),coolpad720x480ppset,sizeof(coolpad720x480ppset));
								tmpbuf->setRange(0,sizeof(coolpad720x480ppset));
								mysource.inputQPush(tmpbuf);
							}
					}
				if(mysource.inputQPop(tmpbuf)>=0)
					{
						//ret = recv(localsocket,LenBuf,4,MSG_WAITALL);
						ret = fread(LenBuf,sizeof(char),4,bits);
						if(ret < 0)
							{
								LOGE(LOG_TAG,"%s",strerror(errno));
								tmpbuf->setRange(0,0);
							}
						else
							{
								Len = LenBuf[0]<<24 | LenBuf[1]<<16 | LenBuf[2]<<8 | LenBuf[3];
								LOGI(LOG_TAG,"recv frame 0x%x 0x%x 0x%x 0x%x bytes %d",LenBuf[0],LenBuf[1],LenBuf[2],LenBuf[3],Len) ;
								LOGI(LOG_TAG,"get len %d",Len);									
								ret = fread((char*)tmpbuf->data(),sizeof(char),Len,bits);
								if(ret >= 0)
									{
										tmpbuf->setRange(0,ret);
										LOGI(LOG_TAG,"get a NALU length:%d NUM:%d\n",ret,i++);
									}
								else tmpbuf->setRange(0,0);
							}
						mysource.inputQPush(tmpbuf);
					}
			}
		else
			{
				if(pre_status==true)
					{
						if(!fseek(bits,0,SEEK_SET))
							LOGI(LOG_TAG,"Reset the file pointer");
					}
				usleep(10000);
			}
	}

	return 0;
}

