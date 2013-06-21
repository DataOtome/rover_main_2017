#include <wiringPiI2C.h>
#include <wiringPi.h>
#include <time.h>
#include <string.h>
#include <sstream>
#include <unistd.h>
#include <stdlib.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "sensor.h"
#include "debug.h"

PressureSensor gPressureSensor;
GPSSensor gGPSSensor;
GyroSensor gGyroSensor;
LightSensor gLightSensor;
WebCamera gWebCamera;

//////////////////////////////////////////////
// Pressure Sensor
//////////////////////////////////////////////
float PressureSensor::val2float(unsigned int val, int total_bits, int fractional_bits, int zero_pad)
{
	//�C���Z���T�̌W����ǂݍ���Ő������l��Ԃ�
	return static_cast<float>((short int)val) / ((unsigned int)1 << (16 - total_bits + fractional_bits + zero_pad));
}

bool PressureSensor::init()
{
	if((mFileHandle = wiringPiI2CSetup(0x60)) == -1)
	{
		Debug::print(LOG_MINIMUM,"Failed to setup Pressure Sensor\r\n");
		return false;
	}

	//�C���v�Z�p�̌W�����擾
	mA0 = val2float((unsigned int)wiringPiI2CReadReg8(mFileHandle,0x04) << 8 | (unsigned int)wiringPiI2CReadReg8(mFileHandle,0x05),16,3,0);
	mB1 = val2float((unsigned int)wiringPiI2CReadReg8(mFileHandle,0x06) << 8 | (unsigned int)wiringPiI2CReadReg8(mFileHandle,0x07),16,13,0);
	mB2 = val2float((unsigned int)wiringPiI2CReadReg8(mFileHandle,0x08) << 8 | (unsigned int)wiringPiI2CReadReg8(mFileHandle,0x09),16,14,0);
	mC12 = val2float((unsigned int)wiringPiI2CReadReg8(mFileHandle,0x0A) << 8 | (unsigned int)wiringPiI2CReadReg8(mFileHandle,0x0B),14,13,9);

	//�C���擾�v��
	requestSample();

	//�C����v�������������L�^
	struct timespec newTime;
	if(clock_gettime(CLOCK_MONOTONIC_RAW,&newTime) == 0)mLastUpdateRequest = newTime;
	else memset(&mLastUpdateRequest,0,sizeof(mLastUpdateRequest));//���Ԏ擾�Ɏ��s������0��ݒ肷��

	Debug::print(LOG_SUMMARY,"Pressure Sensor is Ready!: (%f %f %f %f)\r\n",mA0,mB1,mB2,mC12);
	return true;
}

void PressureSensor::clean()
{
	close(mFileHandle);
}
void PressureSensor::requestSample()
{
	//�V�����C���擾�v��(3ms��ɒl���ǂݍ��܂�ă��W�X�^�Ɋi�[�����)
	wiringPiI2CWriteReg8(mFileHandle,0x12,0x01);
}
void PressureSensor::update()
{
	struct timespec newTime;
	if(clock_gettime(CLOCK_MONOTONIC_RAW,&newTime) == 0)
	{
		double dt = ((double)(newTime.tv_sec - mLastUpdateRequest.tv_sec) * 1000000000 + newTime.tv_nsec - mLastUpdateRequest.tv_nsec) / 1000000000.0;
		if(dt > 0.003)//�O��̃f�[�^�v������3ms�ȏ�o�߂��Ă���ꍇ�l��ǂݎ���čX�V����
		{
			//�C���l�v�Z
			unsigned int Padc = wiringPiI2CReadReg8(mFileHandle,0x00) << 2 | wiringPiI2CReadReg8(mFileHandle,0x01) >> 6;
			unsigned int Tadc = wiringPiI2CReadReg8(mFileHandle,0x02) << 2 | wiringPiI2CReadReg8(mFileHandle,0x03) >> 6;

			float Pcomp = mA0 + (mB1 + mC12 * Tadc) * Padc + mB2 * Tadc;
			mPressure = (Pcomp * (115 - 50) / 1023.0 + 50) * 10;

			//�C���X�V�v��
			requestSample();

			//�C���X�V�v���������L�^
			mLastUpdateRequest = newTime;
		}
	}
}

