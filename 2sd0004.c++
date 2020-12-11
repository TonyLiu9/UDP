//server
#define _CRT_SECURE_NO_WARNINGS
#include <Winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <bits/stdc++.h>
#pragma comment(lib,"wsock32.lib") 
#pragma warning(disable:4996)
#define file_end        "DIAMOND_DUST"
//this is the file end mark, you can enter any characters, just make sure it will not be repeatable.
#define MAX_LEN 500
//the max datagram is 500 bytes
const char* itoa2(int val)
{
    static char result[sizeof(int) << 3 + 2];
    sprintf(result, "%d", val);
    return result;
}
int main()
{
R:WORD wVersionRequested;
    WSADATA wsaData;
    FILE* recvData;
    int n, cc = 0;
    int i = 0;
    int Total_Recv = 0;
    recvData = fopen("e:\\temp.temp", "w+b");//temp file name
    int err;
    int flag = 1;
    int Checkflag = 2;
    char a;
    a = 'A';
    char* mem;
    mem = &a;   // 正确
    wVersionRequested = MAKEWORD(1, 1);   //winsock version is 1.1
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0)
    {
        return -1;
    }
    if (LOBYTE(wsaData.wVersion) != 1 ||
        HIBYTE(wsaData.wVersion) != 1)  //check the winsock version
    {
        WSACleanup();
        return -1;
    }
    SOCKET sockSrv = socket(AF_INET, SOCK_DGRAM, 0); //initialize winsock
    SOCKADDR_IN addrSrv;
    addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(1985);  //use port 1985
    bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)); //bind port
    SOCKADDR_IN addrClient;
    int len = sizeof(SOCKADDR);
    char recvBuf[500];

    static SOCKET ListenSocket;
    ListenSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);



    SOCKET sockClient = socket(AF_INET, SOCK_DGRAM, 0); //initialize winsock
    SOCKADDR_IN addrSrv1;
    addrSrv1.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");  //for test ,I use 127.0.0.1 as the server address.
    addrSrv1.sin_family = AF_INET;
    addrSrv1.sin_port = htons(1986);
    while (1)
    {


        if ((n = recvfrom(sockSrv, recvBuf, MAX_LEN, 0, (SOCKADDR*)&addrClient, &len)) < 0)
        {
            std::cout << stderr << "Can't Receive datagram" << std::endl;
            throw - 1;
        }


        // std::cout<<recvBuf;
        if ((flag == 1) && ((n == 14) && (strncmp(recvBuf, "Here is a file", n) == 0)))//if flag=1, it means the server agree to recive file
        {
            if (sendto(sockClient, "Please send the filename", 25, 0, (SOCKADDR*)&addrSrv1, sizeof(SOCKADDR)) == -1) //tell to the client to continue
            {
                std::cout << stderr << "Can't send datagram.." << std::endl;
                throw - 1;
            }

            memset(recvBuf, 0, 500);  //clear the char array,ready for reciving the file name
            while (1)
            {
                if ((n = recvfrom(sockSrv, recvBuf, MAX_LEN, 0, (SOCKADDR*)&addrClient, &len)) < 0)
                {
                    std::cout << stderr << "Can't Receive datagram" << std::endl;
                    throw - 1;
                }
                if ((fopen(recvBuf, "w+b")) == NULL)
                {
                    std::cout << "Cannot   open   the source  file!" << std::endl;
                    std::cout << "Temp file name will be used." << std::endl;
                }
                else
                {

                    recvData = fopen(recvBuf, "w+b");
                }
                break;

            }


            flag = 0;//when in transporting set the flag=0 to stop another transport require
            continue;
        }


        if ((n == 12) && (strncmp(recvBuf, file_end, n) == 0))// if file end is recived, finish the transporting and set flag=1
        {
            if (sendto(sockClient, itoa2(Total_Recv), MAX_LEN, 0, (SOCKADDR*)&addrSrv1, sizeof(SOCKADDR)) == -1)
            {
                std::cout << stderr << "sending check error!" << std::endl;
                throw - 1;
            }
            else
            {
                std::cout << "Checking!" << std::endl;
            }
            if ((n = recvfrom(sockSrv, recvBuf, MAX_LEN, 0, (SOCKADDR*)&addrClient, &len)) < 0)
            {
                std::cout << stderr << "Can't Receive datagram" << std::endl;
                throw - 1;
            }
            if (strncmp("RECENDINGALL", recvBuf, 12) == 0)
            {
                goto R;
            }
            if ((n == 1) && (strncmp(recvBuf, "1", n) != 0))
                goto R;
            else
                std::cout << "Finished!!! Total Receive : " << Total_Recv << " byte" << std::endl;
            flag = 1;
            Checkflag = 0;
            memset(recvBuf, 0, 500);
            fclose(recvData);
            Total_Recv = 2;
            continue;
        }
        else
        {
            Total_Recv += n;
            Checkflag++;
            cc = 1;
            if ((fwrite(recvBuf, n, 1, recvData)) <= 0)
            {
                fclose(recvData);
                throw - 1;
            }
            if ((n = recvfrom(sockSrv, recvBuf, MAX_LEN, 0, (SOCKADDR*)&addrClient, &len)) < 0)
            {
                std::cout << stderr << "Can't Receive datagram" << std::endl;
                throw - 1;
            }
            if (strncmp(itoa2(Checkflag), recvBuf, strlen(itoa2(Checkflag))) == 0 || strncmp(itoa2(Checkflag - 1), recvBuf, strlen(itoa2(Checkflag - 1))) == 0)
            {
                if (sendto(sockClient, recvBuf, MAX_LEN, 0, (SOCKADDR*)&addrSrv1, sizeof(SOCKADDR)) == -1)
                {
                    std::cout << stderr << "sending check error!" << std::endl;
                    throw - 1;
                }
            }
            else
            {
                if (strncmp("RECENDINGALL", recvBuf, 12) == 0)
                {
                    goto R;
                }
                if (sendto(sockClient, itoa2(Checkflag), MAX_LEN, 0, (SOCKADDR*)&addrSrv1, sizeof(SOCKADDR)) == -1)
                {
                    std::cout << stderr << "sending check error!" << std::endl;
                    throw - 1;
                }
            }
            /*
            if (sendto(sockClient, itoa2(), MAX_LEN, 0, (SOCKADDR*)&addrSrv1, sizeof(SOCKADDR)) == -1) //tell to the client to continue
            {
                std::cout << stderr << "Can't send datagram.." << std::endl;
                throw - 1;
            }*/
        }

    }
    closesocket(sockSrv);
    WSACleanup();
    return 0;
}