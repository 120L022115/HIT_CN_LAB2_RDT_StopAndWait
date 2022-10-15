#include <stdio.h>
#include <string.h>
#include <Windows.h>
#include <process.h>
#include <Winsock.h>
#include <stdlib.h>
#include <conio.h>
#include <time.h>
#pragma comment(lib, "ws2_32.lib")
#define MAXSIZE 1024 //发送数据报文的最大长度
#define PORT 10241 //必须是服务器PORT+1
#define TIME_OUT 20*1000 //20s
#define DST_IP "127.0.0.1"
#define TRANS_FILE "../save.txt"
#define TITLE "客户端"


struct Job {
	SOCKET socket;
	unsigned long srcIp;
	unsigned long dstIp;
	unsigned short srcPort;
	unsigned short dstPort;
} job;

FILE* file;
int proexit(int err) {

	if (file != NULL) fclose(file);
	if (job.socket != NULL) closesocket(job.socket);
	WSACleanup();
	exit(err);
}

const char* getWSAErrorText() {
	switch (WSAGetLastError())
	{
	case 0: return "Directly send error";
	case 10004: return "Interrupted function";//call 操作被终止
	case 10013: return "Permission denied";//c访问被拒绝
	case 10014: return "Bad address"; //c地址错误
	case 10022: return "Invalid argument"; //参数错误
	case 10024: return "Too many open files";// 打开太多的sockets
	case 10035: return "Resource temporarily unavailable"; // 没有可以获取的资料
	case 10036: return "Operation now in progress"; // 一个阻塞操作正在进行中
	case 10037: return "Operation already in progress";// 操作正在进行中
	case 10038: return "Socket operation on non-socket";//非法的socket对象在操作
	case 10039: return "Destination address required"; //目标地址错误
	case 10040: return "Message too long";//数据太长
	case 10041: return "Protocol wrong type for socket"; //协议类型错误
	case 10042: return "Bad protocol option";// 错误的协议选项
	case 10043: return "Protocol not supported"; //协议不被支持
	case 10044: return "Socket type not supported"; //socket类型不支持
	case 10045: return "Operation not supported"; //不支持该操作
	case 10046: return "Protocol family not supported";//协议族不支持
	case 10047: return "Address family not supported by protocol family";//使用的地址族不在支持之列
	case 10048: return "Address already in use"; //地址已经被使用
	case 10049: return "Cannot assign requested address";//地址设置失败
	case 10050: return "Network is down";//网络关闭
	case 10051: return "Network is unreachable"; //网络不可达
	case 10052: return "Network dropped connection on reset";//网络被重置
	case 10053: return "Software caused connection abort";//软件导致连接退出
	case 10054: return "connection reset by peer"; //连接被重置
	case 10055: return "No buffer space available"; //缓冲区不足
	case 10056: return "Socket is already connected";// socket已经连接
	case 10057: return "Socket is not connected";//socket没有连接
	case 10058: return "Cannot send after socket shutdown";//socket已经关闭
	case 10060: return "Connection timed out"; //超时
	case 10061: return "Connection refused"; //连接被拒绝
	case 10064: return "Host is down";//主机已关闭
	case 10065: return "No route to host";// 没有可达的路由
	case 10067: return "Too many processes";//进程太多
	case 10091: return "Network subsystem is unavailable";//网络子系统不可用
	case 10092: return "WINSOCK.DLL version out of range"; //winsock.dll版本超出范围
	case 10093: return "Successful WSAStartup not yet performed"; //没有成功执行WSAStartup
	case 10094: return "Graceful shutdown in progress";//
	case 11001: return "Host not found"; //主机没有找到
	case 11002: return "Non-authoritative host not found"; // 非授权的主机没有找到
	case 11003: return "This is a non-recoverable error";//这是个无法恢复的错误
	case 11004: return "Valid name, no data record of requested type";//请求的类型的名字或数据错误

	default:
		return "未知错误";
	}
}



BOOL InitSocket()
{
	//加载套接字库（必须）
	WORD wVersionRequested;
	WSADATA wsaData;
	//套接字加载时错误提示
	int err;
	//版本 2.2
	wVersionRequested = MAKEWORD(2, 2);
	//加载 dll 文件 Scoket 库
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		//找不到 winsock.dll
		printf("加载 winsock 失败，错误代码为: %d\n", WSAGetLastError());
		return FALSE;
	}
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		printf("不能找到正确的 winsock 版本\n");
		WSACleanup();
		return FALSE;
	}
	SOCKET* s = &job.socket;
	*s = socket(AF_INET, SOCK_DGRAM, 0);
	if (INVALID_SOCKET == *s)
	{
		printf("创建套接字失败，错误代码为：%d\n", WSAGetLastError());
		return FALSE;
	}
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(job.srcPort);
	serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	if (bind(*s, (SOCKADDR*)&serverAddr, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		printf("绑定套接字失败\n");
		return FALSE;
	}
	return TRUE;
}