bool PressureSensor::command(const std::vector<std::string> args)
{
	Debug::print(LOG_MINIMUM, "Pressure: %d\r\n",mPressure);
	return true;
}

int PressureSensor::get()
{
	return mPressure;
}
PressureSensor::PressureSensor()
{
	setName("pressure");
	setPriority(TASK_PRIORITY_SENSOR,TASK_INTERVAL_SENSOR);
}
PressureSensor::~PressureSensor()
{
}

//////////////////////////////////////////////
// GPS Sensor
//////////////////////////////////////////////
int wiringPiI2CReadReg32LittleEndian(int fd, int address)
{
	return (int)((unsigned int)wiringPiI2CReadReg8(fd, address + 3) << 24 | (unsigned int)wiringPiI2CReadReg8(fd, address + 2) << 16 | (unsigned int)wiringPiI2CReadReg8(fd, address + 1) << 8 | (unsigned int)wiringPiI2CReadReg8(fd, address));
}
int wiringPiI2CReadReg16LittleEndian(int fd, int address)
{
	return (short int)((unsigned int)wiringPiI2CReadReg8(fd, address + 1) << 8 | (unsigned int)wiringPiI2CReadReg8(fd, address));

}
bool GPSSensor::init()
{
	if((mFileHandle = wiringPiI2CSetup(0x20)) == -1)
	{
		Debug::print(LOG_MINIMUM,"Failed to setup GPS Sensor\r\n");
		return false;
	}

	//���W���X�V����悤�ɐݒ�(�ꉞ2�񏑂�����)
	wiringPiI2CWriteReg8(mFileHandle, 0x01, 0x05); 
	wiringPiI2CWriteReg8(mFileHandle, 0x01, 0x05);

	//�o�[�W��������\��
	Debug::print(LOG_SUMMARY,"GPS Firmware Version:%d\r\n",wiringPiI2CReadReg8(mFileHandle, 0x03));

	mPos.x = mPos.y = mPos.z = 0;

	return true;
}
void GPSSensor::clean()
{
	//������~����R�}���h�𔭍s
	wiringPiI2CWriteReg8(mFileHandle, 0x01, 0x06); 

	close(mFileHandle);
}
void GPSSensor::update()
{
	unsigned char status = wiringPiI2CReadReg8(mFileHandle, 0x00);
	if(status & 0x04)// Found Position
	{
		//���W���X�V(�ǂݎ�莞�̃f�[�^����h�~�p��2��ǂݎ���ē������l����ꂽ�ꍇ�̂ݍ̗p����)
		int read = wiringPiI2CReadReg32LittleEndian(mFileHandle, 0x07);
		if(read ==  wiringPiI2CReadReg32LittleEndian(mFileHandle, 0x07))mPos.x = read / 10000000.0;

		read = wiringPiI2CReadReg32LittleEndian(mFileHandle, 0x0B);
		if(read ==  wiringPiI2CReadReg32LittleEndian(mFileHandle, 0x0B))mPos.y = read / 10000000.0;

		read = (unsigned int)wiringPiI2CReadReg16LittleEndian(mFileHandle, 0x21);
		if(read == (unsigned int)wiringPiI2CReadReg16LittleEndian(mFileHandle, 0x21))mPos.z = read;
	}
	//�q�������X�V(�ǂݎ�莞�̃f�[�^����h�~�p��2��ǂݎ���ē������l����ꂽ�ꍇ�̂ݍ̗p����)
	if(wiringPiI2CReadReg8(mFileHandle, 0x00) == status)mSatelites = (unsigned char)status >> 4;
}
bool GPSSensor::command(const std::vector<std::string> args)
{
	if(mSatelites < 4)Debug::print(LOG_MINIMUM, "Unknown Position\r\nSatelites: %d\r\n",mSatelites);
	else Debug::print(LOG_MINIMUM, "Satelites: %d\r\nPosition: %f %f %f\r\n",mSatelites,mPos.x,mPos.y,mPos.z);
	return true;
}
bool GPSSensor::get(VECTOR3& pos)
{
	if(mSatelites >= 4)//3D fix
	{
		pos = mPos;
		return true;
	}
	return false;//Invalid Position
}
GPSSensor::GPSSensor() : mSatelites(0)
{
	setName("gps");
	setPriority(TASK_PRIORITY_SENSOR,TASK_INTERVAL_SENSOR);
}
GPSSensor::~GPSSensor()
{
}

