#include <wiringPi.h>
#include <softPwm.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include "utils.h"
#include "motor.h"
#include "sensor.h"

MotorDrive gMotorDrive;

bool Motor::init(int powPin, int revPin)
{
	//�s���ԍ����m�F
    VERIFY(powPin < 0 || revPin < 0);

	//�s����������
    mPowerPin = powPin;
    mReversePin = revPin;
    pinMode(mPowerPin, OUTPUT);
    if(softPwmCreate(mPowerPin ,0,100) != 0)
	{
		Debug::print(LOG_SUMMARY,"Failed to initialize soft-PWM\r\n");
		return false;
	}
    pinMode(mReversePin, OUTPUT);
    digitalWrite(mReversePin, LOW);

	//���݂̏o�͂�ێ�
    mCurPower = 0;
    return true;
}
void Motor::update(double elapsedSeconds)
{
	if(fabs(mCurPower - mTargetPower) != 0)//�ڕW�o�͂ƌ��ݏo�͂ɍ�������ꍇ
	{
		//�Ȃ߂炩�Ƀ��[�^�o�͂�ω�������
		double curFrameTarget = mTargetPower;//���̌Ăяo���Őݒ肷�郂�[�^�[�o��

		double maxMotorPowerChange = MOTOR_MAX_POWER_CHANGE * mCoeff;

		//���[�^�o�͕ω��ʂ𐧌�
		if(fabs(mTargetPower - mCurPower) > maxMotorPowerChange)
		{
			curFrameTarget = mCurPower;
			curFrameTarget += ((mTargetPower > mCurPower) ? maxMotorPowerChange : -maxMotorPowerChange);
			Debug::print(LOG_DETAIL,"MOTOR power Limitation %f %f(%d) \r\n",mCurPower,curFrameTarget,mTargetPower);
		}

		//�V����power�����Ƃ�pin�̏�Ԃ�ݒ肷��
		if(curFrameTarget > 0 && mCurPower <= 0)digitalWrite(mReversePin, HIGH);
		else if(curFrameTarget < 0 && mCurPower >= 0)digitalWrite(mReversePin, LOW);
		mCurPower = curFrameTarget;
		softPwmWrite(mPowerPin, fabs(mCurPower));
	}
}
void Motor::clean()
{
	if(mPowerPin >= 0)softPwmWrite(mPowerPin, 0);
	if(mReversePin >= 0)digitalWrite(mReversePin, LOW);
	mCurPower = 0;
}
void Motor::set(int power)
{
	//�l�͈̔͂��`�F�b�N���A�������͈͂Ɋۂ߂�
	if(power > MOTOR_MAX_POWER)power = MOTOR_MAX_POWER;
	else if(power < -MOTOR_MAX_POWER)power = -MOTOR_MAX_POWER;
	
	//�ڕW�o�͂�ݒ�
	mTargetPower = power;
}
int Motor::getPower()
{
    return mCurPower;
}
void Motor::setCoeff(double coeff)
{
	mCoeff = coeff;
}
Motor::Motor() : mPowerPin(-1), mReversePin(-1), mCurPower(0), mTargetPower(0), mCoeff(1)
{
}
Motor::~Motor()
{
}
MotorEncoder* MotorEncoder::getInstance()
{
	static MotorEncoder singleton;
	return &singleton;
}
void MotorEncoder::pulseLCallback()
{
	MotorEncoder::getInstance()->mPulseCountL++;
}
void MotorEncoder::pulseRCallback()
{
	MotorEncoder::getInstance()->mPulseCountR++;
}
bool MotorEncoder::init()
{
	mPulseCountL = mPulseCountR = 0;

	//�s���̃p���X���Ď�����
	if(wiringPiISR(mEncoderPinL, INT_EDGE_RISING, pulseLCallback) == -1 || wiringPiISR(mEncoderPinR, INT_EDGE_RISING, pulseRCallback) == -1)
	{
		Debug::print(LOG_SUMMARY,"Failed to onInitialize Motor encoder\r\n");
		return false;
	}
	return true;
}
void MotorEncoder::clean()
{
	//�����̃s���̊��荞�݂𖳌��ɂ���
	char command [64];
	sprintf (command, "/usr/local/bin/gpio edge %d none", mEncoderPinL) ;
    system (command) ;
	sprintf (command, "/usr/local/bin/gpio edge %d none", mEncoderPinR) ;
    system (command) ;

	//�X���b�h�������c�邱�Ƃ�h�~���邽��sleep
	delay(100);
}
long long MotorEncoder::getL()
{
	return mPulseCountL;
}
long long MotorEncoder::getR()
{
	return mPulseCountR;
}


