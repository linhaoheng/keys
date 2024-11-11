#include <chrono>

class Timer
{
public:
    // 开始计时的函数
    void start()
    {
        if (!isRunning)
        {
            startTime = std::chrono::steady_clock::now();
            isRunning = true;
        }
    }
    // 暂停计时的函数
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

    // 重置计时器的函数
    void reset() {
        startTime = std::chrono::time_point<std::chrono::steady_clock>();
        pauseTime = std::chrono::time_point<std::chrono::steady_clock>();
        totalTime = 0;
        isRunning = false;
    }
    // 获取已经过去的时间（以自定义的 1/1000 秒为单位）的函数
    long getElapsedTime() 
    {
        if (!isRunning)
            return totalTime;
        auto now = std::chrono::steady_clock::now();
        // 计算从开始时间到当前时间的经过毫秒数，并转换为自定义单位（1/1000 秒）
        long elapsedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
        return totalTime + elapsedMilliseconds;
    }

private:
    // 记录开始时间的时间点
    std::chrono::time_point<std::chrono::steady_clock> startTime;
    // 记录暂停时间的时间点
    std::chrono::time_point<std::chrono::steady_clock> pauseTime;
    // 记录总的经过时间（以自定义的 1/1000 秒为单位）
    long totalTime = 0;
    // 计时器是否正在运行的标志
    bool isRunning = false;
};

