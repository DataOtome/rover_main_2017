#include<stdlib.h>
#include <math.h>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <opencv/cvaux.h>
#include <opencv/highgui.h>
#include "sequence.h"
#include "utils.h"
#include "serial_command.h"
#include "sensor.h"
#include "actuator.h"
#include "motor.h"
#include "image_proc.h"

Testing gTestingState;
Waiting gWaitingState;
Falling gFallingState;
Separating gSeparatingState;
Navigating gNavigatingState;
Escaping gEscapingState;
EscapingRandom gEscapingRandomState;
Waking gWakingState;
Turning gTurningState;
Avoiding gAvoidingState;
WadachiPredicting gPredictingState;
PictureTaking gPictureTakingState;

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
	//gAccelerationSensor.setRunMode(true);
	gLightSensor.setRunMode(true);
	gWebCamera.setRunMode(true);
	gDistanceSensor.setRunMode(true);
	gCameraCapture.setRunMode(true);

	gMotorDrive.setRunMode(true);

	gSerialCommand.setRunMode(true);

	return true;
}
bool Testing::onCommand(const std::vector<std::string> args)
{
	if(args.size() == 2)
	{
		if(args[1].compare("sensor") == 0)
		{
			Debug::print(LOG_SUMMARY, "*** Sensor states ***\r\n");
			
			VECTOR3 vec;
			gGPSSensor.get(vec);
			if(gGPSSensor.isActive())Debug::print(LOG_SUMMARY, " GPS      (%f %f %f)\r\n",vec.x,vec.y,vec.z);

			if(gPressureSensor.isActive())Debug::print(LOG_SUMMARY, " Pressure (%d) hPa\r\n",gPressureSensor.get());

			gGyroSensor.getRPos(vec);
			if(gGyroSensor.isActive())Debug::print(LOG_SUMMARY, " Gyro pos (%f %f %f) d\r\n",vec.x,vec.y,vec.z);
			gGyroSensor.getRVel(vec);
			if(gGyroSensor.isActive())Debug::print(LOG_SUMMARY, " Gyro vel (%f %f %f) dps\r\n",vec.x,vec.y,vec.z);

			if(gLightSensor.isActive())Debug::print(LOG_SUMMARY, " Light    (%s)\r\n",gLightSensor.get() ? "High" : "Low");
			return true;
		}else if(args[1].compare("waking") == 0)
		{
			gWakingState.setRunMode(true);
		}
	}
	if(args.size() == 3)
	{
		if(args[1].compare("start") == 0)
		{
			TaskBase* pTask = TaskManager::getInstance()->get(args[2]);
			if(pTask != NULL)
			{
				Debug::print(LOG_SUMMARY, "Start %s\r\n",args[2].c_str());
				pTask->setRunMode(true);
				return true;
			}else Debug::print(LOG_SUMMARY, "%s Not Found\r\n",args[2].c_str());
			return false;
		}else if(args[1].compare("stop") == 0)
		{
			TaskBase* pTask = TaskManager::getInstance()->get(args[2]);
			if(pTask != NULL)
			{
				Debug::print(LOG_SUMMARY, "Stop %s\r\n",args[2].c_str());
				pTask->setRunMode(false);
				return true;
			}else Debug::print(LOG_SUMMARY, "%s Not Found\r\n",args[2].c_str());
			return false;
		}
	}
	Debug::print(LOG_PRINT, "testing [start/stop] [task name]  : enable/disable task\r\n\
testing sensor                    : check sensor values\r\n");

	return true;
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

	//���݂̎�����ۑ�
	mStartTime = time;

	//�K�v�ȃ^�X�N���g�p�ł���悤�ɂ���
	TaskManager::getInstance()->setRunMode(false);
	setRunMode(true);
	gLightSensor.setRunMode(true);
	gXbeeSleep.setRunMode(true);
	gBuzzer.setRunMode(true);

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
		gBuzzer.start(2);
	}else mContinuousLightCount = 0;

	if(mContinuousLightCount >= WAITING_LIGHT_COUNT)//���邢�ꍇ���o����
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

	mStartTime = mLastCheckTime = time;
	mLastPressure = 0;
	mContinuousPressureCount = 0;
	mCoutinuousGyroCount = 0;

	//�K�v�ȃ^�X�N���g�p�ł���悤�ɂ���
	TaskManager::getInstance()->setRunMode(false);
	setRunMode(true);
	gBuzzer.setRunMode(true);
	gPressureSensor.setRunMode(true);
	gGyroSensor.setRunMode(true);
	gSerialCommand.setRunMode(true);
	gMotorDrive.setRunMode(true);

	return true;
}
void Falling::onUpdate(const struct timespec& time)
{
	//����̂݋C�����擾
	if(mLastPressure == 0)mLastPressure = gPressureSensor.get();

	//�p���x��臒l�ȉ��Ȃ�J�E���g
	if(gGyroSensor.getRvx() < FALLING_GYRO_THRESHOLD && gGyroSensor.getRvy() < FALLING_GYRO_THRESHOLD && gGyroSensor.getRvz() < FALLING_GYRO_THRESHOLD)
	{
		if(mCoutinuousGyroCount < FALLING_GYRO_COUNT)++mCoutinuousGyroCount;
	}else mCoutinuousGyroCount = 0;

	//1�b���ƂɈȉ��̏������s��
	if(Time::dt(time,mLastCheckTime) < 1)return;
	mLastCheckTime = time;

	//�C���̍������ȉ��Ȃ�J�E���g
	int newPressure = gPressureSensor.get();
	if(abs((int)(newPressure - mLastPressure)) < FALLING_DELTA_PRESSURE_THRESHOLD)
	{
		if(mContinuousPressureCount < FALLING_PRESSURE_COUNT)++mContinuousPressureCount;
	}else mContinuousPressureCount = 0;
	mLastPressure = newPressure;

	//�G���R�[�_�̒l�̍������ȏ�Ȃ�J�E���g
	unsigned long long newMotorPulseL = gMotorDrive.getL(), newMotorPulseR = gMotorDrive.getR();
	if(newMotorPulseL - mLastMotorPulseL > FALLING_MOTOR_PULSE_THRESHOLD || newMotorPulseR - mLastMotorPulseR > FALLING_MOTOR_PULSE_THRESHOLD)
	{
		if(mContinuousMotorPulseCount < FALLING_MOTOR_PULSE_COUNT)++mContinuousMotorPulseCount;
	}else mContinuousMotorPulseCount = 0;

	//�����Ԃ�\��
	Debug::print(LOG_SUMMARY, "Pressure Count   %d / %d (%d hPa)\r\n",mContinuousPressureCount,FALLING_PRESSURE_COUNT,newPressure);
	Debug::print(LOG_SUMMARY, "Gyro Count       %d / %d\r\n",mCoutinuousGyroCount,FALLING_GYRO_COUNT);
	Debug::print(LOG_SUMMARY, "MotorPulse Count %d / %d (%llu,%llu)\r\n",mContinuousMotorPulseCount,FALLING_MOTOR_PULSE_COUNT,newMotorPulseL - mLastMotorPulseL,newMotorPulseR - mLastMotorPulseR);

	mLastMotorPulseL = newMotorPulseL;
	mLastMotorPulseR = newMotorPulseR;

	//�J�E���g�񐔂����ȏ�Ȃ玟�̏�ԂɈڍs
	if(mContinuousPressureCount >= FALLING_PRESSURE_COUNT && (mCoutinuousGyroCount >= FALLING_GYRO_COUNT || mContinuousMotorPulseCount >= FALLING_MOTOR_PULSE_COUNT))
	{
		nextState();
		return;
	}

	if(Time::dt(time,mStartTime) > FALLING_ABORT_TIME)//��莞�Ԃ��o�߂����玟�̏�Ԃɋ����ύX
	{
		Debug::print(LOG_SUMMARY, "Waiting Timeout\r\n");
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
Falling::Falling() : mLastPressure(0),mLastMotorPulseL(0),mLastMotorPulseR(0),mContinuousPressureCount(0),mCoutinuousGyroCount(0),mContinuousMotorPulseCount(0)
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
	gBuzzer.setRunMode(true);
	gServo.setRunMode(true);
	gSerialCommand.setRunMode(true);
	gMotorDrive.setRunMode(true);
	gGyroSensor.setRunMode(true);
	gCameraCapture.setRunMode(true);

	mLastUpdateTime = time;
	gServo.start(0);
	mCurServoState = false;
	mServoCount = 0;
	mCurStep = STEP_SEPARATE;

	return true;
}
void Separating::onUpdate(const struct timespec& time)
{
	switch(mCurStep)
	{
	case STEP_SEPARATE:
		//�p���V���[�g��؂藣��
		if(Time::dt(time,mLastUpdateTime) < SEPARATING_SERVO_INTERVAL)return;
		mLastUpdateTime = time;

		mCurServoState = !mCurServoState;
		gServo.start(mCurServoState);
		++mServoCount;
		Debug::print(LOG_SUMMARY, "Separating...(%d/%d)\r\n", mServoCount, SEPARATING_SERVO_COUNT);

		if(mServoCount >= SEPARATING_SERVO_COUNT)//�T�[�{���K��񐔓�������
		{
			//����ԂɑJ��
			mLastUpdateTime = time;
			mCurStep = STEP_PRE_PARA_JUDGE;
			gWakingState.setRunMode(true);
		}
		break;
	case STEP_PRE_PARA_JUDGE:
		//�N���オ�蓮������s���A�摜�������s���O��1�b�ҋ@���ĉ摜�̃u����h�~����
		if(gWakingState.isActive())mLastUpdateTime = time;//�N���オ�蓮�쒆�͑ҋ@����
		if(Time::dt(time,mLastUpdateTime) > 1)//�N���オ�蓮���1�b�ҋ@����
		{
			//����ԂɑJ��
			mLastUpdateTime = time;
			mCurStep = STEP_PARA_JUDGE;
			gCameraCapture.startWarming();
		}
		break;
	case STEP_PARA_JUDGE:
		//���[�o�[���N�����I�������C�p���V���[�g���m���s���C���݂���ꍇ�͉���s���ɑJ�ڂ���
		if(Time::dt(time,mLastUpdateTime) > 2)
		{
			//�p���V���[�g�̑��݃`�F�b�N���s��
			IplImage* pImage = gCameraCapture.getFrame();
			if(gImageProc.isParaExist(pImage))
			{
				//��𓮍�ɑJ��
				mCurStep = STEP_PARA_DODGE;
				mLastUpdateTime = time;
				gTurningState.setRunMode(true);
				Debug::print(LOG_SUMMARY, "Para check: Found!!\r\n");
			}else
			{
				//�����(�i�r)�ɑJ��
				Debug::print(LOG_SUMMARY, "Para check: Not Found!!\r\n");
				nextState();
			}
			//�p�����m�ɗp�����摜��ۑ�����
			gCameraCapture.save(NULL, pImage);
		}
		break;
	case STEP_PARA_DODGE:
		if(!gTurningState.isActive())
		{
			Debug::print(LOG_SUMMARY, "Para check: Turn Finished!\r\n");
			nextState();
		}
	};
}

void Separating::nextState()
{
	//�u�U�[�炵�Ƃ�
	gBuzzer.start(100);

	//���̏�Ԃ�ݒ�
	gNavigatingState.setRunMode(true);
	
	Debug::print(LOG_SUMMARY, "Separating Finished!\r\n");
}

Separating::Separating() : mCurServoState(false),mServoCount(0)
{
	setName("separating");
	setPriority(TASK_PRIORITY_SEQUENCE,TASK_INTERVAL_SEQUENCE);
}
Separating::~Separating()
{
}
//�S�[���ւ̈ړ���
bool Navigating::onInit(const struct timespec& time)
{
	Debug::print(LOG_SUMMARY, "Navigating...\r\n");

	//�K�v�ȃ^�X�N���g�p�ł���悤�ɂ���
	TaskManager::getInstance()->setRunMode(false);
	setRunMode(true);
	gBuzzer.setRunMode(true);
	gGyroSensor.setRunMode(true);
	gGPSSensor.setRunMode(true);
	gSerialCommand.setRunMode(true);
	gMotorDrive.setRunMode(true);
	gCameraCapture.setRunMode(true);
	gPredictingState.setRunMode(true);

	mLastCheckTime = time;
	mLastPos.clear();

	return true;
}
void Navigating::onUpdate(const struct timespec& time)
{
	VECTOR3 currentPos;

	//�S�[�����ݒ肳��Ă��邩�m�F
	if(!mIsGoalPos)
	{
		//�S�[�����ݒ肳��Ă��Ȃ����߈ړ��ł��Ȃ�
		Debug::print(LOG_SUMMARY, "NAVIGATING : Please set goal!\r\n");
		gMotorDrive.drive(0,0);
		nextState();
		return;
	}

	bool isNewData = gGPSSensor.isNewPos();
	//�V�����ʒu���擾�ł��Ȃ���Ώ�����Ԃ�
	if(!gGPSSensor.get(currentPos))return;


	//�V�������W�ł���΃o�b�t�@�ɒǉ�
	if(isNewData && finite(currentPos.x) && finite(currentPos.y) && finite(currentPos.z))
	{
		//�ŏ��̍��W���擾������ړ����J�n����
		if(mLastPos.empty())
		{
			Debug::print(LOG_SUMMARY, "Starting navigation...\r\n");
			gMotorDrive.startPID(0 ,MOTOR_MAX_POWER);
			mLastCheckTime = time;
		}
		mLastPos.push_back(currentPos);
	}	

	//�S�[���Ƃ̋������m�F
	double distance = VECTOR3::calcDistanceXY(currentPos,mGoalPos);
	if(distance < NAVIGATING_GOAL_DISTANCE_THRESHOLD)
	{
		//�S�[������
		gMotorDrive.drive(0,0);
		nextState();
		return;
	}

	//���b�����Ă��Ȃ���Ώ�����Ԃ�
	if(Time::dt(time,mLastCheckTime) < NAVIGATING_DIRECTION_UPDATE_INTERVAL)return;
	mLastCheckTime = time;

	IplImage* pImage = gCameraCapture.getFrame();
	gCameraCapture.save(NULL,pImage);

	if(gAvoidingState.isActive())
	{
		//�Q���
	}else if(isStuck())//�X�^�b�N����
	{
		Debug::print(LOG_SUMMARY, "NAVIGATING: STUCK detected at (%f %f)\r\n",currentPos.x,currentPos.y);
		gEscapingState.setRunMode(true);
	}else
	{
		if(gEscapingState.isActive())//�E�o���[�h������������
		{
			//���[�o�[���Ђ�����Ԃ��Ă���\�������邽�߁A���΂炭�O�i����
			gMotorDrive.startPID(0 ,MOTOR_MAX_POWER);
			gEscapingState.setRunMode(false);
		}else
		{
			//�ʏ�̃i�r�Q�[�V����
			if(mLastPos.size() < 2)return;//�ߋ��̍��W��1�ȏ�(���݂̍��W�����킹��2�ȏ�)�Ȃ���Ώ�����Ԃ�(�i�s��������s�\)
			navigationMove(distance);//�ߋ��̍��W����i�s������ύX����
		}
	}

	//���W�f�[�^���ЂƂc���č폜
	currentPos = mLastPos.back();
	mLastPos.clear();
	mLastPos.push_back(currentPos);
}
bool Navigating::isStuck() const
{
	//�X�^�b�N����
	VECTOR3 averagePos1,averagePos2;
	unsigned int i,border;
	std::list<VECTOR3>::const_iterator it = mLastPos.begin();
	for(i = 0;i < mLastPos.size() / 2;++i)
	{
		averagePos1 += *it;
		it++;
	}
	averagePos1 /= border = i;

	for(;i < mLastPos.size();++i)
	{
		averagePos2 += *it;
		it++;
	}
	averagePos2 /= i - border;

	return VECTOR3::calcDistanceXY(averagePos1,averagePos2) < NAVIGATING_STUCK_JUDGEMENT_THRESHOLD;//�ړ��ʂ�臒l�ȉ��Ȃ�X�^�b�N�Ɣ���
}
void Navigating::navigationMove(double distance) const
{
	//�ߋ��̍��W�̕��ϒl���v�Z����
	VECTOR3 averagePos;
	std::list<VECTOR3>::const_iterator it = mLastPos.begin();
	while(it != mLastPos.end())
	{
		averagePos += *it;
		++it;
	}
	averagePos -= mLastPos.back();
	averagePos /= mLastPos.size() - 1;

	//�V�����p�x���v�Z
	VECTOR3 currentPos = mLastPos.back();
	double currentDirection = -VECTOR3::calcAngleXY(averagePos,currentPos);
	double newDirection = -VECTOR3::calcAngleXY(currentPos,mGoalPos);
	double deltaDirection = GyroSensor::normalize(newDirection - currentDirection);
	deltaDirection = std::max(std::min(deltaDirection,NAVIGATING_MAX_DELTA_DIRECTION),-1 * NAVIGATING_MAX_DELTA_DIRECTION);

	//�V�������x���v�Z
	double speed = MOTOR_MAX_POWER;
	if(distance < NAVIGATING_GOAL_APPROACH_DISTANCE_THRESHOLD)speed *= NAVIGATING_GOAL_APPROACH_POWER_RATE;//�ڋ߂����瑬�x�𗎂Ƃ�

	Debug::print(LOG_SUMMARY, "NAVIGATING: Last %d samples (%f %f) Current(%f %f)\r\n",mLastPos.size(),averagePos.x,averagePos.y,currentPos.x,currentPos.y);
	Debug::print(LOG_SUMMARY, "distance = %f (m)  delta angle = %f(%s)\r\n",distance * DEGREE_2_METER,deltaDirection,deltaDirection > 0 ? "LEFT" : "RIGHT");

	//�����Ƒ��x��ύX
	gMotorDrive.drivePID(deltaDirection ,speed);
}
bool Navigating::onCommand(const std::vector<std::string> args)
{
	if(args.size() == 1)
	{
		if(mIsGoalPos)Debug::print(LOG_SUMMARY ,"Current Goal (%f %f)\r\n",mGoalPos.x,mGoalPos.y);
		else Debug::print(LOG_SUMMARY ,"NO Goal\r\n");
	}
	if(args.size() == 2)
	{
		if(args[1].compare("here") == 0)
		{
			VECTOR3 pos;
			if(!gGPSSensor.get(pos))
			{
				Debug::print(LOG_SUMMARY, "Unable to get current position!\r\n");
				return true;
			}

			setGoal(pos);
			return true;
		}
	}
	if(args.size() == 3)
	{
		VECTOR3 pos;
		pos.x = atof(args[1].c_str());
		pos.y = atof(args[2].c_str());

		setGoal(pos);
		return true;
	}
	Debug::print(LOG_PRINT, "navigating                 : get goal\r\n\
navigating [pos x] [pos y] : set goal at specified position\r\n\
navigating here            : set goal at current position\r\n");
	return true;
}
//���̏�ԂɈڍs
void Navigating::nextState()
{
	gBuzzer.start(1000);

	//���̏�Ԃ�ݒ�
	gTestingState.setRunMode(true);
	gPictureTakingState.setRunMode(true);
	
	Debug::print(LOG_SUMMARY, "Goal!\r\n");
}
void Navigating::setGoal(const VECTOR3& pos)
{
	mIsGoalPos = true;
	mGoalPos = pos;
	Debug::print(LOG_SUMMARY, "Set Goal ( %f %f )\r\n",mGoalPos.x,mGoalPos.y);
}
Navigating::Navigating() : mGoalPos(),  mIsGoalPos(false), mLastPos()
{
	setName("navigating");
	setPriority(TASK_PRIORITY_SEQUENCE,TASK_INTERVAL_SEQUENCE);
}
Navigating::~Navigating()
{
}

bool WadachiPredicting::onInit(const struct timespec& time)
{
	mLastUpdateTime = time;
	gCameraCapture.startWarming();

	return true;
}
void WadachiPredicting::onUpdate(const struct timespec& time)
{
	if(Time::dt(time,mLastUpdateTime) < 5 || gAvoidingState.isActive())return;
	mLastUpdateTime = time;

	//�V�����摜���擾���ď���
	IplImage* pImage = gCameraCapture.getFrame();
	gCameraCapture.save(NULL,pImage,true);
	if(gImageProc.isWadachiExist(pImage))
	{
		//�Q�����O���m����
		if(mIsAvoidingEnable)
		{
			//�Q������L��
			gAvoidingState.setRunMode(true);
		}
	}
	gCameraCapture.startWarming();
}
bool WadachiPredicting::onCommand(const std::vector<std::string> args)
{
	if(args.size() == 2)
	{
		if(args[1].compare("enable") == 0)
		{
			mIsAvoidingEnable = true;
			return true;
		}
		if(args[1].compare("disable") == 0)
		{
			mIsAvoidingEnable = false;
			return true;
		}
	}
	Debug::print(LOG_SUMMARY, "predicting [enable/disable]  : switch avoiding mode\r\n");
	return false;
}
WadachiPredicting::WadachiPredicting() : mIsAvoidingEnable(false)
{
	setName("predicting");
	setPriority(TASK_PRIORITY_SEQUENCE,TASK_INTERVAL_SEQUENCE);
}
WadachiPredicting::~WadachiPredicting()
{
}

bool Escaping::onInit(const struct timespec& time)
{
	mLastUpdateTime = time;
	mCurStep = STEP_BACKWARD;
	gMotorDrive.drive(-100,-100);
	gCameraCapture.setRunMode(true);
	gGyroSensor.setRunMode(true);
	mEscapingTriedCount = 0;
	return true;
}
void Escaping::onClean()
{
	gWakingState.setRunMode(false);
	gTurningState.setRunMode(false);
}
void Escaping::onUpdate(const struct timespec& time)
{
	const static unsigned int ESCAPING_MAX_CAMERA_ESCAPING_COUNT = 20;
	const static unsigned int ESCAPING_MAX_RANDOM_ESCAPING_COUNT = 20;
	switch(mCurStep)
	{
	case STEP_BACKWARD:
		//�o�b�N���s��
		if(Time::dt(time,mLastUpdateTime) >= 2)
		{
			Debug::print(LOG_SUMMARY, "Escaping: Backward finished!\r\n");
			mCurStep = STEP_AFTER_BACKWARD;
			mLastUpdateTime = time;
			gMotorDrive.drive(0,0);
			gCameraCapture.startWarming();
		}
		break;
	case STEP_AFTER_BACKWARD:
		//�ċN���h�~�̂��ߑҋ@
		if(Time::dt(time,mLastUpdateTime) >= 3)
		{
			if(mEscapingTriedCount > ESCAPING_MAX_CAMERA_ESCAPING_COUNT)
			{
				//�����_���ڍs
				Debug::print(LOG_SUMMARY, "Escaping: aborting camera escape!\r\n");
				mEscapingTriedCount = 0;
				mCurStep = STEP_RANDOM;
				mCurRandomStep = RANDOM_STEP_FORWARD;
				break;
			}
			mCurStep = STEP_PRE_CAMERA;
			mLastUpdateTime = time;
			//�N���オ�蓮����s��
			IplImage* pImage = gCameraCapture.getFrame();
			gCameraCapture.save(NULL,pImage);
			if(gImageProc.isSky(pImage))gWakingState.setRunMode(true);
		}
		break;
	case STEP_PRE_CAMERA:
		//�摜�B�e�p�ɋN���オ�蓮����s���A���b�ҋ@����
		if(gWakingState.isActive())mLastUpdateTime = time;//�N���オ�蓮�쒆�͑ҋ@����
		if(Time::dt(time,mLastUpdateTime) > 2)//�N���オ�芮����A��莞�Ԃ��o�߂��Ă�����
		{
			Debug::print(LOG_SUMMARY, "Escaping: camera warming...\r\n");
			//�摜�B�e������s��
			mCurStep = STEP_CAMERA;
			mLastUpdateTime = time;
			gMotorDrive.drive(0,0);
			gCameraCapture.startWarming();
		}
		break;
	case STEP_CAMERA:
		//�摜�������s���A����̍s�������肷��
		if(Time::dt(time,mLastUpdateTime) >= 2)
		{
			Debug::print(LOG_SUMMARY, "Escaping: taking picture!\r\n");
			mLastUpdateTime = time;
			IplImage* pImage = gCameraCapture.getFrame();
			stuckMoveCamera(pImage);
			gCameraCapture.save(NULL,pImage);
			++mEscapingTriedCount;
		}
		break;
	case STEP_CAMERA_TURN:
		//�摜�����̌��ʂɉ����ĉ�]����
		if(!gTurningState.isActive())
		{
			gCameraCapture.startWarming();
			mCurStep = STEP_CAMERA_FORWARD;
			gMotorDrive.startPID(0,100);
			mLastUpdateTime = time;
		}
		break;
	case STEP_CAMERA_FORWARD:
		//�摜�����̌��ʁA���i����K�v���������ꍇ
		if(Time::dt(time,mLastUpdateTime) >= 10)
		{
			gMotorDrive.drive(-100,-100);
			mCurStep = STEP_BACKWARD;
			mLastUpdateTime = time;
		}
		break;
	case STEP_RANDOM:
		//�����_������
		if(Time::dt(time,mLastUpdateTime) >= 5)
		{
			++mEscapingTriedCount;
			if(mEscapingTriedCount > ESCAPING_MAX_RANDOM_ESCAPING_COUNT)
			{
				//�����_���ڍs
				mEscapingTriedCount = 0;
				mCurStep = STEP_BACKWARD;
				break;
			}
			stuckMoveRandom();
			mLastUpdateTime = time;

		}
		break;
	}
}
void Escaping::stuckMoveRandom()
{
	switch(mCurRandomStep)
	{
	case RANDOM_STEP_BACKWARD:
		//�o�b�N���s��
		Debug::print(LOG_SUMMARY, "Escaping(random): backward\r\n");
		mCurRandomStep = RANDOM_STEP_TURN;
		gMotorDrive.drive(100,-100);
		break;
	case RANDOM_STEP_TURN:
		//���̏��]���s��
		Debug::print(LOG_SUMMARY, "Escaping(random): turning\r\n");
		mCurRandomStep = RANDOM_STEP_FORWARD;
		gMotorDrive.drive(100,100);
		break;
	case RANDOM_STEP_FORWARD:
		//�O�i���s��
		Debug::print(LOG_SUMMARY, "Escaping(random): forward\r\n");
		mCurRandomStep = RANDOM_STEP_BACKWARD;
		gMotorDrive.drive(-100,-100);
		break;
	}
}
void Escaping::stuckMoveCamera(IplImage* pImage)
{
	switch(gImageProc.wadachiExiting(pImage)){
		case -1:
			Debug::print(LOG_SUMMARY, "Wadachi kaihi:Turn Left\r\n");
			gTurningState.setRunMode(true);
			gTurningState.setDirection(true);
			mCurStep = STEP_CAMERA_TURN;
			break;
		case 1:
			Debug::print(LOG_SUMMARY, "Wadachi kaihi:Turn Right\r\n");
			gTurningState.setRunMode(true);
			gTurningState.setDirection(false);
			mCurStep = STEP_CAMERA_TURN;
			break;
		case 0:
            Debug::print(LOG_SUMMARY, "Wadachi kaihi:Go Straight\r\n");
			gMotorDrive.startPID(0,100);
			mCurStep = STEP_CAMERA_FORWARD;
			break;
		default://�J�����g���Ȃ�����
			mCurStep = STEP_RANDOM;
			mCurRandomStep = RANDOM_STEP_FORWARD;
			break;
		
	}
}
Escaping::Escaping()
{
	setName("escaping");
	setPriority(TASK_PRIORITY_SEQUENCE,TASK_INTERVAL_SEQUENCE);
}
Escaping::~Escaping()
{
}
bool EscapingRandom::onInit(const struct timespec& time)
{
	mLastUpdateTime = time;
	mCurStep = STEP_BACKWARD;
	gMotorDrive.drive(-100,-100);
	return true;
}
void EscapingRandom::onUpdate(const struct timespec& time)
{
	switch(mCurStep)
	{
	case STEP_BACKWARD:
		//�o�b�N���s��
		if(Time::dt(time,mLastUpdateTime) >= 3)
		{
			mCurStep = STEP_TURN;
			mLastUpdateTime = time;
			gMotorDrive.drive(100,-100);
		}
		break;
	case STEP_TURN:
		//���̏��]���s��
		if(Time::dt(time,mLastUpdateTime) >= 3)
		{
			mCurStep = STEP_FORWARD;
			mLastUpdateTime = time;
			gMotorDrive.drive(100,100);
		}
		break;
	case STEP_FORWARD:
		//�O�i���s��
		if(Time::dt(time,mLastUpdateTime) >= 3)
		{
			mCurStep = STEP_BACKWARD;
			mLastUpdateTime = time;
			gMotorDrive.drive(-100,-100);
		}
		break;
	}
}
EscapingRandom::EscapingRandom()
{
	setName("random");
	setPriority(TASK_PRIORITY_SEQUENCE,TASK_INTERVAL_SEQUENCE);
}
EscapingRandom::~EscapingRandom()
{
}
bool Waking::onInit(const struct timespec& time)
{
	mLastUpdateTime = time;
	mCurStep = STEP_START;
	gMotorDrive.setRunMode(true);
	gMotorDrive.drive(50,50);
	gGyroSensor.setRunMode(true);
	mAngleOnBegin = gGyroSensor.getRvx();
	mWakeRetryCount = 0;
	return true;
}
void Waking::onClean()
{
	gMotorDrive.drive(0,0);
}
void Waking::onUpdate(const struct timespec& time)
{
	double power;
	const static double WAKING_THRESHOLD = 200;
	switch(mCurStep)//�N���オ��J�n�����m���ꂽ�ꍇ
	{
	case STEP_STOP:
		if(Time::dt(time,mLastUpdateTime) > 2)//2�b�܂킵�Ă����n�����m����Ȃ��ꍇ�͂�����߂�
		{
			Debug::print(LOG_SUMMARY, "Waking Timeout : unable to land\r\n");
			setRunMode(false);
		}
		if(abs(gGyroSensor.getRvx()) < WAKING_THRESHOLD)//�p���x�����ȉ��ɂȂ����璅�n�Ɣ���
		{
			Debug::print(LOG_SUMMARY, "Waking Landed!\r\n");
			mLastUpdateTime = time;
			mCurStep = STEP_VERIFY;
			gMotorDrive.drive(0,0);
			gCameraCapture.startWarming();
		}

		//��]�����p�x�ɉ����ă��[�^�̏o�͂�ω�������
		power = std::min(0,std::max(100,MOTOR_MAX_POWER - abs(gGyroSensor.getRvx() - mAngleOnBegin) / 130 + 50));
		gMotorDrive.drive(power,power);
		break;

	case STEP_START:
		if(Time::dt(time,mLastUpdateTime) > 0.5)//��莞�ԉ�]�����m����Ȃ��ꍇ����]�s�\�Ɣ��f
		{
			Debug::print(LOG_SUMMARY, "Waking Timeout : unable to spin\r\n");
			mLastUpdateTime = time;
			mCurStep = STEP_VERIFY;
			gMotorDrive.drive(0,0);
			gCameraCapture.startWarming();
		}
		if(abs(gGyroSensor.getRvx()) > WAKING_THRESHOLD)//��]�����m���ꂽ�ꍇ���N���オ��J�n�����Ɣ��f
		{
			Debug::print(LOG_SUMMARY, "Waking Detected Rotation!\r\n");
			mLastUpdateTime = time;
			mCurStep = STEP_STOP;
		}
		break;

	case STEP_VERIFY:
		//�N���オ�肪�����������ۂ����J�����摜�Ō���
		if(Time::dt(time,mLastUpdateTime) > 1)
		{
			IplImage* pCaptureFrame = gCameraCapture.getFrame();
			gCameraCapture.save(NULL,pCaptureFrame);
			if(gImageProc.isSky(pCaptureFrame))
			{
				mLastUpdateTime = time;
				mCurStep = STEP_START;
				mAngleOnBegin = gGyroSensor.getRvx();
				gMotorDrive.drive(50,50);

				if(++mWakeRetryCount > WAKING_RETRY_COUNT)
				{
					Debug::print(LOG_SUMMARY, "Waking Failed!\r\n");
					setRunMode(false);
					return;
				}
				Debug::print(LOG_SUMMARY, "Waking will be retried (%d / %d)\r\n",mWakeRetryCount,WAKING_RETRY_COUNT);
			}else
			{
				Debug::print(LOG_SUMMARY, "Waking Successed!\r\n");
				setRunMode(false);
			}
		}
		break;
	}
}

Waking::Waking() : mWakeRetryCount(0)
{
	setName("waking");
	setPriority(TASK_PRIORITY_SEQUENCE,TASK_INTERVAL_SEQUENCE);
}
Waking::~Waking()
{
}

bool Turning::onInit(const struct timespec& time)
{
	mTurnPower = 0;
	gGyroSensor.setRunMode(true);
	mAngle = gGyroSensor.getRz();
	mLastUpdateTime = time;
	return true;
}

void Turning::onUpdate(const struct timespec& time)
{
	double turnedAngle = abs(GyroSensor::normalize(gGyroSensor.getRz() - mAngle));
	if(Time::dt(time,mLastUpdateTime) >= 5 || turnedAngle > 15)
	{
		Debug::print(LOG_SUMMARY, "Turning: Detected turning\r\n");
		gMotorDrive.drive(0,0);
		setRunMode(false);
	}else
	{
		if(mIsTurningLeft)gMotorDrive.drive(-mTurnPower,mTurnPower);
		else gMotorDrive.drive(mTurnPower,-mTurnPower);
		if(turnedAngle < 5)mTurnPower += 0.1;
	}
}

void Turning::setDirection(bool left)
{
	mIsTurningLeft = left;
}
Turning::Turning()
{
	setName("turning");
	setPriority(TASK_PRIORITY_SEQUENCE,TASK_INTERVAL_SEQUENCE);
}
Turning::~Turning()
{
}

bool Avoiding::onInit(const struct timespec& time)
{
	mLastUpdateTime = time;
	if(!gEscapingState.isActive())gMotorDrive.drive(0,100);
	mAngle = gGyroSensor.getRz();
	mCurStep = STEP_TURN;
	return true;
}
void Avoiding::onUpdate(const struct timespec& time)
{
	if(gEscapingState.isActive())
	{
		Debug::print(LOG_SUMMARY, "Avoiding: Escaping is already running. Avoiding Canceled!\r\n");
		setRunMode(false);
	}
	switch(mCurStep)
	{
	case STEP_TURN:
		if(Time::dt(time,mLastUpdateTime) > 5 || abs(GyroSensor::normalize(gGyroSensor.getRz() - mAngle)) > 90)
		{
			Debug::print(LOG_SUMMARY, "Avoiding: forwarding\r\n");
			mLastUpdateTime = time;
			gMotorDrive.startPID(0,MOTOR_MAX_POWER);
			mCurStep = STEP_FORWARD;
		}
		break;
	case STEP_FORWARD:
		if(Time::dt(time,mLastUpdateTime) > 5)
		{
			Debug::print(LOG_SUMMARY, "Avoiding: finished\r\n");
			setRunMode(false);
		}
		break;
	}
}
Avoiding::Avoiding()
{
	setName("avoiding");
	setPriority(TASK_PRIORITY_SEQUENCE,TASK_INTERVAL_SEQUENCE);
}
Avoiding::~Avoiding()
{
}

bool PictureTaking::onInit(const struct timespec& time)
{
	mLastUpdateTime = time;
	gCameraCapture.setRunMode(true);
	gBuzzer.setRunMode(true);
	gWakingState.setRunMode(true);
	mStepCount = 0;
	return true;
}
void PictureTaking::onUpdate(const struct timespec& time)
{
	if(gWakingState.isActive())return;
	if(Time::dt(time,mLastUpdateTime) > 1)
	{
		mLastUpdateTime = time;
		++mStepCount;
		gBuzzer.start(mStepCount > 25 ? 30 : 10);

		if(mStepCount == 25)
		{
			gCameraCapture.startWarming();
		}
		if(mStepCount >= 30)
		{
			Debug::print(LOG_SUMMARY, "Say cheese!\r\n");
			setRunMode(false);
			gBuzzer.start(300);
			gCameraCapture.save();
		}
	}
}

PictureTaking::PictureTaking() : mStepCount(0)
{
	setName("kinen");
	setPriority(TASK_PRIORITY_SEQUENCE,TASK_INTERVAL_SEQUENCE);
}
PictureTaking::~PictureTaking()
{
}