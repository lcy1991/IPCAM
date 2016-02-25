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
struct sockaddr_in address;//��������ͨ�ŵĵ�ַ  
int rtpsock;
int rtcpsock;

bzero(&address,sizeof(address));	
address.sin_family=AF_INET;  
address.sin_addr.s_addr=inet_addr("127.0.0.1");//���ﲻһ��  
address.sin_port=htons(6789); 
RTPConn->setSource(&mysource);

//MakePortPair(&rtpsock,&rtcpsock,address);
printf("MakePortPair %d\n",MakePortPair(&rtpsock,&rtcpsock,address));
address.sin_family=AF_INET;  
address.sin_addr.s_addr=inet_addr("127.0.0.1");//���ﲻһ��  
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
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=1) return 0; //�ж��Ƿ�Ϊ0x000001,����Ƿ���1
	else return 1;
}

static int FindStartCode3 (unsigned char *Buf)
{
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=0 || Buf[3] !=1) return 0;//�ж��Ƿ�Ϊ0x00000001,����Ƿ���1
	else return 1;
}


//�����������Ϊһ��NAL�ṹ�壬��Ҫ����Ϊ�õ�һ��������NALU��������NALU_t��buf�У�
//��ȡ���ĳ��ȣ����F,IDC,TYPEλ��
//���ҷ���������ʼ�ַ�֮�������ֽ�������������ǰ׺��NALU�ĳ���
int GetAnnexbNALU (sp<ABuffer> nalu)
{
	int pos = 0;
	int StartCodeFound, rewind;
//	unsigned char *Buf;
	int info2,info3,startcodeprefix_len,len;

	static unsigned char Buf[50000];

	startcodeprefix_len=3;//��ʼ���������еĿ�ʼ�ַ�Ϊ3���ֽ�

	if (3 != fread (Buf, 1, 3, bits))//�������ж�3���ֽ�
	{
		//free(Buf);
		return 0;
	}
	info2 = FindStartCode2 (Buf);//�ж��Ƿ�Ϊ0x000001 
	if(info2 != 1) 
	{
		//������ǣ��ٶ�һ���ֽ�
		if(1 != fread(Buf+3, 1, 1, bits))//��һ���ֽ�
		{
			//free(Buf);
			return 0;
		}
		info3 = FindStartCode3 (Buf);//�ж��Ƿ�Ϊ0x00000001
		if (info3 != 1)//������ǣ�����-1
		{ 
			//free(Buf);
			return -1;
		}
		else 
		{
			//�����0x00000001,�õ���ʼǰ׺Ϊ4���ֽ�
			pos = 4;
			startcodeprefix_len = 4;
		}
	}
	else
	{
		//�����0x000001,�õ���ʼǰ׺Ϊ3���ֽ�
		startcodeprefix_len = 3;
		pos = 3;
	}
	//������һ����ʼ�ַ��ı�־λ
	StartCodeFound = 0;
	info2 = 0;
	info3 = 0;

	while (!StartCodeFound)
	{
		if (feof (bits))//�ж��Ƿ����ļ�β
		{
			//return 0;
			len = (pos-1)- startcodeprefix_len;
			memcpy (nalu->data(), &Buf[startcodeprefix_len], len);     
			//free(Buf);
			printf("lcy 1991 len %d\n",len);
			return pos-1;
		}
		Buf[pos++] = fgetc (bits);//��һ���ֽڵ�BUF��
		info3 = FindStartCode3(&Buf[pos-4]);//�ж��Ƿ�Ϊ0x00000001
		if(info3 != 1)
			info2 = FindStartCode2(&Buf[pos-3]);//�ж��Ƿ�Ϊ0x000001
		StartCodeFound = (info2 == 1 || info3 == 1);
	}

	// Here, we have found another start code (and read length of startcode bytes more than we should
	// have.  Hence, go back in the file
	rewind = (info3 == 1)? -4 : -3;

	if (0 != fseek (bits, rewind, SEEK_CUR))//���ļ�ָ��ָ��ǰһ��NALU��ĩβ
	{
		//free(Buf);
		printf("GetAnnexbNALU: Cannot fseek in the bit stream file");
	}

	// Here the Start code, the complete NALU, and the next start code is in the Buf.  
	// The size of Buf is pos, pos+rewind are the number of bytes excluding the next
	// start code, and (pos+rewind)-startcodeprefix_len is the size of the NALU excluding the start code

	len = (pos+rewind)-startcodeprefix_len;
	memcpy (nalu->data(), &Buf[startcodeprefix_len], len);//����һ������NALU����������ʼǰ׺0x000001��0x00000001
	//free(Buf);
	printf("memcpy2\n");

	return (pos+rewind);//����������ʼ�ַ�֮�������ֽ�������������ǰ׺��NALU�ĳ���
}




