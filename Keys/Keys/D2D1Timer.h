/***********************************************************************
���ܣ�һ���߾��ȼ�ʱ����������������֡��/���ʱ��
���ߣ�Ray1024
��ַ��http://www.cnblogs.com/Ray1024/
***********************************************************************/


#include "framework.h"

// ��ʱ��
class D2D1Timer
{
public:
	// ���캯��
	D2D1Timer()
	{
		__int64 countsPerSec;
		QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
		m_secondsPerCount = 1.0 / (double)countsPerSec;
	}
	// ��ʱ�䣬��λΪ��
	float TotalTime() const
	{
		if (m_stopped)
			return (float)(((m_stopTime - m_pausedTime) - m_baseTime) * m_secondsPerCount);
		else
			return (float)(((m_currTime - m_pausedTime) - m_baseTime) * m_secondsPerCount);
	}
	// ���ʱ�䣬��λΪ��
	float DeltaTime() const { return (float)m_deltaTime; }

	// ��Ϣѭ��ǰ����
	void Reset()
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

		m_baseTime = currTime;
		m_prevTime = currTime;
		m_stopTime = 0;
		m_stopped = false;
	}
	// ȡ����ͣʱ����
	void Start()
	{
		__int64 startTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

		if (m_stopped)
		{
			// ���ۼ���ͣʱ��
			m_pausedTime += (startTime - m_stopTime);
			// ��Ϊ�������¿�ʼ��ʱ�����m_prevTime��ֵ�Ͳ���ȷ�ˣ�
			// Ҫ��������Ϊ��ǰʱ�� 
			m_prevTime = startTime;
			// ȡ����ͣ״̬
			m_stopTime = 0;
			m_stopped = false;
		}
	}
	// ��ͣʱ����
	void Stop()
	{
		// �����������ͣ״̬�����Թ�����Ĳ���
		if (!m_stopped)
		{
			__int64 currTime;
			QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

			// ��¼��ͣ��ʱ�䣬�����ñ�ʾ��ͣ״̬�ı�־
			m_stopTime = currTime;
			m_stopped = true;
		}
	}
	// ÿ֡����
	void Tick()
	{
		if (m_stopped)
		{
			m_deltaTime = 0.0;
			return;
		}

		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
		m_currTime = currTime;

		// ��ǰ֡����һ֮֡���ʱ���
		m_deltaTime = (m_currTime - m_prevTime) * m_secondsPerCount;

		// Ϊ������һ֡��׼��
		m_prevTime = m_currTime;

		// ȷ����Ϊ��ֵ��DXSDK�е�CDXUTTimer�ᵽ����������������˽ڵ�ģʽ
		// ���л�����һ����������m_deltaTime���Ϊ��ֵ��
		if (m_deltaTime < 0.0)
		{
			m_deltaTime = 0.0;
		}
	}

private:
	double						m_secondsPerCount = 0.0;
	double						m_deltaTime = -1.0;

	__int64						m_baseTime = 0;
	__int64						m_pausedTime = 0;
	__int64						m_stopTime = 0;
	__int64						m_prevTime = 0;
	__int64						m_currTime = 0;

	bool						m_stopped = false;
};

