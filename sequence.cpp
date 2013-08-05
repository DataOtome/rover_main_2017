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

Testing gTestingState;
Waiting gWaitingState;
Falling gFallingState;
Separating gSeparatingState;
Navigating gNavigatingState;

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
				pTask->setRunMode(true);
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

	return true;
}
void Falling::onUpdate(const struct timespec& time)
{
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
		Debug::print(LOG_SUMMARY, "Pressure Count %d / %d\r\n",mContinuousPressureCount,FALLING_PRESSURE_COUNT);
	}else mContinuousPressureCount = 0;

	//�W���C���̔����Ԃ�\��
	Debug::print(LOG_SUMMARY, "Gyro Count     %d / %d\r\n",mCoutinuousGyroCount,FALLING_GYRO_COUNT);

	//�J�E���g�񐔂����ȏ�Ȃ玟�̏�ԂɈڍs
	if(mContinuousPressureCount >= FALLING_PRESSURE_COUNT && mCoutinuousGyroCount >= FALLING_GYRO_COUNT)
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
Falling::Falling() : mLastPressure(0),mContinuousPressureCount(0),mCoutinuousGyroCount(0)
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
	//if(!gGPSSensor.get(currentPos))return;

	//�ŏ��̍��W���擾������ړ����J�n����
	if(mLastPos.empty())
	{
		Debug::print(LOG_SUMMARY, "Starting navigation...\r\n");
		gMotorDrive.startPID(0 ,MOTOR_MAX_POWER);
		mLastCheckTime = time;
	}

	//�V�������W�ł���΃o�b�t�@�ɒǉ�
	if(isNewData && finite(currentPos.x) && finite(currentPos.y) && finite(currentPos.z))
	{
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

	//�X�^�b�N����
	if(isStuck())
	{
		Debug::print(LOG_SUMMARY, "NAVIGATING: STUCK detected at (%f %f)\r\n",currentPos.x,currentPos.y);
		gBuzzer.start(10);
		mLastStuckMoveUpdateTime.tv_sec = 0;
		mLastStuckMoveUpdateTime.tv_nsec = 0;

		if(mIsStucked == STUCK_NONE)mIsStucked = STUCK_BACKWORD;

		//�X�^�b�N���Ă���ꍇ�̓X�^�b�N���̓�������s
		switch(mIsStucked)
		{
		case STUCK_RANDOM:
			stuckMoveRandom();
			Debug::print(LOG_SUMMARY, "Random kaihi\r\n");
			break;
		case STUCK_CAMERA:
			stuckMoveCamera();
			Debug::print(LOG_SUMMARY, "Camera kaihi\r\n");
			break;
		case STUCK_BACKWORD:
			gMotorDrive.drive(-100,-100);
			mIsStucked = STUCK_FORWORD;
			Debug::print(LOG_SUMMARY, "kaihi junbi 1\r\n");
			break;
		case STUCK_FORWORD:
			gMotorDrive.drive(30,30);
			mIsStucked = STUCK_CAMERA;
			Debug::print(LOG_SUMMARY, "kaihi junbi 2\r\n");
			break;
		default:
			break;
		};
	}else
	{
		if(mIsStucked != STUCK_NONE)
		{
			//���[�o�[���Ђ�����Ԃ��Ă���\�������邽�߁A���΂炭�O�i����
			gMotorDrive.startPID(0 ,MOTOR_MAX_POWER);
			mIsStucked = STUCK_NONE;
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
	double averageVel = 0;
	VECTOR3 lastPos = mLastPos.front();
	std::list<VECTOR3>::const_iterator it = mLastPos.begin();
	while(it != mLastPos.end())
	{
		averageVel += VECTOR3::calcDistanceXY(*it,lastPos);
		lastPos = *it;
		++it;
	}

	return averageVel < NAVIGATING_STUCK_JUDGEMENT_THRESHOLD;
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
void Navigating::stuckMoveRandom()
{
	//�i�s�����������_���ŕύX
	gMotorDrive.drive(100 * (rand() % 2 ? 1 : -1), 100 * (rand() % 2 ? 1 : -1));
}
void Navigating::stuckMoveCamera()
{
	gMotorDrive.drive(30,30);
	const static int DIV_NUM = 3,WIDTH = 320,HEIGHT = 240;
	CvSize size = cvSize(WIDTH,HEIGHT);
	CvCapture *pCapture = cvCreateCameraCapture(0);
	IplImage *src_img, *gray_img, *dst_img1, *tmp_img;
	double risk[DIV_NUM];

	if(pCapture == NULL)
	{
		mIsStucked = STUCK_RANDOM;
		return;
	}
	cvSetCaptureProperty(pCapture, CV_CAP_PROP_FRAME_WIDTH, WIDTH); //�B�e�T�C�Y���w��
	cvSetCaptureProperty(pCapture, CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);

	src_img = cvQueryFrame(pCapture);
	gray_img = cvCreateImage(size, IPL_DEPTH_8U, 1);
	cvCvtColor(src_img, gray_img, CV_BGR2GRAY);
	cvRectangle(gray_img, cvPoint(0, 0),cvPoint(WIDTH, HEIGHT / 3),cvScalar(0), CV_FILLED, CV_AA);

	// Median�t�B���^
	cvSmooth (gray_img, gray_img, CV_MEDIAN, 5, 0, 0, 0);
		
	tmp_img = cvCreateImage(size, IPL_DEPTH_16S, 1);
	dst_img1 = cvCreateImage(size, IPL_DEPTH_8U, 1);
		
	// Sobel�t�B���^X����
	cvSobel(gray_img, tmp_img, 1, 0, 3);
	cvConvertScaleAbs (tmp_img, dst_img1);
	cvThreshold (dst_img1, dst_img1, 50, 255, CV_THRESH_BINARY);

	//Sum
	int width = src_img->width / DIV_NUM;
	double risksum = 0;
	int i;
	for(i = 0;i < DIV_NUM;++i)
	{
		cvSetImageROI(dst_img1, cvRect(width * i,0,width,src_img->height));//Set image part
		risksum += risk[i] = sum(cv::cvarrToMat(dst_img1))[0];
		cvResetImageROI(dst_img1);//Reset image part (normal)
	}

	//Draw graph
	for(i = 0;i < DIV_NUM;++i){
		cvRectangle(dst_img1, cvPoint(width * i,src_img->height - risk[i] / risksum * src_img->height),cvPoint(width * (i + 1),src_img->height),cvScalar(255), 2, CV_AA);
	}

	int min_id = 0;
	for(int i=1; i<3; ++i){
		if(risk[min_id] > risk[i]){
			min_id = i;
		}
	}

	switch(min_id){
		case 0:
			Debug::print(LOG_SUMMARY, "Wadachi kaihi:Turn Left\r\n");
			gMotorDrive.drive(60, 100);
			break;
		case 1:
			Debug::print(LOG_SUMMARY, "Wadachi kaihi:Go Straight\r\n");
			gMotorDrive.drive(100, 100);
			break;
		case 2:
			Debug::print(LOG_SUMMARY, "Wadachi kaihi:Turn Right\r\n");
			gMotorDrive.drive(100, 60);
			break;
		default:
			break;
	}
	cvReleaseImage (&dst_img1);
	cvReleaseImage (&tmp_img);
	cvReleaseCapture(&pCapture);

	mIsStucked = STUCK_RANDOM;
}
bool Navigating::onCommand(const std::vector<std::string> args)
{
	if(args.size() == 1)
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
	if(args.size() == 3)
	{
		VECTOR3 pos;
		pos.x = atof(args[1].c_str());
		pos.y = atof(args[2].c_str());

		setGoal(pos);
		return true;
	}
	Debug::print(LOG_PRINT, "navigating [pos x] [pos y]\r\n");
	return true;
}
//���̏�ԂɈڍs
void Navigating::nextState()
{
	gBuzzer.start(1000);

	//���̏�Ԃ�ݒ�
	gTestingState.setRunMode(true);
	
	Debug::print(LOG_SUMMARY, "Goal!\r\n");
}
void Navigating::setGoal(const VECTOR3& pos)
{
	mIsGoalPos = true;
	mGoalPos = pos;
	Debug::print(LOG_SUMMARY, "Set Goal ( %f %f )\r\n",mGoalPos.x,mGoalPos.y);
}
Navigating::Navigating() : mGoalPos(),  mIsGoalPos(false),mIsStucked(STUCK_NONE), mLastPos()
{
	setName("navigating");
	setPriority(TASK_PRIORITY_SEQUENCE,TASK_INTERVAL_SEQUENCE);
}
Navigating::~Navigating()
{
}
