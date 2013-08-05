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
#include "utils.h"

PressureSensor gPressureSensor;
GPSSensor gGPSSensor;
GyroSensor gGyroSensor;
LightSensor gLightSensor;
WebCamera gWebCamera;
DistanceSensor gDistanceSensor;
StereoCamera gStereoCamera;

unsigned int wiringPiI2CReadReg32LE(int fd, int address)
{
	return (unsigned int)((unsigned long)wiringPiI2CReadReg8(fd, address + 3) << 24 | (unsigned int)wiringPiI2CReadReg8(fd, address + 2) << 16 | (unsigned int)wiringPiI2CReadReg8(fd, address + 1) << 8 | (unsigned int)wiringPiI2CReadReg8(fd, address));
}
unsigned short wiringPiI2CReadReg16LE(int fd, int address)
{
	return (unsigned short)((unsigned short)wiringPiI2CReadReg8(fd, address + 1) << 8 | (unsigned short)wiringPiI2CReadReg8(fd, address));
}
unsigned short wiringPiI2CReadReg16BE(int fd, int address)
{
	return (unsigned short)((unsigned short)wiringPiI2CReadReg8(fd, address + 1) | (unsigned short)wiringPiI2CReadReg8(fd, address) << 8);
}


//////////////////////////////////////////////
// Pressure Sensor
//////////////////////////////////////////////
float PressureSensor::val2float(unsigned int val, int total_bits, int fractional_bits, int zero_pad)
{
	//�C���Z���T�̌W����ǂݍ���Ő������l��Ԃ�
	return static_cast<float>((short int)val) / ((unsigned int)1 << (16 - total_bits + fractional_bits + zero_pad));
}

bool PressureSensor::onInit(const struct timespec& time)
{
	if((mFileHandle = wiringPiI2CSetup(0x60)) == -1)
	{
		Debug::print(LOG_SUMMARY,"Failed to setup Pressure Sensor\r\n");
		return false;
	}

	//�C���Z���T�[�̓�����m�F(0xc - 0xf��0�������Ă��邩�m���߂�)
	if(wiringPiI2CReadReg32LE(mFileHandle,0x0c) != 0)
	{
		//close(mFileHandle);
		Debug::print(LOG_SUMMARY,"Failed to verify Pressure Sensor\r\n");
		//return false;
	}

	//�C���v�Z�p�̌W�����擾
	mA0 = val2float(wiringPiI2CReadReg16BE(mFileHandle,0x04),16,3,0);
	mB1 = val2float(wiringPiI2CReadReg16BE(mFileHandle,0x06),16,13,0);
	mB2 = val2float(wiringPiI2CReadReg16BE(mFileHandle,0x08),16,14,0);
	mC12 = val2float(wiringPiI2CReadReg16BE(mFileHandle,0x0A),14,13,9);

	//�C���擾�v��
	requestSample();

	//�C���X�V���N�G�X�g
	mLastUpdateRequest = time;
	requestSample();

	Debug::print(LOG_SUMMARY,"Pressure Sensor is Ready!: (%f %f %f %f)\r\n",mA0,mB1,mB2,mC12);
	return true;
}

void PressureSensor::onClean()
{
	close(mFileHandle);
}
void PressureSensor::requestSample()
{
	//�V�����C���擾�v��(3ms��ɒl���ǂݍ��܂�ă��W�X�^�Ɋi�[�����)
	wiringPiI2CWriteReg8(mFileHandle,0x12,0x01);
}
void PressureSensor::onUpdate(const struct timespec& time)
{
	if(Time::dt(time,mLastUpdateRequest) > 0.003)//�O��̃f�[�^�v������3ms�ȏ�o�߂��Ă���ꍇ�l��ǂݎ���čX�V����
	{
		//�C���l�v�Z
		unsigned int Padc = wiringPiI2CReadReg8(mFileHandle,0x00) << 2 | wiringPiI2CReadReg8(mFileHandle,0x01) >> 6;
		unsigned int Tadc = wiringPiI2CReadReg8(mFileHandle,0x02) << 2 | wiringPiI2CReadReg8(mFileHandle,0x03) >> 6;

		float Pcomp = mA0 + (mB1 + mC12 * Tadc) * Padc + mB2 * Tadc;
		mPressure = (Pcomp * (115 - 50) / 1023.0 + 50) * 10;

		//�C���X�V�v��
		requestSample();

		//�C���X�V�v���������L�^
		mLastUpdateRequest = time;
	}
}

bool PressureSensor::onCommand(const std::vector<std::string> args)
{
	if(!isActive())return false;
	Debug::print(LOG_SUMMARY, "Pressure: %d\r\n",mPressure);
	return true;
}

