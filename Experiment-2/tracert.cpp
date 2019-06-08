//TraceRoute3.cpp
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <IPHlpApi.h>
//增加静态链接库ws2_32.lib
#pragma comment(lib,"ws2_32.lib")
//声明3个函数类型的指针
typedef HANDLE(WINAPI *lpIcmpCreateFile)(VOID);
typedef BOOL(WINAPI *lpIcmpCloseHandle)(HANDLE  IcmpHandle);
typedef DWORD(WINAPI *lpIcmpSendEcho)(
	HANDLE                   IcmpHandle,
	IPAddr                   DestinationAddress,
	LPVOID                   RequestData,
	WORD                     RequestSize,
	PIP_OPTION_INFORMATION   RequestOptions,
	LPVOID                   ReplyBuffer,
	DWORD                    ReplySize,
	DWORD                    Timeout
	);
int main(int argc, char* argv[]) {
	if (argc != 2) {
		printf("Usage: %s destIP\n", argv[0]);
		exit(-1);
	}
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("WSAStartup failed.\n");
		exit(-1);
	}
	//转换IP地址到整数
	unsigned long ip = inet_addr(argv[1]);
	if (ip == INADDR_NONE) {
		//用户可能输入的是域名
		hostent* pHost = gethostbyname(argv[1]);
		//如果域名无法解析
		if (pHost == NULL) {
			printf("Invalid IP or domain name: %s\n", argv[1]);
			exit(-1);
		}
		//取域名的第一个IP地址
		ip = *(unsigned long*)pHost->h_addr_list[0];
		printf("trace route to %s(%s)\n\n", argv[1], inet_ntoa(*(in_addr*)&ip));
	}
	else {
		printf("trace route to %s\n\n", argv[1]);
	}
	//载入ICMP.DLL动态库
	HMODULE hIcmpDll = LoadLibrary("icmp.dll");
	if (hIcmpDll == NULL) {
		printf("fail to load icmp.dll\n");
		exit(-1);
	}
	//定义3个函数指针
	lpIcmpCreateFile IcmpCreateFile;
	lpIcmpCloseHandle IcmpCloseHandle;
	lpIcmpSendEcho IcmpSendEcho;
	//从ICMP.DLL中获取所需的函数入口地址
	IcmpCreateFile = (lpIcmpCreateFile)GetProcAddress(hIcmpDll, "IcmpCreateFile");
	IcmpCloseHandle = (lpIcmpCloseHandle)GetProcAddress(hIcmpDll, "IcmpCloseHandle");
	IcmpSendEcho = (lpIcmpSendEcho)GetProcAddress(hIcmpDll, "IcmpSendEcho");
	//打开ICMP句柄
	HANDLE hIcmp;
	if ((hIcmp = IcmpCreateFile()) == INVALID_HANDLE_VALUE) {
		printf("/tUnable to open ICMP file.\n");
		exit(-1);
	}
	//设置IP报头的TTL值
	IP_OPTION_INFORMATION IpOption;
	ZeroMemory(&IpOption, sizeof(IP_OPTION_INFORMATION));
	IpOption.Ttl = 1;
	//设置要发送的数据
	char SendData[32];
	memset(SendData, '0', sizeof(SendData));
	//设置接收缓冲区
	char ReplyBuffer[sizeof(ICMP_ECHO_REPLY) + 32];
	PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
	BOOL bLoop = TRUE;
	int iMaxHop = 30;
	bool traced = false;
	while (bLoop && iMaxHop--) {
		printf("%2d: ", IpOption.Ttl);
		//发送ICMP回显请求
		for (int i = 0; i < 3; i++) {
			if (IcmpSendEcho(hIcmp, (IPAddr)ip, SendData, sizeof(SendData), &IpOption,
				ReplyBuffer, sizeof(ReplyBuffer), 3000) != 0) {
				if (pEchoReply->RoundTripTime == 0) {
					printf("\t<1ms");
				}
				else {
					printf("\t%dms", pEchoReply->RoundTripTime);
				}
				traced = true;
				//判断是否完成路由路径探测
				if ((unsigned long)pEchoReply->Address == ip) {
					bLoop = FALSE;
				}
			}
			else {
				printf("\t*");
			}
		}	
		if (traced == true) {
			printf("\t%s\n", inet_ntoa(*(in_addr*)&(pEchoReply->Address)));
			traced = false;
		}
		else {
			printf("\tRequest time out.\n");
		}
		if (bLoop == FALSE) {
			printf("\nTrace complete.\n");
		}
		IpOption.Ttl++;
	}
	IcmpCloseHandle(hIcmp);
	FreeLibrary(hIcmpDll);
	WSACleanup();
	return 0;
}