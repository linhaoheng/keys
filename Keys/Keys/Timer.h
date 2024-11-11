#include <chrono>

class Timer
{
public:
    // ��ʼ��ʱ�ĺ���
    void start()
    {
        if (!isRunning)
        {
            startTime = std::chrono::steady_clock::now();
            isRunning = true;
        }
    }
    // ��ͣ��ʱ�ĺ���
    void pause() 
    {
        if (isRunning) 
        {
            pauseTime = std::chrono::steady_clock::now();
            totalTime += std::chrono::duration_cast<std::chrono::milliseconds>(pauseTime - startTime).count();
            isRunning = false;
            startTime = std::chrono::time_point<std::chrono::steady_clock>();
        }
    }

    // ���ü�ʱ���ĺ���
    void reset() {
        startTime = std::chrono::time_point<std::chrono::steady_clock>();
        pauseTime = std::chrono::time_point<std::chrono::steady_clock>();
        totalTime = 0;
        isRunning = false;
    }
    // ��ȡ�Ѿ���ȥ��ʱ�䣨���Զ���� 1/1000 ��Ϊ��λ���ĺ���
    long getElapsedTime() 
    {
        if (!isRunning)
            return totalTime;
        auto now = std::chrono::steady_clock::now();
        // ����ӿ�ʼʱ�䵽��ǰʱ��ľ�������������ת��Ϊ�Զ��嵥λ��1/1000 �룩
        long elapsedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
        return totalTime + elapsedMilliseconds;
    }

private:
    // ��¼��ʼʱ���ʱ���
    std::chrono::time_point<std::chrono::steady_clock> startTime;
    // ��¼��ͣʱ���ʱ���
    std::chrono::time_point<std::chrono::steady_clock> pauseTime;
    // ��¼�ܵľ���ʱ�䣨���Զ���� 1/1000 ��Ϊ��λ��
    long totalTime = 0;
    // ��ʱ���Ƿ��������еı�־
    bool isRunning = false;
};

