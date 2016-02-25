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
	bits = fopen("vivotest.264","r");

#if 1
	ALooper looper1;
	MyRTSPHandler handler_rtsp;
	ARTPConnection handler_rtp;
	handler_rtp.setSource(&mysource);
	handler_rtsp.setRTPConnection(&handler_rtp);
	looper1.registerHandler(&handler_rtsp);
	looper1.registerHandler(&handler_rtp);
	looper1.start();
	handler_rtsp.StartServer();
#endif
#if 0
pthread_t idsend;
pthread_t idget;

int i,ret;b


ret=pthread_create(&idsend,NULL,sendbuf,NULL);

ret=pthread_create(&idget,NULL,getbuf,NULL);

//sleep(1);
void* status;
pthread_join(idsend,&status);
pthread_join(idget,&status);

#endif

#if 0
ARTPConnection* RTPConn = new ARTPConnection();
ALooper* looper1 =	new ALooper;
looper1->registerHandler(RTPConn);
looper1->start();
struct sockaddr_in address;//处理网络通信的地址  
int rtpsock;
int rtcpsock;

bzero(&address,sizeof(address));	
address.sin_family=AF_INET;  
address.sin_addr.s_addr=inet_addr("127.0.0.1");//这里不一样  
address.sin_port=htons(6789); 
RTPConn->setSource(&mysource);

//MakePortPair(&rtpsock,&rtcpsock,address);
printf("MakePortPair %d\n",MakePortPair(&rtpsock,&rtcpsock,address));
address.sin_family=AF_INET;  
address.sin_addr.s_addr=inet_addr("127.0.0.1");//这里不一样  
address.sin_port=htons(6789); 

RTPConn->addStream(rtpsock,rtcpsock,0,&address);
#endif

sp<ABuffer> tmpbuf;
int Len;
int i=0;
while(!handler_rtp.getStatus())
{
	usleep(2000);
}
while(feof(bits)==0)
{
	if(mysource.inputQPop(tmpbuf)>=0)
		{
			Len = GetAnnexbNALU(tmpbuf);
			tmpbuf->setRange(0,Len);
			LOGI(LOG_TAG,"get a NALU length:%d NUM:%d\n",Len,i++);
			mysource.inputQPush(tmpbuf);
		}
}

sleep(10);

LOGI(LOG_TAG,"END");



	return 0;

}

static int FindStartCode2 (unsigned char *Buf)
{
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=1) return 0; //判断是否为0x000001,如果是返回1
	else return 1;
}

static int FindStartCode3 (unsigned char *Buf)
{
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=0 || Buf[3] !=1) return 0;//判断是否为0x00000001,如果是返回1
	else return 1;
}


//这个函数输入为一个NAL结构体，主要功能为得到一个完整的NALU并保存在NALU_t的buf中，
//获取他的长度，填充F,IDC,TYPE位。
//并且返回两个开始字符之间间隔的字节数，即包含有前缀的NALU的长度
int GetAnnexbNALU (sp<ABuffer> nalu)
{
	int pos = 0;
	int StartCodeFound, rewind;
//	unsigned char *Buf;
	int info2,info3,startcodeprefix_len,len;

	static unsigned char Buf[50000];

	startcodeprefix_len=3;//初始化码流序列的开始字符为3个字节

	if (3 != fread (Buf, 1, 3, bits))//从码流中读3个字节
	{
		//free(Buf);
		return 0;
	}
	info2 = FindStartCode2 (Buf);//判断是否为0x000001 
	if(info2 != 1) 
	{
		//如果不是，再读一个字节
		if(1 != fread(Buf+3, 1, 1, bits))//读一个字节
		{
			//free(Buf);
			return 0;
		}
		info3 = FindStartCode3 (Buf);//判断是否为0x00000001
		if (info3 != 1)//如果不是，返回-1
		{ 
			//free(Buf);
			return -1;
		}
		else 
		{
			//如果是0x00000001,得到开始前缀为4个字节
			pos = 4;
			startcodeprefix_len = 4;
		}
	}
	else
	{
		//如果是0x000001,得到开始前缀为3个字节
		startcodeprefix_len = 3;
		pos = 3;
	}
	//查找下一个开始字符的标志位
	StartCodeFound = 0;
	info2 = 0;
	info3 = 0;

	while (!StartCodeFound)
	{
		if (feof (bits))//判断是否到了文件尾
		{
			//return 0;
			len = (pos-1)- startcodeprefix_len;
			memcpy (nalu->data(), &Buf[startcodeprefix_len], len);     
			//free(Buf);
			printf("lcy 1991 len %d\n",len);
			return pos-1;
		}
		Buf[pos++] = fgetc (bits);//读一个字节到BUF中
		info3 = FindStartCode3(&Buf[pos-4]);//判断是否为0x00000001
		if(info3 != 1)
			info2 = FindStartCode2(&Buf[pos-3]);//判断是否为0x000001
		StartCodeFound = (info2 == 1 || info3 == 1);
	}

	// Here, we have found another start code (and read length of startcode bytes more than we should
	// have.  Hence, go back in the file
	rewind = (info3 == 1)? -4 : -3;

	if (0 != fseek (bits, rewind, SEEK_CUR))//把文件指针指向前一个NALU的末尾
	{
		//free(Buf);
		printf("GetAnnexbNALU: Cannot fseek in the bit stream file");
	}

	// Here the Start code, the complete NALU, and the next start code is in the Buf.  
	// The size of Buf is pos, pos+rewind are the number of bytes excluding the next
	// start code, and (pos+rewind)-startcodeprefix_len is the size of the NALU excluding the start code

	len = (pos+rewind)-startcodeprefix_len;
	memcpy (nalu->data(), &Buf[startcodeprefix_len], len);//拷贝一个完整NALU，不拷贝起始前缀0x000001或0x00000001
	//free(Buf);
	printf("memcpy2\n");

	return (pos+rewind);//返回两个开始字符之间间隔的字节数，即包含有前缀的NALU的长度
}




