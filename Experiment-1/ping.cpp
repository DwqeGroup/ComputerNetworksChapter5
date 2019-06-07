#include "stdafx.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "WS2_32.lib")

#define RECV_BUFF_SIZE  1024

struct icmp_header {
	unsigned char type;     //消息类型
	unsigned char code;     //代码
	unsigned short checksum;//校验和
	unsigned short id;      //唯一标识此请求的id,一般设为进程ID
	unsigned short sequence;//序列号
	ULONG m_ulTimeStamp;
};

struct IPHeader
{
	BYTE m_byVerHLen; //4位版本+4位首部长度
	BYTE m_byTOS; //服务类型
	USHORT m_usTotalLen; //总长度
	USHORT m_usID; //标识
	USHORT m_usFlagFragOffset; //3位标志+13位片偏移
	BYTE m_byTTL; //TTL
	BYTE m_byProtocol; //协议
	USHORT m_usHChecksum; //首部检验和
	ULONG m_ulSrcIP; //源IP地址
	ULONG m_ulDestIP; //目的IP地址
};

#define ICMP_HEADER_SIZE    sizeof(icmp_header)
#define ICMP_PACKET_SIZE    (ICMP_HEADER_SIZE + 32)

#define ICMP_ECHO_REQUEST 0x08
#define ICMP_ECHO_REPLY   0x00

ULONG GetTickCountCalibrate()
{
	static ULONG s_ulFirstCallTick = 0;
	static LONGLONG s_ullFirstCallTickMS = 0;

	SYSTEMTIME systemtime;
	FILETIME filetime;
	GetLocalTime(&systemtime);
	SystemTimeToFileTime(&systemtime, &filetime);
	LARGE_INTEGER liCurrentTime;
	liCurrentTime.HighPart = filetime.dwHighDateTime;
	liCurrentTime.LowPart = filetime.dwLowDateTime;
	LONGLONG llCurrentTimeMS = liCurrentTime.QuadPart / 10000;

	if (s_ulFirstCallTick == 0)
	{
		s_ulFirstCallTick = GetTickCount64();
	}
	if (s_ullFirstCallTickMS == 0)
	{
		s_ullFirstCallTickMS = llCurrentTimeMS;
	}

	return s_ulFirstCallTick + (ULONG)(llCurrentTimeMS - s_ullFirstCallTickMS);
}

unsigned short calcChecksum(struct icmp_header *picmp, int len) {
	picmp->checksum = 0;
	//把IP数据包的校验和字段置为0
	long sum = 0;
	//把首部看成以16位为单位的数字组成，依次进行二进制求和（注意：求和时应将最高位的进位保存，所以加法应采用32位加法）
	unsigned short *pusicmp = (unsigned short *)picmp;
	while (len > 1) {
		sum += *(pusicmp++);
		//加法过程中产生的进位（最高位的进位）加到低16位（采用32位加法时，即为将高16位与低16位相加，之后还要把该次加法最高位产生的进位加到低16位）
		if (sum & 0x80000000) {
			sum = (sum & 0xffff) + (sum >> 16);
		}
		len -= 2;
	}
	//如果长度是奇数,还要加上最后1字节
	if (len) {
		sum += (unsigned short)*(unsigned char*)pusicmp;
	}
	while (sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}
	//将上述的和取反，即得到校验和。
	return (unsigned short)~sum;
}

