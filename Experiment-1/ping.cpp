#include "stdafx.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "WS2_32.lib")

#define RECV_BUFF_SIZE  1024

struct icmp_header {
	unsigned char type;     //��Ϣ����
	unsigned char code;     //����
	unsigned short checksum;//У���
	unsigned short id;      //Ψһ��ʶ�������id,һ����Ϊ����ID
	unsigned short sequence;//���к�
	ULONG m_ulTimeStamp;
};

struct IPHeader
{
	BYTE m_byVerHLen; //4λ�汾+4λ�ײ�����
	BYTE m_byTOS; //��������
	USHORT m_usTotalLen; //�ܳ���
	USHORT m_usID; //��ʶ
	USHORT m_usFlagFragOffset; //3λ��־+13λƬƫ��
	BYTE m_byTTL; //TTL
	BYTE m_byProtocol; //Э��
	USHORT m_usHChecksum; //�ײ������
	ULONG m_ulSrcIP; //ԴIP��ַ
	ULONG m_ulDestIP; //Ŀ��IP��ַ
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
	//��IP���ݰ���У����ֶ���Ϊ0
	long sum = 0;
	//���ײ�������16λΪ��λ��������ɣ����ν��ж�������ͣ�ע�⣺���ʱӦ�����λ�Ľ�λ���棬���Լӷ�Ӧ����32λ�ӷ���
	unsigned short *pusicmp = (unsigned short *)picmp;
	while (len > 1) {
		sum += *(pusicmp++);
		//�ӷ������в����Ľ�λ�����λ�Ľ�λ���ӵ���16λ������32λ�ӷ�ʱ����Ϊ����16λ���16λ��ӣ�֮��Ҫ�Ѹôμӷ����λ�����Ľ�λ�ӵ���16λ��
		if (sum & 0x80000000) {
			sum = (sum & 0xffff) + (sum >> 16);
		}
		len -= 2;
	}
	//�������������,��Ҫ�������1�ֽ�
	if (len) {
		sum += (unsigned short)*(unsigned char*)pusicmp;
	}
	while (sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}
	//�������ĺ�ȡ�������õ�У��͡�
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
	//����Ŀ�ĵ�ַ
	sockaddr_in dest_addr;
	dest_addr.sin_family = PF_INET;
	inet_pton(PF_INET, szDestIp, &dest_addr.sin_addr);
	dest_addr.sin_port = htons(0);

	//����ICMP���
	unsigned short sequence = 0;
	pIcmp->type = ICMP_ECHO_REQUEST;
	pIcmp->code = 0;
	//pIcmp->id = GetCurrentProcessId();
	pIcmp->id = 0x0100;
	pIcmp->checksum = 0;

	while (sequence < 4) {
		pIcmp->m_ulTimeStamp = GetTickCountCalibrate();

		pIcmp->sequence = sequence++;

		//��������,��������ݿ����������
		memcpy((szBuff + ICMP_HEADER_SIZE), "abcdefghijklmnopqrstuvwabcdefghi", 32);

		//����У���
		pIcmp->checksum = calcChecksum((struct icmp_header *)szBuff, ICMP_PACKET_SIZE);

		//���ͺͽ���
		sockaddr_in recv_addr;
		char szRecvBuff[RECV_BUFF_SIZE];
		int addrLen = sizeof(recv_addr);
		int sendLen = sendto(s, szBuff, ICMP_PACKET_SIZE, 0, (SOCKADDR *)&dest_addr, sizeof(dest_addr));
		if (sendLen == SOCKET_ERROR) {
			printf("����ʧ��(%d)\n", WSAGetLastError());
			fail++;
		}
		else {
			int recvLen = recvfrom(s, szRecvBuff, RECV_BUFF_SIZE, 0, (SOCKADDR *)&recv_addr, &addrLen);
			ULONG nRecvTimestamp = GetTickCountCalibrate();
			if (recvLen == SOCKET_ERROR) {
				printf("����ʧ��(%d)\n", WSAGetLastError());
				fail++;
			}
			else {
				IPHeader* pIPHeader = (IPHeader*)szRecvBuff;
				USHORT usIPHeaderLen = (USHORT)((pIPHeader->m_byVerHLen & 0x0f) * 4);
				icmp_header* pICMPHeader = (icmp_header*)(szRecvBuff + usIPHeaderLen);
				int m = nRecvTimestamp - pICMPHeader->m_ulTimeStamp;
				printf("���� %s �Ļظ� : �ֽ� = %d ʱ��=%d ms TTL = %d\n", szDestIp, recvLen, m, pIPHeader->m_byTTL);

				//�жϽ��յ����Ƿ����Լ�����ĵ�ַ
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
	printf("%s ��pingͳ����Ϣ��\n", szDestIp);
	printf("���ݰ����ѷ��ͣ�4���ɹ���%d����ʧ��%d��%d %%��ʧ��\n",success,fail,fail*25);
	if (success>0)
	{
		printf("�����г̹���ʱ�䣺\n");
		average = (float)sum / 4.0;
		printf("���ݰ�����̣�%d ms�����%dms��ƽ����%.0fms",least,most,average);
	}
	closesocket(s);

	WSACleanup();
}

int main(int argc ,char* argv[])
{
	if (argv[1] == NULL)
	{
		printf("��ҪIP��ַ\n");
		system("pause");
		return 0;
	}
	printf("���� Ping %s:\n",argv[1]);
	ping(argv[1]);
	system("pause");
	return 0;
}