//////////////////////////////////////////////
// Gyro Sensor
//////////////////////////////////////////////

bool GyroSensor::init()
{
	mRVel.x = mRVel.y = mRVel.z = 0;
	mRAngle.x = mRAngle.y = mRAngle.z = 0;
	memset(&mLastSampleTime,0,sizeof(mLastSampleTime));

	if((mFileHandle = wiringPiI2CSetup(0x6b)) == -1)
	{
		Debug::print(LOG_MINIMUM,"Failed to setup Gyro Sensor\r\n");
		return false;
	}
	if(wiringPiI2CReadReg8(mFileHandle,0x0F) != 0xD4)
	{
		Debug::print(LOG_MINIMUM,"Failed to verify Gyro Sensor\r\n");
		return false;
	}
	//�f�[�^�T���v�����O������
	wiringPiI2CWriteReg8(mFileHandle,0x20,0x00);

	//�r�b�O�G���f�B�A���ł̃f�[�^�o�͂ɐݒ�&�X�P�[����2000dps�ɕύX
	wiringPiI2CWriteReg8(mFileHandle,0x23,0x40 | 0x20);

	//FIFO�L����(�X�g���[�����[�h)
	wiringPiI2CWriteReg8(mFileHandle,0x24,0x40);
	wiringPiI2CWriteReg8(mFileHandle,0x2E,0x40);

	//�f�[�^�T���v�����O�L����
	wiringPiI2CWriteReg8(mFileHandle,0x20,0x0f);

	return true;
}

void GyroSensor::clean()
{
	//�f�[�^�T���v�����O������
	wiringPiI2CWriteReg8(mFileHandle,0x20,0x00);

	close(mFileHandle);
}

void GyroSensor::update()
{
	int status_reg;
	int data_samples = 0;
	double newRvx = 0,newRvy = 0,newRvz = 0;

	//�~����ꂽ�T���v���̕��ϒl�������_�ł̑��x�Ƃ���
	while((status_reg = wiringPiI2CReadReg8(mFileHandle,0x27)) & 0x08)
	{
		if(status_reg & 0x70)Debug::print(LOG_DETAIL,"Gyro Data Overrun!\r\n");
		newRvx += (short int)wiringPiI2CReadReg16(mFileHandle,0x28) * 0.070;
		newRvy += (short int)wiringPiI2CReadReg16(mFileHandle,0x2A) * 0.070;
		newRvz += (short int)wiringPiI2CReadReg16(mFileHandle,0x2C) * 0.070;
		++data_samples;
	}
	
	//�f�[�^�����Ă����猻�݂̊p���x�Ɗp�x���X�V
	if(data_samples != 0)
	{
		//����
		mRVel.x = newRvx / data_samples;
		mRVel.y = newRvy / data_samples;
		mRVel.z = newRvz / data_samples;

		//�ϕ�
		struct timespec newTime;
		if(clock_gettime(CLOCK_MONOTONIC_RAW,&newTime) == 0)
		{
			if(mLastSampleTime.tv_sec != 0)
			{
				double dt = ((double)(newTime.tv_sec - mLastSampleTime.tv_sec) * 1000000000 + newTime.tv_nsec - mLastSampleTime.tv_nsec) / 1000000000.0;
				mRAngle.x += mRVel.x * dt;
				mRAngle.y += mRVel.y * dt;
				mRAngle.z += mRVel.z * dt;

				normalize(mRAngle);
			}
			mLastSampleTime = newTime;
		}else Debug::print(LOG_MINIMUM,"Failed to get time\r\n");
	}
}
bool GyroSensor::command(const std::vector<std::string> args)
{
	if(args.size() >= 2)
	{
		if(args[1].compare("reset") == 0)
		{
			setZero();
			return true;
		}
		return false;
	}
	Debug::print(LOG_MINIMUM, "Angle: %f %f %f\r\nAngle Velocity: %f %f %f\r\n",getRx(),getRy(),getRz(),getRvx(),getRvy(),getRvz());
	return true;
}
void GyroSensor::getRVel(VECTOR3& vel)
{
	vel = mRVel;
}
double GyroSensor::getRvx()
{
	return mRVel.x;
}
double GyroSensor::getRvy()
{
	return mRVel.y;
}
double GyroSensor::getRvz()
{
	return mRVel.z;
}
void GyroSensor::setZero()
{
	mRAngle.x = mRAngle.y = mRAngle.z = 0;
}
void GyroSensor::getRPos(VECTOR3& vel)
{
	vel = mRAngle;
}
double GyroSensor::getRx()
{
	return mRAngle.x;
}
double GyroSensor::getRy()
{
	return mRAngle.y;
}
double GyroSensor::getRz()
{
	return mRAngle.z;
}
double GyroSensor::normalize(double pos)
{
	while(pos >= 180 || pos < -180)pos += (pos > 0) ? -360 : 360;
	return pos;
}
void GyroSensor::normalize(VECTOR3& pos)
{
	pos.x = normalize(pos.x);
	pos.y = normalize(pos.y);
	pos.z = normalize(pos.z);
}
GyroSensor::GyroSensor()
{
	setName("gyro");
	setPriority(TASK_PRIORITY_SENSOR,TASK_INTERVAL_GYRO);
}
GyroSensor::~GyroSensor()
{
}


