// main.cpp : ����Ӧ�ó������ڵ㡣
//
#include "App.h"

#define MAX_LOADSTRING 100
//-----------------------------------------------------------------
// ���ܣ�key
// ���ߣ��ֺ���
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// ��ֹ�ظ�����
void  ForbiddenRepeatedlyRun()
{
    if (OpenEventA(EVENT_ALL_ACCESS, false, "WR_KEY") != NULL)
    {
        exit(0);
    }
    CreateEvent(NULL, false, false, L"WR_KEY");
}

/******************************************************************
�������
******************************************************************/
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ForbiddenRepeatedlyRun();

    // ���Է���ֵ����Ϊ����ϣ����ʹ�� HeapSetInformation(ָ���Ķ����ù���) ��̫����ʧ�ܵ������Ҳ��������
    ::HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0); //Ϊָ���Ķ����ù��ܡ�

    if (SUCCEEDED(CoInitialize(NULL)))          //����ɹ�����ʼ����ǰ�߳��ϵ� COM �⣬��������ģ�ͱ�ʶΪ���̵߳�Ԫ (STA) ����
    {
        {
            App app;                        //��ʾӦ�ó���

            if (SUCCEEDED(app.Initialize()))    //����ɹ�(��ʼ����
            {
                app.Run();                      //������Ϣѭ��
            }
        }
        ::CoUninitialize();                       //�رյ�ǰ�߳��ϵ� COM �⣬ж���̼߳��ص����� DLL���ͷ��߳�ά�����κ�������Դ����ǿ�ƹر��߳��ϵ����� RPC ���ӡ�
    }

    return 0;
}