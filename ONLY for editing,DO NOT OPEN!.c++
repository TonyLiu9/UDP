#include<stdio.h>
#include<Windows.h>
#include<stdlib.h>
#include<time.h>
#include<math.h>
//#include "winsock2.h"
#pragma     comment(lib,"WS2_32.LIB")

#define SWS 2//定义发送窗口
#define RWS 2//定义接收窗口

typedef u_char SwpSeqno;//定义序列号
typedef HANDLE Semaphore;//定义信号量
typedef HANDLE Event;//定义定时器事件
typedef char Msg;//定义消息的类型

typedef struct {
	SwpSeqno SeqNum;//帧序号
	SwpSeqno AckNum;//已收到确认帧序号
	char	 Flags;//8 bit 的标志
}SwpHdr;
struct sendQ_slot {
	Event timeout;//与发送方相关的超时事件
	Msg* msg;//发送的消息
};
struct recvQ_slot {
	int recevied; //msg是正确的吗？
	Msg* msg;
};
typedef struct {
	//发送方状态
	SwpSeqno LAR;//最近的收到的ACK序号
	SwpSeqno LFS;//最近的发送的帧序号
	Semaphore sendWindowNotFull;//信号量，控制滑动窗口的界
	SwpHdr hdr;
	struct sendQ_slot sendQ[SWS];//发送消息

	//接收方状态
	SwpSeqno NFE;//期待的下一帧的序号
	struct recvQ_slot recvQ[RWS];//接收消息
}SwpState;

//超时线程参数
typedef struct {
	int time;
	Msg frame[11];
}TimeOutType;

#define HLEN 3//帧头部长度
#define DLEN 8//帧数据长度
#define ALEN 3//ACK帧长度
#define SWP_SEND_TIMEOUT 500//定义超时长度为500ms
#define LINK "127.0.0.1"//定义要发送的对象
#define FLAG_ACK_VALID 'a'//定义ACK帧
#define FLAG_DATA_VALID 'd'//定义数据帧
#define SUCCESS 1;//定义已经成功收到的消息


static void sendSWP(SwpState* state, Msg* frame);//发送函数
static int deliverSWP(SwpState* state, Msg* frame);//接收并返回ACK函数

//发送
static void semWait(Semaphore* sendWindowNotFull);//信号量处理（-1）
static void store_swp_hdr(SwpHdr hdr, char* hbuf);//存储发送的帧头部
static void msgAddHdr(Msg* frame, char* hbuf);//将头部添加到帧上
static void msgSaveCopy(char* msg, Msg* frame);//将消息添加到帧上
static Event evSchedule(Msg* frame, int time);//调用定时器函数
DWORD WINAPI swpTimeout(LPVOID threadtype);//创建定时器线程
static void send_socket(char* addr, Msg* frame, int size);//UDP发送
static void mlisten();//监听数据
static void msend();//发送数据
//接收
static char* msgStripHdr(Msg* frame, int length);//获取帧的头部
static void load_swp_hdr(SwpHdr* hdr, char* hbuf);//帧头部提取字符串
static bool swpInWindow(SwpSeqno AckNum, SwpSeqno LAR, SwpSeqno LFS);//判断收到的帧是否在窗口内
static void evCancel(Event*);//取消超时定时器
static void msgDestroy(Msg* msg);//释放msg空间
static void semSignal(Semaphore* sendWindowNotFull);//信号量处理（+1）
static void prepare_ack(Msg* msg, SwpSeqno n);//组成ACK消息

//全局变量
HANDLE hlisten;
HANDLE hsend;
SwpState* send_state;
Msg* frame;
HANDLE Mutex = CreateMutex(NULL, FALSE, NULL);

