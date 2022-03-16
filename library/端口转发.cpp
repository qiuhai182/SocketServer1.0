// /*
// 端口映射
// PortTransfer_三种模式。
// (1) PortTransfer Port Dest_IP Port
// 在运行本程序的计算机上监听Port端口，并将所有连接请求转到Dest_IP的Port去
// (2) PortTransfer ctrlIP ctrlPort Dest_IP Port
// 和模式3配合用，程序将先主动连接ctrlIP:ctrlPort,之后所有在模式3的ServerPort端口的请求转到Dest_IP:Port去
// (3) PortTransfer ctrlPort ServerPort
// 在执行模式2前先执行，将监听ctrlPort和ServerPort 端口,ctrlPort供模式2使用，ServerPort提供服务.
// 模式1适合在网关上运行，将内网IP的端口映射到网关上，
// 如：PortTransfer 88 192.168.0.110 80
// 那么网关将开启88端口，所有在88端口的请求将转到内网的192.168.0.110的80端口
// 模式2和模式3联合使用可以将内网的IP和端口映射到指定的IP和端口上，
// 一般在公网IP(假设61.1.1.1)上执行模式3，如：PortTransfer 99 80, 80是映射过来的端口
// 内网用户执行模式2如：PortTransfer 61.1.1.1 99 127.0.0.1 80，
// 那么程序在内网将先连接61.1.1.1:99建立个连接，并等待接收命令。
// 之后当61.1.1.1的80端口有请求，将通过99端口命令内网机子和公网机子建立条新的数据连接，
// 并将请求通过新建的连接将请求转发到内网机.
// Code By LZX.
// 2006.08.31
// */
// /*
// Author: LZX
// E-mail: LZX@qq.com
// Version: V1.1
// Purpose: Mapping Port, Not support for UDP
// Test PlatForm: WinXP SP2
// Compiled On: VC++ 6.0
// Last Modified: 2006.08.31
// */
// // #include <WINSOCK2.H>
// // #include <windows.h>
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <stdio.h>
// #pragma comment(lib, "ws2_32.lib")
// #define SERVERNAME "ZXPortMap"
// #define VERSION "v1.0"
// #define MAXBUFSIZE 8192
// #define ADDRSIZE 32
// struct SOCKINFO
// {
//     Socket ClientSock;
//     Socket ServerSock;
// };
// struct ADDRESS
// {
//     char szIP[ADDRSIZE];
//     WORD wPort;
//     Socket s;
// };
// // A simple class of stack operator. code start.....
// template <typename T>
// class STACK
// {
// #define MAXSTACK 1024 * 2
// private:
//     int top;
//     T Data[MAXSTACK];

// public:
//     STACK()
//     {
//         top = -1;
//     }
//     bool IsEmpty()
//     {
//         return top < 0;
//     }
//     bool IsFull()
//     {
//         return top >= MAXSTACK;
//     }
//     bool Push(T data)
//     {
//         if (IsFull())
//             return false;
//         top++;
//         Data[top] = data;
//         return true;
//     }
//     T Pop()
//     {
//         return Data[top--];
//     }
// }; //stack end
// //Transfer some Parameters
// template <typename X, typename Y>
// class TransferParam
// {
// public:
//     X GlobalData;
//     STACK<Y> LocalData;