int PressureSensor::get()
{
	return mPressure;
}
PressureSensor::PressureSensor() : mA0(0),mB1(0),mB2(0),mC12(0),mPressure(0),mFileHandle(-1)
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
bool GPSSensor::onInit(const struct timespec& time)
{
	if((mFileHandle = wiringPiI2CSetup(0x20)) == -1)
	{
		Debug::print(LOG_SUMMARY,"Failed to setup GPS Sensor\r\n");
		return false;
	}

	//���W���X�V����悤�ɐݒ�(�ꉞ2�񏑂�����)
	wiringPiI2CWriteReg8(mFileHandle, 0x01, 0x05); 
	wiringPiI2CWriteReg8(mFileHandle, 0x01, 0x05);

	//�o�[�W��������\��
	Debug::print(LOG_SUMMARY,"GPS Firmware Version:%d\r\n",wiringPiI2CReadReg8(mFileHandle, 0x03));

	mPos.x = mPos.y = mPos.z = 0;
	mIsNewData = false;

	return true;
}
void GPSSensor::onClean()
{
	//������~����R�}���h�𔭍s
	wiringPiI2CWriteReg8(mFileHandle, 0x01, 0x06); 

	close(mFileHandle);
}
void GPSSensor::onUpdate(const struct timespec& time)
{
	unsigned char status = wiringPiI2CReadReg8(mFileHandle, 0x00);
	if(status & 0x06)// Found Position
	{
		//���W���X�V(�ǂݎ�莞�̃f�[�^����h�~�p��2��ǂݎ���ē������l����ꂽ�ꍇ�̂ݍ̗p����)

		//�o�x
		int read = (int)wiringPiI2CReadReg32LE(mFileHandle, 0x07);
		if(read ==  (int)wiringPiI2CReadReg32LE(mFileHandle, 0x07))mPos.x = (int)read / 10000000.0;

		//�ܓx
		read = (int)wiringPiI2CReadReg32LE(mFileHandle, 0x0B);
		if(read ==  (int)wiringPiI2CReadReg32LE(mFileHandle, 0x0B))mPos.y = (int)read / 10000000.0;

		//���x
		read = wiringPiI2CReadReg16LE(mFileHandle, 0x21);
		if(read == wiringPiI2CReadReg16LE(mFileHandle, 0x21))mPos.z = read;

		//�V�����f�[�^���͂������Ƃ��L�^����
		if(status & 0x01)mIsNewData = true;
	}
	//�q�������X�V(�ǂݎ�莞�̃f�[�^����h�~�p��2��ǂݎ���ē������l����ꂽ�ꍇ�̂ݍ̗p����)
	if(wiringPiI2CReadReg8(mFileHandle, 0x00) == status)mSatelites = (unsigned char)status >> 4;
}
bool GPSSensor::onCommand(const std::vector<std::string> args)
{
	if(!isActive())return false;
	if(mSatelites < 4)Debug::print(LOG_SUMMARY, "Unknown Position\r\nSatelites: %d\r\n",mSatelites);
	else Debug::print(LOG_SUMMARY, "Satelites: %d \r\nPosition: %f %f %f\r\n",mSatelites,mPos.x,mPos.y,mPos.z);
	return true;
}
bool GPSSensor::get(VECTOR3& pos)
{
	if(mSatelites >= 4)//3D fix
	{
		mIsNewData = false;//�f�[�^���擾�������Ƃ��L�^
		pos = mPos;//������pos�ɑ��
		return true;
	}
	return false;//Invalid Position
}
bool GPSSensor::isNewPos()
{
	return mIsNewData;
}
GPSSensor::GPSSensor() : mFileHandle(-1),mPos(),mSatelites(0),mIsNewData(false)
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