void main() {
	//模拟滑动窗口接收和发送过程
	//初始化SwpState,Msg,数据帧组帧
	//启动两个线程，模拟收发
	//监听终止
	frame = (Msg*)malloc(11 * sizeof(Msg));
	//初始化消息
	//frame="message";//8个字节的数据
	//初始化状态
	send_state = (SwpState*)malloc(sizeof(SwpState));
	send_state->LAR = '0';
	send_state->LFS = '0';
	send_state->NFE = '0';
	send_state->sendWindowNotFull = CreateSemaphore(NULL, SWS, SWS, NULL);
	if (send_state->sendWindowNotFull == NULL) {
		printf("CreateSemaphore error\n", GetLastError());
		exit(0);
	}
	send_state->hdr.SeqNum = '0';//3个字节头部
	send_state->hdr.AckNum = '0';
	send_state->hdr.Flags = 'd';
	for (int i = 0; i < SWS; i++) {
		send_state->sendQ[i].msg = (Msg*)malloc(8 * sizeof(Msg));
		send_state->sendQ[i].msg = (Msg*)"message";
		send_state->sendQ[i].timeout = NULL;
	}
	for (int i = 0; i < RWS; i++) {
		send_state->recvQ[i].msg = (Msg*)malloc(8 * sizeof(Msg));
		send_state->recvQ[i].msg = (Msg*)"message";
		send_state->recvQ[i].recevied = NULL;
	}
	//初始化UDP函数
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 1), &wsaData)) {
		printf("Winsock initializing fail\n");
		WSACleanup();
		return;
	}
	//建立监听线程
	hlisten = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)mlisten, NULL, CREATE_SUSPENDED, NULL);
	ResumeThread(hlisten);
	if (hlisten == NULL) {
		printf("thread_listen create fail\n");
	}
	hsend = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)msend, NULL, CREATE_SUSPENDED, NULL);
	ResumeThread(hsend);
	if (hsend == NULL) {
		printf("thread_send create fail\n");
	}
	Sleep(10000);
	WSACleanup();
}
static void msend() {
	printf("thread_send started\n");
	while (1) {
		sendSWP(send_state, frame);
	}
	printf("thread_send quit\n");
}

static void sendSWP(SwpState* state, Msg* frame) {
	struct sendQ_slot* slot;//等待发送
	char hbuf[HLEN];//附加在帧头部的字符串

	//等待打开发送窗口
	semWait(&state->sendWindowNotFull);
	WaitForSingleObject(Mutex, INFINITE);
	state->hdr.SeqNum = state->LFS++;
	printf("send %d\n", state->hdr.SeqNum);
	slot = &(state->sendQ[state->hdr.SeqNum % SWS]);
	store_swp_hdr(state->hdr, hbuf);
	msgAddHdr(frame, hbuf);
	msgSaveCopy(slot->msg, frame);
	slot->timeout = evSchedule(frame, SWP_SEND_TIMEOUT);
	send_socket(const_cast<char*>(LINK), frame, HLEN + DLEN);
	ReleaseMutex(Mutex);
}

static int deliverSWP(SwpState* state, Msg* frame) {
	SwpHdr hdr;
	char* hbuf;
	hbuf = msgStripHdr(frame, HLEN);
	load_swp_hdr(&hdr, hbuf);
	if (hdr.Flags == FLAG_ACK_VALID) {
		//发送方收到一个ACK,处理ACK帧
		if (swpInWindow(hdr.AckNum, state->LAR, state->LFS)) {
			do {
				WaitForSingleObject(Mutex, INFINITE);
				printf("send get ack %d\n", hdr.AckNum);
				struct sendQ_slot* slot;
				slot = &state->sendQ[state->LAR++ % SWS];
				evCancel(&slot->timeout);
				//msgDestroy(slot->msg);
				semSignal(&state->sendWindowNotFull);
				ReleaseMutex(Mutex);
			} while (state->LAR == hdr.AckNum);
		}
	}
	if (hdr.Flags == FLAG_DATA_VALID) {
		//接收到数据帧，处理数据帧
		WaitForSingleObject(Mutex, INFINITE);
		struct recvQ_slot* slot;
		slot = &state->recvQ[hdr.SeqNum % RWS];
		if (!swpInWindow(hdr.SeqNum, state->NFE, state->NFE + RWS - 1)) {
			ReleaseMutex(Mutex);
			return SUCCESS;
		}
		msgSaveCopy(slot->msg, frame);
		slot->recevied = TRUE;
		if (hdr.SeqNum == state->NFE) {
			Msg* m = (Msg*)malloc(3 * sizeof(Msg));
			while (slot->recevied) {
				//deliver(HLP,&slot->msg)//传向上层
				printf("receive get data %d\n", hdr.SeqNum);
				printf("%s\n", slot->msg);
				//msgDestroy(slot->msg);
				slot->recevied = FALSE;
				slot = &state->recvQ[state->NFE++ % RWS];
			}
			prepare_ack(m, state->NFE - 1);
			send_socket(const_cast<char*>(LINK), m, ALEN);
			msgDestroy(m);
		}
		ReleaseMutex(Mutex);
	}
	return SUCCESS;
}

static void semWait(Semaphore* sendWindowNotFull) {
	DWORD	wait_for_semaphore;
	wait_for_semaphore = WaitForSingleObject(*sendWindowNotFull, -1);
}

static void store_swp_hdr(SwpHdr hdr, char* hbuf) {
	hbuf[0] = hdr.SeqNum;
	hbuf[1] = hdr.AckNum;
	hbuf[2] = hdr.Flags;
}