void ping(const char *szDestIp) {
	BOOL ret;
	int success=0,fail=0,least=10000,most=-1,sum=0;
	float average=0;
	char szBuff[ICMP_PACKET_SIZE] = { 0 };
	struct icmp_header *pIcmp = (struct icmp_header *)szBuff;

	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 2);
	ret = WSAStartup(wVersionRequested, &wsaData);
	if (ret != 0) {
		printf("WSAStartup() called failed!\n");
		return;
	}
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		WSACleanup();
		return;
	}

	SOCKET s = WSASocket(PF_INET, SOCK_RAW, IPPROTO_ICMP, NULL, 0, WSA_FLAG_OVERLAPPED);

	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	int setResult;
	setResult = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
	if (setResult == SOCKET_ERROR) {
		printf("setsockopt SO_RCVTIMEO fail(%d)\n", WSAGetLastError());
	}
	int ttl = 64;
	setResult = setsockopt(s, IPPROTO_IP, IP_TTL, (const char*)&ttl, sizeof(ttl));
	if (setResult == SOCKET_ERROR) {
		printf("setsockopt IP_TTL fail(%d)\n", WSAGetLastError());
	}
	//设置目的地址
	sockaddr_in dest_addr;
	dest_addr.sin_family = PF_INET;
	inet_pton(PF_INET, szDestIp, &dest_addr.sin_addr);
	dest_addr.sin_port = htons(0);

	//构造ICMP封包
	unsigned short sequence = 0;
	pIcmp->type = ICMP_ECHO_REQUEST;
	pIcmp->code = 0;
	//pIcmp->id = GetCurrentProcessId();
	pIcmp->id = 0x0100;
	pIcmp->checksum = 0;

	while (sequence < 4) {
		pIcmp->m_ulTimeStamp = GetTickCountCalibrate();

		pIcmp->sequence = sequence++;

		//拷贝数据,这里的数据可以是任意的
		memcpy((szBuff + ICMP_HEADER_SIZE), "abcdefghijklmnopqrstuvwabcdefghi", 32);

		//计算校验和
		pIcmp->checksum = calcChecksum((struct icmp_header *)szBuff, ICMP_PACKET_SIZE);

		//发送和接收
		sockaddr_in recv_addr;
		char szRecvBuff[RECV_BUFF_SIZE];
		int addrLen = sizeof(recv_addr);
		int sendLen = sendto(s, szBuff, ICMP_PACKET_SIZE, 0, (SOCKADDR *)&dest_addr, sizeof(dest_addr));
		if (sendLen == SOCKET_ERROR) {
			printf("发送失败(%d)\n", WSAGetLastError());
			fail++;
		}
		else {
			int recvLen = recvfrom(s, szRecvBuff, RECV_BUFF_SIZE, 0, (SOCKADDR *)&recv_addr, &addrLen);
			ULONG nRecvTimestamp = GetTickCountCalibrate();
			if (recvLen == SOCKET_ERROR) {
				printf("接收失败(%d)\n", WSAGetLastError());
				fail++;
			}
			else {
				IPHeader* pIPHeader = (IPHeader*)szRecvBuff;
				USHORT usIPHeaderLen = (USHORT)((pIPHeader->m_byVerHLen & 0x0f) * 4);
				icmp_header* pICMPHeader = (icmp_header*)(szRecvBuff + usIPHeaderLen);
				int m = nRecvTimestamp - pICMPHeader->m_ulTimeStamp;
				printf("来自 %s 的回复 : 字节 = %d 时间=%d ms TTL = %d\n", szDestIp, recvLen, m, pIPHeader->m_byTTL);

				//判断接收到的是否是自己请求的地址
				char szRecvIp[20];
				inet_ntop(PF_INET, &recv_addr.sin_addr, szRecvIp, 20);
				if (strcmp(szRecvIp, szDestIp)==0) {
					ret = TRUE;
					success++;
					if (least > m)
						least = m;
					if(most<m)
					{
						most = m;
					}
					sum += m;
				}
				else {
					ret = FALSE;
					fail++;
				}
			}
		}
		Sleep(500);
	}
	printf("%s 的ping统计信息：\n", szDestIp);
	printf("数据包：已发送：4，成功：%d，丢失：%d（%d %%丢失）\n",success,fail,fail*25);
	if (success>0)
	{
		printf("往返行程估计时间：\n");
		average = (float)sum / 4.0;
		printf("数据包：最短：%d ms，最长：%dms，平均：%.0fms",least,most,average);
	}
	closesocket(s);

	WSACleanup();
}

int main(int argc ,char* argv[])
{
	if (argv[1] == NULL)
	{
		printf("需要IP地址\n");
		system("pause");
		return 0;
	}
	printf("正在 Ping %s:\n",argv[1]);
	ping(argv[1]);
	system("pause");
	return 0;
}