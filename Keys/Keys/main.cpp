// main.cpp : 定义应用程序的入口点。
//
#include "App.h"

#define MAX_LOADSTRING 100
//-----------------------------------------------------------------
// 功能：key
// 作者：林豪横
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// 禁止重复运行
void  ForbiddenRepeatedlyRun()
{
    if (OpenEventA(EVENT_ALL_ACCESS, false, "WR_KEY") != NULL)
    {
        exit(0);
    }
    CreateEvent(NULL, false, false, L"WR_KEY");
}

/******************************************************************
程序入口
******************************************************************/
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ForbiddenRepeatedlyRun();

    // 忽略返回值，因为我们希望即使在 HeapSetInformation(指定的堆启用功能) 不太可能失败的情况下也继续运行
    ::HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0); //为指定的堆启用功能。

    if (SUCCEEDED(CoInitialize(NULL)))          //如果成功（初始化当前线程上的 COM 库，并将并发模型标识为单线程单元 (STA) 。）
    {
        {
            App app;                        //演示应用程序

            if (SUCCEEDED(app.Initialize()))    //如果成功(初始化）
            {
                app.Run();                      //运行消息循环
            }
        }
        ::CoUninitialize();                       //关闭当前线程上的 COM 库，卸载线程加载的所有 DLL，释放线程维护的任何其他资源，并强制关闭线程上的所有 RPC 连接。
    }

    return 0;
}