MotorEncoder::MotorEncoder() : mEncoderPinL(PIN_PULSE_B),mEncoderPinR(PIN_PULSE_A),mPulseCountL(0),mPulseCountR(0)
{
}
MotorEncoder::~MotorEncoder()
{
}

bool MotorDrive::onInit(const struct timespec& time)
{
	//�W���C�����g���悤�ɐݒ�
	gGyroSensor.setRunMode(true);

	//������
    if(!mMotorR.init(PIN_PWM_A,PIN_INVERT_MOTOR_A) || !mMotorL.init(PIN_PWM_B,PIN_INVERT_MOTOR_B))
	{
		Debug::print(LOG_SUMMARY,"Failed to initialize Motors\r\n");
		return false;
	}
	if(!mpMotorEncoder->init())
	{
		Debug::print(LOG_SUMMARY,"Failed to initialize Motor Encoders\r\n");
		return false;
	}
	if(clock_gettime(CLOCK_MONOTONIC_RAW,&mLastUpdateTime) != 0)
	{
		Debug::print(LOG_SUMMARY,"Unable to get time!\r\n");
	}
	Debug::print(LOG_DETAIL,"MotorDrive is Ready!\r\n");

	mLastUpdateTime = time;
	mAngle = 0;
    return true;
}

void MotorDrive::onClean()
{
	mpMotorEncoder->clean();
	mMotorL.clean();
	mMotorR.clean();
}

void MotorDrive::updatePIDState(double p,double i,double d)
{
	//��������X�V
	mDiff3 = mDiff2;mDiff2 = mDiff1;mDiff1 = gGyroSensor.normalize(gGyroSensor.getRz() - mAngle);

	//����������ɐV�������[�^�[�o�͂�ݒ�(PID)
	double powerDiff = p * (mDiff1 - mDiff2) + i * mDiff1 + d * ((mDiff1 - mDiff2) - (mDiff2 - mDiff3));
	mControlPower += powerDiff;
}
void MotorDrive::updatePIDMove()
{
	updatePIDState(mP,mI,mD);

	//���[�^���x�W����p��
	double drivePowerRatio = (double)mDrivePower / MOTOR_MAX_POWER;//���[�^�o�͂̊���

	//���[�^�̋t��]�������ɕ����]������
	double controlRatio = 1 - fabs(mControlPower);
	if(controlRatio <= 0)controlRatio = 0;

	//���[�^�o�͂�K�p
	if(mControlPower > 0)
	{
		//���ɋȂ���
		mMotorL.set(mRatioL * drivePowerRatio);
		mMotorR.set(-mRatioR * controlRatio * drivePowerRatio);
	}else
	{
		//�E�ɋȂ���
		mMotorL.set(mRatioL * controlRatio * drivePowerRatio);
		mMotorR.set(-mRatioR * drivePowerRatio);
	}
}

void MotorDrive::onUpdate(const struct timespec& time)
{
	//�Ō�̏o�͍X�V����̌o�ߎ��Ԃ��擾
	double dt = Time::dt(time,mLastUpdateTime);
	mLastUpdateTime = time;

	switch(mDriveMode)
	{
	case DRIVE_PID:
		updatePIDMove();
		break;
	default:
		break;
	}
	
	//���[�^�o�͂��X�V
	mMotorL.update(dt);
	mMotorR.update(dt);
}
void MotorDrive::setRatio(int ratioL,int ratioR)
{
	mMotorL.setCoeff((double)(mRatioL = std::max(std::min(ratioL,MOTOR_MAX_POWER),0)) / MOTOR_MAX_POWER);
	mMotorR.setCoeff((double)(mRatioR = std::max(std::min(ratioR,MOTOR_MAX_POWER),0)) / MOTOR_MAX_POWER);
}

