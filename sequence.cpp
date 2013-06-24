#include<stdlib.h>
#include "sequence.h"
#include "utils.h"
#include "serial_command.h"
#include "sensor.h"
#include "actuator.h"
#include "motor.h"

Testing gTestingState;
Waiting gWaitingState;
Falling gFallingState;
Separating gSeparatingState;

bool Testing::onInit(const struct timespec& time)
{
	TaskManager::getInstance()->setRunMode(false);
	setRunMode(true);
	gBuzzer.setRunMode(true);
	gServo.setRunMode(true);
	gXbeeSleep.setRunMode(true);

	gPressureSensor.setRunMode(true);
	gGPSSensor.setRunMode(true);
	gGyroSensor.setRunMode(true);
	gLightSensor.setRunMode(true);
	gWebCamera.setRunMode(true);

	gMotorDrive.setRunMode(true);

	gSerialCommand.setRunMode(true);

	return true;
}
bool Testing::onCommand(const std::vector<std::string> args)
{
	if(args.size() == 3)
	{
		if(args[1].compare("start") == 0)
		{
			TaskBase* pTask = TaskManager::getInstance()->get(args[2]);
			if(pTask != NULL)
			{
				Debug::print(LOG_SUMMARY, "Starting %s...\r\n",args[2].c_str());
				pTask->setRunMode(true);
				return true;
			}else Debug::print(LOG_SUMMARY, "%s Not Found\r\n",args[2].c_str());
			return false;
		}else if(args[1].compare("stop") == 0)
		{
			TaskBase* pTask = TaskManager::getInstance()->get(args[2]);
			if(pTask != NULL)
			{
				Debug::print(LOG_SUMMARY, "Starting %s...\r\n",args[2].c_str());
				pTask->setRunMode(true);
				return true;
			}else Debug::print(LOG_SUMMARY, "%s Not Found\r\n",args[2].c_str());
			return false;
		}
	}
	Debug::print(LOG_PRINT, "testing [start/stop] module_name\r\n");

	return false;
}
Testing::Testing()
{
	setName("testing");
	setPriority(UINT_MAX,UINT_MAX);
}
Testing::~Testing()
{
}


bool Waiting::onInit(const struct timespec& time)
{
	Debug::print(LOG_SUMMARY, "Waiting...\r\n");

	mContinuousLightCount = 0;

	//�K�v�ȃ^�X�N���g�p�ł���悤�ɂ���
	TaskManager::getInstance()->setRunMode(false);
	setRunMode(true);
	gSerialCommand.setRunMode(true);
	gLightSensor.setRunMode(true);
	gXbeeSleep.setRunMode(true);
	gBuzzer.setRunMode(true);

	//���݂̎�����ۑ�
	mStartTime = time;
	

	Debug::print(LOG_SUMMARY, "Disable Communication\r\ncya!\r\n");

	return true;
}
void Waiting::nextState()
{
	gBuzzer.start(100);

	//�X���[�v������
	gXbeeSleep.setState(false);
	//���̏�Ԃ�ݒ�
	gFallingState.setRunMode(true);
	
	Debug::print(LOG_SUMMARY, "Waiting Finished!\r\n");
}
void Waiting::onUpdate(const struct timespec& time)
{
	//XBee���X���[�v���[�h�ɐݒ�(���P�b�g���d�g�K��)
	gXbeeSleep.setState(true);

	//���邢�ꍇ�J�E���g
	if(gLightSensor.get())
	{
		++mContinuousLightCount;
		gBuzzer.start(1);
	}else mContinuousLightCount = 0;

	if(mContinuousLightCount >= CONTINUOUS_LIGHT_COUNT)//���邢�ꍇ���o����
	{
		nextState();
		return;
	}

	if(Time::dt(time,mStartTime) > WAITING_ABORT_TIME)//��莞�Ԃ��o�߂����玟�̏�Ԃɋ����ύX
	{
		Debug::print(LOG_SUMMARY, "Waiting Timeout\r\n");
		nextState();
		return;
	}
}
Waiting::Waiting()
{
	setName("waiting");
	setPriority(TASK_PRIORITY_SEQUENCE,TASK_INTERVAL_SEQUENCE);
}
Waiting::~Waiting(){}

bool Falling::onInit(const struct timespec& time)
{
	Debug::print(LOG_SUMMARY, "Falling...\r\n");

	//�K�v�ȃ^�X�N���g�p�ł���悤�ɂ���
	TaskManager::getInstance()->setRunMode(false);
	setRunMode(true);
	gSerialCommand.setRunMode(true);
	gBuzzer.setRunMode(true);
	gPressureSensor.setRunMode(true);

	mLastCheckTime = time;

	return true;
}
void Falling::onUpdate(const struct timespec& time)
{
	//1�b���ƂɋC�����`�F�b�N
	if(Time::dt(time,mLastCheckTime) < 1)return;
	mLastCheckTime = time;

	//�C���̍������ȏ�Ȃ�J�E���g
	int newPressure = gPressureSensor.get();
	if(abs((int)(newPressure - mLastPressure)) < FALLING_DELTA_PRESSURE_THRESHOLD)++mContinuousPressureCount;
	else mContinuousPressureCount = 0;

	//�J�E���g�񐔂����ȏ�Ȃ玟�̏�ԂɈڍs
	if(mContinuousPressureCount >= FALLING_PRESSURE_COUNT)
	{
		nextState();
		return;
	}
}
void Falling::nextState()
{
	gBuzzer.start(100);

	//���̏�Ԃ�ݒ�
	gSeparatingState.setRunMode(true);
	
	Debug::print(LOG_SUMMARY, "Falling Finished!\r\n");
}
Falling::Falling()
{
	setName("falling");
	setPriority(TASK_PRIORITY_SEQUENCE,TASK_INTERVAL_SEQUENCE);
}
Falling::~Falling()
{
}

bool Separating::onInit(const struct timespec& time)
{
	Debug::print(LOG_SUMMARY, "Separating...\r\n");

	//�K�v�ȃ^�X�N���g�p�ł���悤�ɂ���
	TaskManager::getInstance()->setRunMode(false);
	setRunMode(true);
	gSerialCommand.setRunMode(true);
	gBuzzer.setRunMode(true);
	gServo.setRunMode(true);

	mLastUpdateTime = time;
	gServo.start(0);
	mCurServoState = false;
	mServoCount = 0;

	return true;
}
void Separating::onUpdate(const struct timespec& time)
{
	if(Time::dt(time,mLastUpdateTime) < SEPARATING_SERVO_INTERVAL)return;
	mLastUpdateTime = time;

	mCurServoState = !mCurServoState;
	gServo.start(mCurServoState);
	++mServoCount;

	if(mServoCount >= SEPARATING_SERVO_COUNT)
	{
		nextState();
	}
}

void Separating::nextState()
{
	gBuzzer.start(100);

	//���̏�Ԃ�ݒ�
	//gLandingState.setRunMode(true);
	
	Debug::print(LOG_SUMMARY, "Separating Finished!\r\n");
}

Separating::Separating()
{
	setName("separating");
	setPriority(TASK_PRIORITY_SEQUENCE,TASK_INTERVAL_SEQUENCE);
}
Separating::~Separating()
{
}