static void msgAddHdr(Msg* frame, char* hbuf) {
	frame[0] = hbuf[0];
	frame[1] = hbuf[1];
	frame[2] = hbuf[2];
}

static void msgSaveCopy(char* msg, Msg* frame) {
	int j = 0;
	for (int i = 3; i < 11; i++) {
		frame[i] = msg[j];
		j++;
	}
}

static Event evSchedule(Msg* frame, int time) {
	TimeOutType* timetype = (TimeOutType*)malloc(sizeof(TimeOutType));//超时线程参数
	timetype->time = time;
	for (int i = 0; i < 11; i++) {
		timetype->frame[i] = frame[i];
	}
	//创建定时器线程
	DWORD targetThreadID;
	HANDLE Timer = CreateThread(NULL, 0, swpTimeout, timetype, CREATE_SUSPENDED, NULL);
	ResumeThread(Timer);
	if (Timer == NULL) {
		printf("thread_timeout create fail\n");
	}
	return Timer;
}

DWORD WINAPI swpTimeout(LPVOID threadtype) {
	printf("thread_timeout started\n");
	TimeOutType* timetype = (TimeOutType*)threadtype;
	int time = timetype->time;
	Msg* frame;
	DWORD result = 1;
	frame = timetype->frame;
	SetTimer(NULL, 0, time, NULL);

	MSG msg;
	BOOL bRet;

	while (TRUE)
		//该循环捕捉定时器消息   
	{
		bRet = PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
		if (bRet == -1) {
			// handle the error and possibly exit
		}
		else if (bRet && msg.message == WM_TIMER) {//定时器
			//处理超时事件
			printf("send重发%d\n", frame[0]);
			send_socket(const_cast<char*>(LINK), frame, HLEN + DLEN);//超时重发
		}
		else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	printf("thread_timeout_quit");
	return result;
}

static void mlisten() {
	printf("thread_listen started\n");
	SOCKET socket1;
	struct sockaddr_in local;
	struct sockaddr_in from;
	int fromlen = sizeof(from);
	local.sin_family = AF_INET;
	local.sin_port = htons(8808); ///监听端口
	local.sin_addr.s_addr = INADDR_ANY; ///本机

	socket1 = socket(AF_INET, SOCK_DGRAM, 0);
	bind(socket1, (struct sockaddr*)&local, sizeof(local));
	while (1) {
		char buffer[11] = { 0 };
		if (recvfrom(socket1, buffer, sizeof(buffer), 0, (struct sockaddr*)&from, &fromlen) != SOCKET_ERROR) {
			deliverSWP(send_state, buffer);
		}
	}
	closesocket(socket1);
	printf("listen thread quit\n");
}

static void send_socket(char* addr, Msg* frame, int size) {
	//UDP发送函数
	SOCKET socket1;

	struct sockaddr_in server;
	int len = sizeof(server);
	server.sin_family = AF_INET;
	server.sin_port = htons(8808); ///server的监听端口
	server.sin_addr.s_addr = inet_addr(LINK); ///server的地址

	socket1 = socket(AF_INET, SOCK_DGRAM, 0);
	if (sendto(socket1, frame, sizeof(frame), 0, (struct sockaddr*)&server, len) != SOCKET_ERROR) {

	}
	closesocket(socket1);
}

static char* msgStripHdr(Msg* frame, int length) {
	char* result = (char*)malloc(sizeof(char));
	for (int i = 0; i < length; i++) {
		result[i] = frame[i];
	}
	return result;
}

static void load_swp_hdr(SwpHdr* hdr, char* hbuf) {
	hdr->SeqNum = hbuf[0];
	hdr->AckNum = hbuf[1];
	hdr->Flags = hbuf[2];
}

static bool swpInWindow(SwpSeqno seqno, SwpSeqno min, SwpSeqno max) {
	SwpSeqno pos, maxpos;
	pos = seqno - min;
	maxpos = max - min + 1;
	return pos < maxpos;
}

static void evCancel(Event* thread) {
	TerminateThread(*thread, 0);
	printf("thread_timeout quit\n");
}

static void msgDestroy(Msg* msg) {
	free(msg);
}

static void semSignal(Semaphore* sendWindowNotFull) {
	if (!ReleaseSemaphore(*sendWindowNotFull, 1, NULL)) {
		printf("ReleseSemphore error\n");
		exit(0);
	}
}

static void prepare_ack(Msg* m, SwpSeqno n) {
	//ack组帧
	m[0] = NULL;
	m[1] = n;
	m[2] = 'a';
}