// public:
//     TransferParam();
//     virtual ~TransferParam();
//     bool Push(Y data);
//     Y Pop();
// };
// template <typename X, typename Y>
// TransferParam<X, Y>::TransferParam()
// {
//     memset(this, 0, sizeof(TransferParam));
// }
// template <typename X, typename Y>
// TransferParam<X, Y>::~TransferParam()
// {
// }
// template <typename X, typename Y>
// bool TransferParam<X, Y>::Push(Y data)
// {
//     return LocalData.Push(data);
// }
// template <typename X, typename Y>
// Y TransferParam<X, Y>::Pop()
// {
//     return LocalData.Pop(data);
// }
// //
// int nTimes = 0;
// int DataSend(Socket s, char *DataBuf, int DataLen) //将DataBuf中的DataLen个字节发到s去
// {
//     int nBytesLeft = DataLen;
//     int nBytesSent = 0;
//     int ret;
//     //set Socket to blocking mode
//     int iMode = 0;
//     ioctlSocket(s, FIONBIO, (u_long FAR *)&iMode);
//     while (nBytesLeft > 0)
//     {
//         ret = send(s, DataBuf + nBytesSent, nBytesLeft, 0);
//         if (ret <= 0)
//             break;
//         nBytesSent += ret;
//         nBytesLeft -= ret;
//     }
//     return nBytesSent;
// }
// DWORD WINAPI TransmitData(LPVOID lParam) //在两个Socket中进行数据转发
// {
//     SOCKINFO socks = *((SOCKINFO *)lParam);
//     Socket ClientSock = socks.ClientSock;
//     Socket ServerSock = socks.ServerSock;
//     char RecvBuf[MAXBUFSIZE] = {0};
//     fd_set Fd_Read;
//     int ret, nRecv;
//     while (1)
//     {
//         FD_ZERO(&Fd_Read);
//         FD_SET(ClientSock, &Fd_Read);
//         FD_SET(ServerSock, &Fd_Read);
//         ret = select(0, &Fd_Read, NULL, NULL, NULL);
//         if (ret <= 0)
//             goto error;
//         if (FD_ISSET(ClientSock, &Fd_Read))
//         {
//             nRecv = recv(ClientSock, RecvBuf, sizeof(RecvBuf), 0);
//             if (nRecv <= 0)
//                 goto error;
//             ret = DataSend(ServerSock, RecvBuf, nRecv);
//             if (ret == 0 || ret != nRecv)
//                 goto error;
//         }
//         if (FD_ISSET(ServerSock, &Fd_Read))
//         {
//             nRecv = recv(ServerSock, RecvBuf, sizeof(RecvBuf), 0);
//             if (nRecv <= 0)
//                 goto error;
//             ret = DataSend(ClientSock, RecvBuf, nRecv);
//             if (ret == 0 || ret != nRecv)
//                 goto error;
//         }
//     }
// error:
//     closeSocket(ClientSock);
//     closeSocket(ServerSock);
//     return 0;
// }
// Socket ConnectHost(DWORD dwIP, WORD wPort) //连接指定IP和端口
// {
//     Socket sockid;
//     if ((sockid = Socket(AF_INET, SOCK_STREAM, 0)) == INVALID_Socket)
//         return 0;
//     struct sockaddr_in srv_addr;
//     srv_addr.sin_family = AF_INET;
//     srv_addr.sin_addr.S_un.S_addr = dwIP;
//     srv_addr.sin_port = htons(wPort);
//     if (connect(sockid, (struct sockaddr *)&srv_addr, sizeof(struct sockaddr_in)) == Socket_ERROR)
//         goto error;
//     return sockid;
// error:
//     closeSocket(sockid);
//     return 0;
// }
// Socket ConnectHost(char *szIP, WORD wPort)
// {
//     return ConnectHost(inet_addr(szIP), wPort);
// }
// Socket CreateSocket(DWORD dwIP, WORD wPort) //在dwIP上绑定wPort端口
// {
//     Socket sockid;
//     if ((sockid = Socket(AF_INET, SOCK_STREAM, 0)) == INVALID_Socket)
//         return 0;
//     struct sockaddr_in srv_addr = {0};
//     srv_addr.sin_family = AF_INET;
//     srv_addr.sin_addr.S_un.S_addr = dwIP;
//     srv_addr.sin_port = htons(wPort);
//     if (bind(sockid, (struct sockaddr *)&srv_addr, sizeof(struct sockaddr_in)) == Socket_ERROR)
//         goto error;
//     listen(sockid, 3);
//     return sockid;
// error:
//     closeSocket(sockid);
//     return 0;
// }
// Socket CreateTmpSocket(WORD *wPort) //创建一个临时的套接字,指针wPort获得创建的临时端口
// {
//     struct sockaddr_in srv_addr = {0};
//     int addrlen = sizeof(struct sockaddr_in);
//     Socket s = CreateSocket(INADDR_ANY, 0);
//     if (s <= 0)
//         goto error;
//     if (getsockname(s, (struct sockaddr *)&srv_addr, &addrlen) == Socket_ERROR)
//         goto error;
//     *wPort = ntohs(srv_addr.sin_port);
//     return s;
// error:
//     closeSocket(s);
//     return 0;
// }
// BOOL InitSocket()
// {
//     WSADATA wsadata;
//     return WSAStartup(MAKEWORD(2, 2), &wsadata) == 0;
// }
// DWORD WINAPI PortTransfer_1(LPVOID lParam)
// {
//     TransferParam<ADDRESS, Socket> *ConfigInfo = (TransferParam<ADDRESS, Socket> *)lParam;
//     Socket ClientSock, ServerSock;
//     //出栈，获得客户的套接字
//     ClientSock = ConfigInfo->LocalData.Pop();
//     printf("ThreadID: %d ==> Now Connecting To Server...", nTimes);
//     //先连接到目标计算机的服务
//     ServerSock = ConnectHost(ConfigInfo->GlobalData.szIP, ConfigInfo->GlobalData.wPort);
//     if (ServerSock <= 0)
//     {
//         printf("Error./r/n");
//         closeSocket(ClientSock);
//         return 0;
//     }
//     printf("OK./r/nStarting TransmitData/r/n");
//     SOCKINFO socks;
//     socks.ClientSock = ClientSock; //客户的套接字
//     socks.ServerSock = ServerSock; //目标计算机服务的套接字
//     //进入纯数据转发状态
//     return TransmitData((LPVOID)&socks);
// }
// BOOL PortTransfer_1(WORD ListenPort, char *szIP, WORD wPort)
// {
//     HANDLE hThread;
//     DWORD dwThreadId;
//     Socket AcceptSocket;
//     TransferParam<ADDRESS, Socket> ConfigInfo;
//     _snprintf(ConfigInfo.GlobalData.szIP, ADDRSIZE, "%s", szIP);
//     ConfigInfo.GlobalData.wPort = wPort;
//     //监听个服务端口，即映射端口
//     Socket localsockid = CreateSocket(INADDR_ANY, ListenPort);
//     if (localsockid <= 0)
//         goto error;
//     while (1)
//     {
//         printf("Accepting new Client...");
//         AcceptSocket = accept(localsockid, NULL, NULL);
//         if (AcceptSocket == INVALID_Socket)
//             goto error;
//         nTimes++;
//         printf("OK./r/n");
//         //将接受到的客户请求套接字转到新的线程里处理
//         //然后继续等待新的请求
//         ConfigInfo.LocalData.Push(AcceptSocket);
//         hThread = CreateThread(NULL, 0, PortTransfer_1, (LPVOID)&ConfigInfo, NULL, &dwThreadId);
//         if (hThread)
//             CloseHandle(hThread);
//         else
//             Sleep(1000);
//     }
// error:
//     printf("Error./r/n");
//     closeSocket(localsockid);
//     return false;
// }
// DWORD WINAPI PortTransfer_2(LPVOID lParam)
// {
//     TransferParam<ADDRESS, WORD> *ConfigInfo = (TransferParam<ADDRESS, WORD> *)lParam;
//     Socket CtrlSocket = ConfigInfo->GlobalData.s;
//     DWORD dwCtrlIP;
//     //WORD wPort;
//     SOCKADDR_IN clientaddr;
//     int addrlen = sizeof(clientaddr);
//     //之前用错了个API(getsockname),这里应该用getpeername
//     if (getpeername(CtrlSocket, (SOCKADDR *)&clientaddr, &addrlen) == Socket_ERROR)
//         return 0;
//     //获得运行PortTransfer_3模式的计算机的IP
//     dwCtrlIP = clientaddr.sin_addr.S_un.S_addr;
//     //wPort = ntohs(clientaddr.sin_port);
//     Socket ClientSocket, ServerSocket;
//     SOCKINFO socks;
//     printf("ThreadID: %d ==> Connecting to Client...", nTimes);
//     //向公网建立新的连接
//     ClientSocket = ConnectHost(dwCtrlIP, ConfigInfo->LocalData.Pop());
//     if (ClientSocket <= 0)
//         return 0;
//     printf("OK./r/n");
//     printf("ThreadID: %d ==> Connect to Server...", nTimes);
//     //连接到目标计算机的服务
//     ServerSocket = ConnectHost(ConfigInfo->GlobalData.szIP, ConfigInfo->GlobalData.wPort);
//     if (ServerSocket <= 0)
//     {
//         printf("Error./r/n");
//         closeSocket(ClientSocket);
//         return 0;
//     }
//     printf("OK./r/nStarting TransmitData/r/n", nTimes);
//     socks.ClientSock = ClientSocket; //公网计算机的套接字
//     socks.ServerSock = ServerSocket; //目标计算机服务的套接字
//     //进入纯数据转发状态
//     return TransmitData((LPVOID)&socks);
// }
// BOOL PortTransfer_2(char *szCtrlIP, WORD wCtrlPort, char *szIP, WORD wPort)
// {
//     int nRecv;
//     WORD ReqPort;
//     HANDLE hThread;
//     DWORD dwThreadId;
//     TransferParam<ADDRESS, WORD> ConfigInfo;
//     _snprintf(ConfigInfo.GlobalData.szIP, ADDRSIZE, "%s", szIP);
//     ConfigInfo.GlobalData.wPort = wPort;
//     printf("Creating a ctrlconnection...");
//     //与PortTransfer_3模式（工作在共网）的计算机建立控制管道连接
//     Socket CtrlSocket = ConnectHost(szCtrlIP, wCtrlPort);
//     if (CtrlSocket <= 0)
//         goto error;
//     ConfigInfo.GlobalData.s = CtrlSocket;
//     printf("OK./r/n");
//     while (1)
//     {
//         //接收来自（工作在公网）计算机的命令，数据为一个WORD，
//         //表示公网计算机监听了这个端口
//         nRecv = recv(CtrlSocket, (char *)&ReqPort, sizeof(ReqPort), 0);
//         if (nRecv <= 0)
//             goto error;
//         nTimes++;
//         ConfigInfo.LocalData.Push(ReqPort); //传递信息的结构
//         hThread = CreateThread(NULL, 0, PortTransfer_2, (LPVOID)&ConfigInfo, NULL, &dwThreadId);
//         if (hThread)
//             CloseHandle(hThread);
//         else
//             Sleep(1000);
//     }
// error:
//     printf("Error./r/n");
//     closeSocket(CtrlSocket);
//     return false;
// }
// DWORD WINAPI PortTransfer_3(LPVOID lParam)
// {
//     SOCKINFO socks;
//     Socket ClientSocket, ServerSocket, CtrlSocket, tmpSocket;
//     TransferParam<Socket, Socket> *ConfigInfo = (TransferParam<Socket, Socket> *)lParam;
//     CtrlSocket = ConfigInfo->GlobalData;
//     ClientSocket = ConfigInfo->LocalData.Pop();
//     WORD wPort;
//     tmpSocket = CreateTmpSocket(&wPort); //创建个临时端口
//     if (tmpSocket <= 0 || wPort <= 0)
//     {
//         closeSocket(ClientSocket);
//         return 0;
//     }
//     //通知内网用户发起新的连接到刚创建的临时端口
//     if (send(CtrlSocket, (char *)&wPort, sizeof(wPort), 0) == Socket_ERROR)
//     {
//         closeSocket(ClientSocket);
//         closeSocket(CtrlSocket);
//         return 0;
//     }
//     printf("ThreadID: %d ==> Waiting for server connection...", nTimes);
//     ServerSocket = accept(tmpSocket, NULL, NULL);
//     if (ServerSocket == INVALID_Socket)
//     {
//         printf("Error./r/n");
//         closeSocket(ClientSocket);
//         return 0;
//     }
//     printf("OK./r/n");
//     socks.ClientSock = ClientSocket;
//     socks.ServerSock = ServerSocket;
//     //进入纯数据转发状态
//     return TransmitData((LPVOID)&socks);
// }
// BOOL PortTransfer_3(WORD wCtrlPort, WORD wServerPort) //监听的两个端口
// {
//     HANDLE hThread;
//     DWORD dwThreadId;
//     BOOL bOptVal = TRUE;
//     int bOptLen = sizeof(BOOL);
//     TransferParam<Socket, Socket> ConfigInfo;
//     Socket ctrlsockid, serversockid, CtrlSocket, AcceptSocket;
//     ctrlsockid = CreateSocket(INADDR_ANY, wCtrlPort); //创建套接字
//     if (ctrlsockid <= 0)
//         goto error2;
//     serversockid = CreateSocket(INADDR_ANY, wServerPort); //创建套接字
//     if (serversockid <= 0)
//         goto error1;
//     CtrlSocket = accept(ctrlsockid, NULL, NULL); //接受来自（内网用户发起）PortTransfer_2模式建立控制管道连接的请求
//     if (CtrlSocket == INVALID_Socket)
//         goto error0;
//     //setsockopt( keep-alive......
//     if (setsockopt(CtrlSocket, SOL_Socket, SO_KEEPALIVE, (char *)&bOptVal, bOptLen) == Socket_ERROR)
//     {
//         goto error0;
//         //printf("Set SO_KEEPALIVE: ON/n");
//     }
//     //与内网用户建立了连接后就相当端口映射成功了
//     //准备进入接收服务请求状态，并将在新起的线程中通过控制管道通知内网用户发起新的连接进行数据转发
//     ConfigInfo.GlobalData = CtrlSocket;
//     while (1)
//     {
//         printf("Accepting new Client.../r/n");
//         AcceptSocket = accept(serversockid, NULL, NULL);
//         if (AcceptSocket == INVALID_Socket)
//         {
//             printf("Error./r/n");
//             Sleep(1000);
//             continue;
//         }
//         nTimes++;
//         printf("OK./r/n");
//         ConfigInfo.LocalData.Push(AcceptSocket); //把接受到的套接字Push到栈结构中，传到新起线程那边可以再Pop出来
//         hThread = CreateThread(NULL, 0, PortTransfer_3, (LPVOID)&ConfigInfo, NULL, &dwThreadId);
//         if (hThread)
//             CloseHandle(hThread);
//         else
//             Sleep(1000);
//     }
// error0:
//     closeSocket(CtrlSocket);
// error1:
//     closeSocket(serversockid);
// error2:
//     closeSocket(ctrlsockid);
//     return false;
// }
// void Usage(char *ProName)
// {
//     printf("%s %s By LZX./r/n", SERVERNAME, VERSION);
//     printf("Usage:/r/n"
//            " %s ctrlPort ServerPort/r/n"
//            " %s Port Dest_IP Port/r/n"
//            " %s ctrlIP ctrlPort Dest_IP Port/r/n",
//            ProName, ProName, ProName);
// }
// int main(int argc, char **argv)
// {
//     if (!InitSocket())
//         return 0;
//     if (argc == 3)
//         PortTransfer_3(atoi(argv[1]), atoi(argv[2]));
//     else if (argc == 4)
//         PortTransfer_1(atoi(argv[1]), argv[2], atoi(argv[3]));
//     else if (argc == 5)
//         PortTransfer_2(argv[1], atoi(argv[2]), argv[3], atoi(argv[4]));
//     else
//         Usage(argv[0]);
//     WSACleanup();
//     return 0;
// }
