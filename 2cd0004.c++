//client
#include <Winsock2.h>
#include <stdio.h>
#include <stdlib.h> 
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/stat.h>
#define MAX_LEN 500
#define file_end        "DIAMOND_DUST" 
//this is the file end mark, you can enter any characters, just make sure it will not be repeatable.
#pragma   comment(lib,   "WS2_32.LIB") 


const char* itoa2(int val)
{
    static char result[sizeof(int) << 3 + 2];
    sprintf(result, "%d", val);
    return result;
}//一个int强制转化const char*的较简洁的函数



int main()
{
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    FILE* tempfile;
    struct stat fileState;
    int data_size;
    char filename[50];
    int Send_Total = 0;
    int n = 0;
    int Checkflag = 2, flagcount = 0;
    int x = 0, y = 0;
    std::cout << "Please enter the filename you want to transport:" << std::endl;
    std::cin >> filename;
    char destinationname[50];
    std::cout << "Please enter the destination filename you want to transport:" << std::endl;
    std::cin >> destinationname;

R:if ((fopen(filename, "r")) == NULL)
{
    std::cout << "Cannot   open   the source  file!" << std::endl;
    return -1;
}

wVersionRequested = MAKEWORD(1, 1); //winsock version is 1.1

err = WSAStartup(wVersionRequested, &wsaData); //check if there is an error in WSAStartup
if (err != 0)
{
    std::cout << "WSAStartup error!";
    return -1;
}

if (LOBYTE(wsaData.wVersion) != 1 ||
    HIBYTE(wsaData.wVersion) != 1)  //check the winsock version
{
    WSACleanup();
    std::cout << "winsock version error!";
    return -1;
}






SOCKET sockClient = socket(AF_INET, SOCK_DGRAM, 0); //initialize winsock(client)
SOCKADDR_IN addrSrv1;
addrSrv1.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");  //for test ,I use 127.0.0.1 as the server address.
addrSrv1.sin_family = AF_INET;
addrSrv1.sin_port = htons(1985);  //use port 1985

tempfile = fopen(filename, "r+b");
stat(filename, &fileState);
std::cout << "size of file: " << std::endl << fileState.st_size;//print the file size



char* dest = new char[MAX_LEN];


SOCKET sockSrv = socket(AF_INET, SOCK_DGRAM, 0); //initialize winsock(server)
SOCKADDR_IN addrSrv;
addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
addrSrv.sin_family = AF_INET;
addrSrv.sin_port = htons(1986);  //use port 1986
bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)); //bind port
int len = sizeof(SOCKADDR);
SOCKADDR_IN addrClient;
char recvBuf[500];
static SOCKET ListenSocket;
ListenSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

if (sendto(sockClient, "Here is a file", 14, 0, (SOCKADDR*)&addrSrv1, sizeof(SOCKADDR)) == -1) //tell to the server I want to send a file
{
    std::cout << stderr << "sendto error" << std::endl;
    throw - 1;
}


while (1)
{
    if ((n = recvfrom(sockSrv, recvBuf, MAX_LEN, 0, (SOCKADDR*)&addrClient, &len)) < 0) //recive the data from the server
    {

        std::cout << stderr << "Can't Receive datagram" << std::endl;
        throw - 1;
    }
    // std::cout<<recvBuf;
    if (strncmp(recvBuf, "Please send the filename", n) == 0) //continue waiting untill the server agree to transport,then send the file name
    {
        if (sendto(sockClient, destinationname, strlen(destinationname), 0, (SOCKADDR*)&addrSrv1, sizeof(SOCKADDR)) == -1)
        {
            std::cout << stderr << "sendto error" << std::endl;
            throw - 1;
        }
        //  std::cout<<recvBuf;
        break;
    }


}
Sleep(1000);//wait for a second,then send the file data