bool GyroSensor::onInit(const struct timespec& time)
{
	mRVel.x = mRVel.y = mRVel.z = 0;
	mRAngle.x = mRAngle.y = mRAngle.z = 0;
	memset(&mLastSampleTime,0,sizeof(mLastSampleTime));
	
	if((mFileHandle = wiringPiI2CSetup(0x6b)) == -1)
	{
		Debug::print(LOG_SUMMARY,"Failed to setup Gyro Sensor\r\n");
		return false;
	}

	//�W���C���Z���T�[�����퓮�쒆���m�F
	if(wiringPiI2CReadReg8(mFileHandle,0x0F) != 0xD4)
	{
		close(mFileHandle);
		Debug::print(LOG_SUMMARY,"Failed to verify Gyro Sensor\r\n");
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

void GyroSensor::onClean()
{
	//�f�[�^�T���v�����O������
	wiringPiI2CWriteReg8(mFileHandle,0x20,0x00);

	close(mFileHandle);
}

void GyroSensor::onUpdate(const struct timespec& time)
{
	int status_reg;
	int data_samples = 0;
	VECTOR3 newRv;

	//�~����ꂽ�T���v���̕��ϒl�������_�ł̑��x�Ƃ���
	while((status_reg = wiringPiI2CReadReg8(mFileHandle,0x27)) & 0x08)
	{
		//if(status_reg & 0x70)Debug::print(LOG_DETAIL,"Gyro Data Overrun!\r\n");

		//�W���C����FIFO���̃f�[�^�����ׂēǂݍ��݁A�a�����
		VECTOR3 sample;
		sample.x = (short)wiringPiI2CReadReg16BE(mFileHandle,0x28) * 0.070;
		sample.y = (short)wiringPiI2CReadReg16BE(mFileHandle,0x2A) * 0.070;
		sample.z = (short)wiringPiI2CReadReg16BE(mFileHandle,0x2C) * 0.070;
		newRv += sample;

		//�h���t�g�덷�v�Z���ł���Δz��Ƀf�[�^��˂�����
		if(mIsCalculatingOffset)
		{
			mRVelHistory.push_back(sample);
			if(mRVelHistory.size() >= GYRO_SAMPLE_COUNT_FOR_CALCULATE_OFFSET)//�K�v�ȃT���v�������������
			{
				//���ϒl������Ă݂�
				std::list<VECTOR3>::iterator it = mRVelHistory.begin();
				while(it != mRVelHistory.end())
				{
					mRVelOffset += *it;
					++it;
				}
				mRVelOffset /= mRVelHistory.size();//�h���t�g�덷�␳�ʂ�K�p
				mRVelHistory.clear();
				mIsCalculatingOffset = false;
				Debug::print(LOG_SUMMARY, "Gyro: offset is (%f %f %f)\r\n",mRVelOffset.x,mRVelOffset.y,mRVelOffset.z);
			}
		}

		//�h���t�g�덷��␳
		newRv -= mRVelOffset;

		++data_samples;
	}
	
	//�f�[�^�����Ă����猻�݂̊p���x�Ɗp�x���X�V
	if(data_samples != 0)
	{
		//����
		newRv /= data_samples;

		//�ϕ�
		if(mLastSampleTime.tv_sec != 0 || mLastSampleTime.tv_nsec != 0 )
		{
			double dt = Time::dt(time,mLastSampleTime);
			mRAngle += (newRv + mRVel) / 2 * dt;
			normalize(mRAngle);
		}
		mRVel = newRv;
		mLastSampleTime = time;
	}
}
bool GyroSensor::onCommand(const std::vector<std::string> args)
{
	if(args.size() == 2)
	{
		if(args[1].compare("reset") == 0)
		{
			setZero();
			return true;
		}else if(args[1].compare("calib") == 0)
		{
			if(!isActive())return false;
			calibrate();
			return true;
		}
		return false;
	}else if(args.size() == 5)
	{
		if(args[1].compare("calib") == 0)
		{
			mRVelOffset.x = atof(args[2].c_str());
			mRVelOffset.y = atof(args[3].c_str());
			mRVelOffset.z = atof(args[4].c_str());
			Debug::print(LOG_SUMMARY, "Gyro: offset is (%f %f %f)\r\n",mRVelOffset.x,mRVelOffset.y,mRVelOffset.z);
			return true;
		}
		return false;
	}
	Debug::print(LOG_SUMMARY, "Angle: %f %f %f\r\nAngle Velocity: %f %f %f\r\n\
gyro reset  : set angle to zero point\r\n\
gyro calib  : calibrate gyro *do NOT move*\r\n\
gyro calib [x_offset] [y_offset] [z_offset] : calibrate gyro by specified params\r\n",getRx(),getRy(),getRz(),getRvx(),getRvy(),getRvz());
	return true;
}
bool GyroSensor::getRVel(VECTOR3& vel)
{
	if(isActive())
	{
		vel = mRVel;
		return true;
	}
	return false;
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
bool GyroSensor::getRPos(VECTOR3& pos)
{
	if(isActive())
	{
		pos = mRAngle;
		return true;
	}
	return false;
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
void GyroSensor::calibrate()
{
	mIsCalculatingOffset = true;
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
GyroSensor::GyroSensor() : mFileHandle(-1),mRVel(),mRAngle(),mRVelHistory(),mRVelOffset(),mIsCalculatingOffset(false)
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
bool LightSensor::onInit(const struct timespec& time)
{
	pinMode(mPin, INPUT);
	return true;
}
void LightSensor::onClean()
{
}
bool LightSensor::onCommand(const std::vector<std::string> args)
{
	if(!isActive())return false;
	if(get())Debug::print(LOG_SUMMARY,"light is high\r\n");
	else Debug::print(LOG_SUMMARY,"light is low\r\n");
	return true;
}
bool LightSensor::get()
{
	return digitalRead(mPin) == 0;
}
LightSensor::LightSensor() : mPin(PIN_LIGHT_SENSOR)
{
	setName("light");
	setPriority(TASK_PRIORITY_SENSOR,UINT_MAX);
}
LightSensor::~LightSensor()
{
}

///////////////////////////////////////////////
// Web�J����
///////////////////////////////////////////////

bool WebCamera::onCommand(const std::vector<std::string> args)
{
	if(!isActive())return false;
	if(args.size() >= 2)
	{
		if(args[1].compare("start") == 0)
		{
			stop();
			Debug::print(LOG_SUMMARY, "Start capturing!\r\n");
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
		Debug::print(LOG_PRINT, "capture start [filename] : save movie to filename\r\n\
capture stop             : stop capturing movie\r\n");
		return true;
	}
	return false;
}
void WebCamera::onClean()
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

void* DistanceSensor::waitingThread(void* arg)
{
	DistanceSensor& parent = *reinterpret_cast<DistanceSensor*>(arg);
	struct timespec newTime;

	//Send Ping
	pinMode(PIN_DISTANCE, OUTPUT);
	digitalWrite(PIN_DISTANCE, HIGH);
	clock_gettime(CLOCK_MONOTONIC_RAW,&parent.mLastSampleTime);
	do
	{
		clock_gettime(CLOCK_MONOTONIC_RAW,&newTime);
	}while(Time::dt(newTime,parent.mLastSampleTime) < 0.000001);
	digitalWrite(PIN_DISTANCE, LOW);

	//Wait For Result
	pinMode(PIN_DISTANCE, INPUT);
	do
	{
		clock_gettime(CLOCK_MONOTONIC_RAW,&newTime);
		if(Time::dt(newTime,parent.mLastSampleTime) > 0.03)
		{
			//Timeout
			parent.mIsCalculating = false;
			parent.mLastDistance = -1;
			return NULL;
		}	
	}while(digitalRead(PIN_DISTANCE) == LOW);
	parent.mLastSampleTime = newTime;
	while(digitalRead(PIN_DISTANCE) == HIGH)
	{
		clock_gettime(CLOCK_MONOTONIC_RAW,&newTime);
		if(Time::dt(newTime,parent.mLastSampleTime) > 0.03)
		{
			//Timeout
			parent.mIsCalculating = false;
			parent.mLastDistance = -1;
			return NULL;
		}
	}
	clock_gettime(CLOCK_MONOTONIC_RAW,&newTime);

	double delay = Time::dt(newTime,parent.mLastSampleTime);
	parent.mLastDistance = delay * 100 * 3 / 2;
	if(delay > 0.022)parent.mLastDistance = -1;
	parent.mIsNewData = true;
	parent.mIsCalculating = false;

	return NULL;
}
bool DistanceSensor::onInit(const struct timespec& time)
{
	return true;
}
void DistanceSensor::onClean()
{
	if(mIsCalculating)pthread_cancel(mPthread);
	mLastDistance = -1;
	mIsCalculating = false;
	mIsNewData = false;
}

void DistanceSensor::onUpdate(const struct timespec& time)
{

}
bool DistanceSensor::onCommand(const std::vector<std::string> args)
{
	if(!isActive())return false;
	if(args.size() == 1)
	{
		Debug::print(LOG_SUMMARY, "Distance: %f cm\r\n",mLastDistance);
		if(ping())Debug::print(LOG_SUMMARY, "Calculating New Distance!\n",mLastDistance);
		return true;
	}
	return false;
}

bool DistanceSensor::ping()
{
	if(mIsCalculating)return false;//���łɌv�����J�n���Ă���
	if(pthread_create(&mPthread, NULL, waitingThread, this) != 0)
	{
		Debug::print(LOG_SUMMARY, "DistanceSensor: Unable to create thread!\r\n");
		return false;
	}
	mIsCalculating = true;
	return true;
}
bool DistanceSensor::getDistance(double& distance)
{
	bool ret = mIsNewData;
	mIsNewData = false;
	distance = mLastDistance;
	return ret;
}

DistanceSensor::DistanceSensor() : mLastDistance(-1), mIsCalculating(false), mIsNewData(false)
{
	setName("distance");
	setPriority(TASK_PRIORITY_SENSOR,TASK_INTERVAL_SENSOR);
}
DistanceSensor::~DistanceSensor()
{
}

bool StereoCamera::onCommand(const std::vector<std::string> args)
{
	//if(!isActive())return false;
	if(args.size() >= 1)
	{
		Debug::print(LOG_SUMMARY, "Start capturing!\r\n");
		capture();
		return true;
	}else
	{
		Debug::print(LOG_PRINT, "stereo [filename] : save movie to filename\r\n");
		return true;
	}
	return false;
}
void StereoCamera::onClean()
{
	if(mpCapture1 != NULL)cvReleaseCapture(&mpCapture1);
	if(mpCapture2 != NULL)cvReleaseCapture(&mpCapture2);
	if(mpGrayFrame1 != NULL)cvReleaseImage(&mpGrayFrame1);
	if(mpGrayFrame2 != NULL)cvReleaseImage(&mpGrayFrame2);
}
bool StereoCamera::onInit(const struct timespec& time)
{
	mpCapture1 = cvCreateCameraCapture(0);							  //2��Web�J����
	if(mpCapture1 != NULL)
	{
		cvSetCaptureProperty (mpCapture1, CV_CAP_PROP_FRAME_WIDTH, WIDTH); //�B�e�T�C�Y���w��
		cvSetCaptureProperty (mpCapture1, CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);
	}else Debug::print(LOG_SUMMARY, "Unable to initialize Camera0\r\n");

    mpCapture2 = cvCreateCameraCapture(1);
	if(mpCapture2 != NULL)
	{
		cvSetCaptureProperty (mpCapture2, CV_CAP_PROP_FRAME_WIDTH, WIDTH); //�B�e�T�C�Y���w��
		cvSetCaptureProperty (mpCapture2, CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);
	}else Debug::print(LOG_SUMMARY, "Unable to initialize Camera1\r\n");

	mpGrayFrame1 = cvCreateImage(cvSize(WIDTH,HEIGHT), IPL_DEPTH_8U, 1);
	mpGrayFrame2 = cvCreateImage(cvSize(WIDTH,HEIGHT), IPL_DEPTH_8U, 1); 
	return mpGrayFrame1 != NULL && mpGrayFrame2 != NULL;
}
void StereoCamera::capture()
{
	if(!isActive())return;

	IplImage *frame1, *frame2;
	
	//���E�œ����u�Ԃ̉摜��ۑ����邽�߂ɁAGrabFrame�𓯎����ɌĂяo���Ă�����ۂ̕ۑ�������s���悤�ɂ��Ă���܂�
	if(mpCapture1 != NULL)cvGrabFrame(mpCapture1);//�摜���m��
	if(mpCapture2 != NULL)cvGrabFrame(mpCapture2);//�摜���m��
	
	
	if(mpCapture1 != NULL)
	{
		frame1 = cvRetrieveFrame(mpCapture1); //Web�J��������1�t���[�����i�[
		cvCvtColor(frame1, mpGrayFrame1, CV_BGR2GRAY); // �O���[�X�P�[���ɕϊ�

		// �ۑ�
		std::stringstream filename;
		filename << "stereo_l" <<  mSavePicCount << ".jpg";
		cvSaveImage(filename.str().c_str(), mpGrayFrame1);

		filename << ".bmp";
		cvSaveImage(filename.str().c_str(), mpGrayFrame1);
	}else Debug::print(LOG_SUMMARY, "Camera0 is Unavailable\r\n");
	
	if(mpCapture2 != NULL)
	{
		frame2 = cvRetrieveFrame(mpCapture2);
		cvCvtColor(frame2, mpGrayFrame2, CV_BGR2GRAY);

		// �ۑ�
		std::stringstream filename;
		filename << "stereo_r" <<  mSavePicCount << ".jpg";
		cvSaveImage(filename.str().c_str(), mpGrayFrame2);

		filename << ".bmp";
		cvSaveImage(filename.str().c_str(), mpGrayFrame2);
	}else Debug::print(LOG_SUMMARY, "Camera1 is Unavailable\r\n");
	mSavePicCount++;
}

StereoCamera::StereoCamera() : mSavePicCount(0),mpGrayFrame1(NULL),mpGrayFrame2(NULL),mpCapture1(NULL),mpCapture2(NULL)
{
	setName("stereo");
	setPriority(TASK_PRIORITY_SENSOR,UINT_MAX);
}
StereoCamera::~StereoCamera()
{
}