///////////////////////////////////////////////
// CdS Sensor
///////////////////////////////////////////////
bool LightSensor::init()
{
	mPin = PIN_LIGHT_SENSOR;
	pinMode(mPin, INPUT);
	return true;
}
void LightSensor::clean()
{
}
bool LightSensor::command(const std::vector<std::string> args)
{
	if(get())Debug::print(LOG_MINIMUM,"light is high\r\n");
	else Debug::print(LOG_MINIMUM,"light is low\r\n");
	return true;
}
bool LightSensor::get()
{
	return digitalRead(mPin) == 0;
}
LightSensor::LightSensor()
{
	setName("light");
	setPriority(UINT_MAX,UINT_MAX);
}
LightSensor::~LightSensor()
{
}

///////////////////////////////////////////////
// Web�J����
///////////////////////////////////////////////

bool WebCamera::command(const std::vector<std::string> args)
{
	if(args.size() >= 2)
	{
		if(args[1].compare("start") == 0)
		{
			stop();
			Debug::print(LOG_MINIMUM, "Start capturing!\r\n");
			if(args.size() == 3)start(args[2].c_str());
			else start();

			return true;
		}else if(args[1].compare("stop") == 0)
		{
			stop();
			return true;
		}
	}else
	{
		Debug::print(LOG_MINIMUM, "camera start [filename] : save movie to filename\r\n\
camera stop             : stop capturing movie\r\n");
		return true;
	}
	return false;
}
void WebCamera::clean()
{
	stop();
}
void WebCamera::start(const char* filename)
{
	time_t timer = time(NULL);
	std::stringstream ss;
	std::string name;
	if(filename == NULL)
	{
		name = ctime(&timer) + std::string(".avi");
	}else
	{
		name = filename;
	}

	ss << "mencoder tv:// -tv width=320:height=240:device=/dev/video0 -nosound -ovc lavc -o \"" << name << "\" 1> /dev/null 2>&1 &";
	system(ss.str().c_str());
}
void WebCamera::stop()
{
	//Todo: ������5�b���炢�ۑ�����Ȃ����
	system("killall -15 mencoder 1> /dev/null 2>&1 ");
	Debug::print(LOG_DETAIL, "Send kill signal to mencoder\r\n");
}

WebCamera::WebCamera()
{
	setName("capture");
	setPriority(UINT_MAX,UINT_MAX);
}
WebCamera::~WebCamera()
{
}