/*
char* unPackData(char* Buffer) {

}

char* packData(char* Buffer) {

}*/

//返回1为超时，需传入last并自动更改
int timer(time_t* lastt) {
	time_t t = time(NULL);
	if (*lastt == 0) {
		*lastt = t;
		return 0;
	}
	else {
		if (t - *lastt > TIME_OUT) {
			*lastt = 0;
			return 1;
		}
		return 0;
	}
}
struct time {
	SOCKET as;
	char* buffer;
	bool* stop;
	int readSize;
	time(SOCKET s, char* b, bool* st, int reS) {
		as = s;
		buffer = b;
		stop = st;
		readSize = reS;
	}
};
typedef struct time TIME;

unsigned int __stdcall timeThread(void* s) {
	TIME* st = (TIME*)s;
	SOCKET as = st->as;
	char* buffer = st->buffer;
	bool* stop = st->stop;
	time_t t = 0;
	timer(&t);
	printf("启动定时器：%lld\n", t);
	while (!*stop) {
		Sleep(200);
		if (timer(&t)) {
			//超时了，重传
			printf("定时器超时，重发数据\n");
			int sendSize = send(as, buffer, st->readSize, 0);
			if (sendSize <= 0) {
				printf("发送失败，错误代码为: %s\n", getWSAErrorText());
				proexit(1);
			}
			timer(&t);
		}
	}
	delete[]buffer;
	delete stop;
	delete st;
	return 0;//_endthreadex(0);
}

void transfer(FILE* f) {
	sockaddr_in serverAddr;
	serverAddr.sin_addr.S_un.S_addr = inet_addr(DST_IP);
	serverAddr.sin_port = htons((short)PORT - 1);
	serverAddr.sin_family = AF_INET;
	printf("正在连接服务器 %s\n",DST_IP);
	if (connect(job.socket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		printf("连接服务器失败");
		printf("：%s\n", getWSAErrorText());
		proexit(1);
	}

	printf("连接成功：%s:%d\n", inet_ntoa(serverAddr.sin_addr), ntohs(serverAddr.sin_port));
	job.dstPort = ntohs(serverAddr.sin_port);
	job.dstIp = inet_addr(inet_ntoa(serverAddr.sin_addr));
	
	SOCKET as = job.socket;
	char sbuf[100] = { 0 };
	int sendSize = 0, recvSize = 0;
	sprintf_s(sbuf, 99, "GET %s %d", "127.0.0.1", PORT);
	sendSize = send(as, sbuf, 100, 0);
	if (sendSize <= 0) {
		printf("发送GET失败，错误代码为: %s\n", getWSAErrorText());
		proexit(1);
	}
	printf("成功发送GET：%s\n", sbuf);



	HANDLE hThread;
	char buffer[MAXSIZE + 1] = { 0 };
	int writeSize = 0;// = fread_s(buffer, MAXSIZE, sizeof(char), MAXSIZE, f);
	bool* stop;
	char* bufp;
	bool end = false;
	
	while (!end) {
		recvSize =recv(as, buffer, MAXSIZE, 0);
		if (recvSize <= 0) {
			printf("接收失败，错误代码为: %s\n", getWSAErrorText());
			proexit(1);
		}
		else if (!_strnicmp("END", buffer, 3)) {
			printf("成功接收END：%d\n", recvSize);
			return;
		}
		printf("成功接收数据：%d\n", recvSize);
		writeSize = fwrite(buffer, sizeof(char), recvSize, f);
		sendSize = send(as, "ACK", 3, 0);
		if (sendSize <= 0) {
			printf("发送失败，错误代码为: %s\n", getWSAErrorText());
			proexit(1);
		}
		printf("成功发送ACK：%d\n", sendSize);
		ZeroMemory(buffer, MAXSIZE);
	}
}

void init() {
	sockaddr_in adds;
	job.srcIp = inet_addr("0.0.0.0");
	job.srcPort = PORT;
	if (!InitSocket()) proexit(1);
	SetConsoleTitleA(TITLE); //设置一个新标题
}

int main() {
	printf("初始化\n");
	init();
	char cmd[100] = { 0 };
	while (true) {
		if (scanf_s("%s", cmd, 100) != 1) {
			printf("无效命令！\n");
			continue;
		}
		else if (!strcmp(cmd, "q")) {
			proexit(0);
		}
		else if (!strcmp(cmd, "g")) {
			fopen_s(&file, TRANS_FILE, "w");
			if (file == NULL) {
				printf("打开文件失败！\n");
				continue;
			}
			else {
				printf("客户端已准备好进行传输！正连接服务器！");
				transfer(file);
				fclose(file);
			}
		}
	}
	proexit(0);
}