void MotorDrive::drive(int powerL, int powerR)
{
	mDriveMode = DRIVE_RATIO;
	Debug::print(LOG_DETAIL,"Motor ratio: %d %d\r\n",powerL,powerR);
    mMotorL.set(mRatioL * powerL / MOTOR_MAX_POWER);
    mMotorR.set(-mRatioR * powerR / MOTOR_MAX_POWER);

	mAngle = 0;
}

void MotorDrive::set(double p,double i,double d)
{
	Debug::print(LOG_SUMMARY, "PID params: %f %f %f\r\n",p,i,d);
	mP = p;
	mI = i;
	mD = d;
}
void MotorDrive::startPID(double angle,int power)
{
	gGyroSensor.setZero();
	drivePID(angle,power);
	mAngle = 0;
	mDiff1 = mDiff2 = mDiff3 = 0;
	mControlPower = 0;
}
void MotorDrive::drivePID(double angle,int power)
{
	mAngle = GyroSensor::normalize(angle + mAngle);
	mDrivePower = std::max(std::min(power,MOTOR_MAX_POWER),0);
	Debug::print(LOG_SUMMARY, "PID is Started (%f, %d)\r\n",mAngle,mDrivePower);
	mDriveMode = DRIVE_PID;
}
bool MotorDrive::onCommand(const std::vector<std::string> args)
{
	int size = args.size();
	if(size == 1)
	{
		Debug::print(LOG_SUMMARY, "Current Motor Ratio: %d %d\r\n",mMotorL.getPower(),-mMotorR.getPower());
		Debug::print(LOG_SUMMARY, "Current Motor Pulse: %lld %lld\r\n",mpMotorEncoder->getL(),mpMotorEncoder->getR());
	}else if(size >= 2)
	{
		if(args[1].compare("w") == 0)
		{
			//�O�i
			drive(MOTOR_MAX_POWER,MOTOR_MAX_POWER);
			return true;
		}else if(args[1].compare("s") == 0)
		{
			//���
			drive(-MOTOR_MAX_POWER,-MOTOR_MAX_POWER);
			return true;
		}else if(args[1].compare("a") == 0)
		{
			//����
			drive(0,MOTOR_MAX_POWER * 0.7);
			return true;
		}else if(args[1].compare("d") == 0)
		{
			//�E��
			drive(MOTOR_MAX_POWER * 0.7,0);
			return true;
		}else if(args[1].compare("h") == 0)
		{
			//��~
			drive(0,0);
			return true;
		}else if(args[1].compare("p") == 0)
		{
			//PID����֘A
			if(size == 2)
			{
				//PID����J�n(���݂̌���)
				startPID(0,MOTOR_MAX_POWER);
				return true;
			}else if(size == 3)
			{
				//PID(���Ίp�x�w��)
				startPID(atoi(args[2].c_str()),MOTOR_MAX_POWER);
				return true;
			}else if(size == 5)
			{
				//PID�p�����[�^�ݒ�
				set(atof(args[2].c_str()),atof(args[3].c_str()),atof(args[4].c_str()));
				return true;
			}
		}else if(args[1].compare("r") == 0)
		{
			if(size == 4)
			{
				//���V�I�ݒ�
				setRatio(atoi(args[2].c_str()),atoi(args[3].c_str()));
				return true;
			}
		}else
		{
			if(size == 3)
			{
				drive(atoi(args[1].c_str()),atoi(args[2].c_str()));//�o�͒��ڎw��
				return true;
			}
		}
	}
	Debug::print(LOG_PRINT, "motor [w/s/a/d/h]  : move\r\n\
motor p            : pid start\r\n\
motor p [angle]    : pid start with angle to move\r\n\
motor p [P] [I] [D]: set pid params\r\n\
motor r [l] [r]    : set motor ratio\r\n\
motor [l] [r]      : drive motor by specified ratio\r\n");
	return true;
}
MotorDrive::MotorDrive() : mMotorL(),mMotorR(),mDriveMode(DRIVE_RATIO),mRatioL(100),mRatioR(100),mP(0),mI(0),mD(0),mDiff1(0),mDiff2(0),mDiff3(0),mAngle(0),mControlPower(0),mDrivePower(0)
{
	setName("motor");
	setPriority(TASK_PRIORITY_MOTOR,TASK_INTERVAL_MOTOR);

	mpMotorEncoder = MotorEncoder::getInstance();
}
MotorDrive::~MotorDrive(){}