CK:while ((data_size = fread(dest, 1, MAX_LEN, tempfile)) > 0) //read 500 times and each time read a char to ausure the data size
{
    Send_Total += data_size;
    //  std::cout<<dest; //print the result.
   // std::cout << data_size;
    if (sendto(sockClient, dest, data_size, 0, (SOCKADDR*)&addrSrv1, sizeof(SOCKADDR)) == -1)
    {
        std::cout << stderr << "sendto error" << std::endl;
        throw - 1;
    }
    if (sendto(sockClient, itoa2(Checkflag), MAX_LEN, 0, (SOCKADDR*)&addrSrv1, sizeof(SOCKADDR)) == -1)
    {
        std::cout << stderr << "sendto error" << std::endl;
        throw - 1;
    }
P1:std::cout << "Transmit:" << filename << "     byte:" << data_size << "    flag:" << Checkflag - 2 << std::endl;
    Checkflag++;
CO:if ((data_size = fread(dest, 1, MAX_LEN, tempfile)) > 0) //read 500 times and each time read a char to ausure the data size
{
    Send_Total += data_size;
    //  std::cout<<dest; //print the result.
   // std::cout << data_size;
RESEND:if (sendto(sockClient, dest, data_size, 0, (SOCKADDR*)&addrSrv1, sizeof(SOCKADDR)) == -1)
{
    std::cout << stderr << "sendto error" << std::endl;
    throw - 1;
}
if (sendto(sockClient, itoa2(Checkflag), MAX_LEN, 0, (SOCKADDR*)&addrSrv1, sizeof(SOCKADDR)) == -1)
{
    std::cout << stderr << "sendto error" << std::endl;
    throw - 1;
}
P2:std::cout << "Transmit:" << filename << "     byte:" << data_size << "flag:" << Checkflag - 2 << std::endl;
LISTEN:if ((n = recvfrom(sockSrv, recvBuf, MAX_LEN, 0, (SOCKADDR*)&addrClient, &len)) < 0) //recive the data from the server
{

    std::cout << stderr << "Can't Receive datagram" << std::endl;
    throw - 1;
}
if (strncmp(recvBuf, itoa2(Checkflag), strlen(itoa2(Checkflag))) == 0)
{
    if (x == 0)
    {
        y++;
        goto LISTEN;
    }
    else
    {
        x = 0;
        Checkflag++;
        goto CK;
    }
}
else
if (strncmp(recvBuf, itoa2(Checkflag - 1), strlen(itoa2(Checkflag - 1))) == 0)
{

    Checkflag++;
    goto CO;
}
else
{
    {
        if (++flagcount >= 3)
        {
            std::cout << "resend failed!" << "ID:" << recvBuf << std::endl;
            Checkflag++;
            if (sendto(sockClient, "RESENDINGALL", 12, 0, (SOCKADDR*)&addrSrv1, sizeof(SOCKADDR)) == -1)
            {
                std::cout << stderr << "sendto error" << std::endl;
                throw - 1;
            }
            goto R;
        }
        else
        {
            std::cout << "wrong!trying resend!" << std::endl;
            goto CO;
        }
    }
}
}
}

/*
if ((n = recvfrom(sockSrv, recvBuf, MAX_LEN, 0, (SOCKADDR*)&addrClient, &len)) < 0) //recive the data from the server
{

    std::cout << stderr << "Can't Receive datagram" << std::endl;
    std::cout << "resending!" << std::endl;
    goto RESEND;
}
// std::cout<<recvBuf;
if (strncmp(recvBuf, itoa2(), n) != 0) //continue waiting untill the server agree to transport,then send the file name
{
    std::cout << "resending!" << std::endl;
    goto RESEND;
}*/
Sleep(1000); //sleep another second, then send the file end mark.


if (sendto(sockClient, file_end, strlen(file_end), 0, (SOCKADDR*)&addrSrv1, sizeof(SOCKADDR)) == -1)
{
    std::cout << stderr << "sendto error" << std::endl;
    throw - 1;
}

if ((n = recvfrom(sockSrv, recvBuf, MAX_LEN, 0, (SOCKADDR*)&addrClient, &len)) < 0) //recive the data from the server
{

    std::cout << stderr << "Can't Receive datagram" << std::endl;
    throw - 1;
}
if ((n = recvfrom(sockSrv, recvBuf, MAX_LEN, 0, (SOCKADDR*)&addrClient, &len)) < 0) //recive the data from the server
{

    std::cout << stderr << "Can't Receive datagram" << std::endl;
    throw - 1;
}
if (strncmp(recvBuf, itoa2(Send_Total), n) != 0)
{
    std::cout << "Lost while Sending!" << std::endl;
    if (sendto(sockClient, "RESENDINGALL", 12, 0, (SOCKADDR*)&addrSrv1, sizeof(SOCKADDR)) == -1)
    {
        std::cout << stderr << "sendto error" << std::endl;
        throw - 1;
    }
    goto R;
}
else
{
    std::cout << "Checking!" << std::endl;
    if (sendto(sockClient, "1", strlen("1"), 0, (SOCKADDR*)&addrSrv1, sizeof(SOCKADDR)) == -1)
    {
        std::cout << stderr << "sendto error" << std::endl;
        throw - 1;
    }
}
std::cout << "Finished!! Totally Sent : " << Send_Total << " byte" << std::endl;
fclose(tempfile);
closesocket(sockClient);
WSACleanup();
return 0;

}