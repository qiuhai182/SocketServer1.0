
#include <signal.h>
#include "../library/EventLoop.hpp"
#include "../library/EchoServer.hpp"

EventLoop *lp;

static void sighandler1( int sig_no )   
{   
      exit(0);   
}

static void sighandler2( int sig_no )   
{   
    lp->Quit();
}   

int main(int argc, char *argv[])
{
    signal(SIGUSR1, sighandler1);
    signal(SIGUSR2, sighandler2);
    signal(SIGINT, sighandler2);
    signal(SIGPIPE, SIG_IGN);  //SIG_IGN,系统函数，忽略信号的处理程序
    
    int port = 8000;
    int iothreadnum = 4;
    int workerthreadnum = 4;
    if(argc == 4)
    {
        port = atoi(argv[1]);
        iothreadnum = atoi(argv[2]);
        workerthreadnum = atoi(argv[3]);
    }   

    // EventLoop loop;
    // lp = &loop;
    // HttpServer httpServer(&loop, port, iothreadnum, workerthreadnum);
    // httpServer.Start();
    // try
    // {
    //     loop.loop();
    // }
    // catch (std::bad_alloc& ba)
    // {
    //     std::cerr << "bad_alloc caught in ThreadPool::ThreadFunc task: " << ba.what() << '\n';
    // }

    EventLoop loop;
    lp = &loop;
    EchoServer echoServer(&loop, port, iothreadnum);
    echoServer.Start();
    loop.loop();

    return 0;
}
