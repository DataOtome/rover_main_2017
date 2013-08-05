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
	//気圧センサの係数を読み込んで正しい値を返す
	return static_cast<float>((short int)val) / ((unsigned int)1 << (16 - total_bits + fractional_bits + zero_pad));
}

bool PressureSensor::onInit(const struct timespec& time)
{
	if((mFileHandle = wiringPiI2CSetup(0x60)) == -1)
	{
		Debug::print(LOG_SUMMARY,"Failed to setup Pressure Sensor\r\n");
		return false;
	}

	//気圧センサーの動作を確認(0xc - 0xfに0が入っているか確かめる)
	if(wiringPiI2CReadReg32LE(mFileHandle,0x0c) != 0)
	{
		//close(mFileHandle);
		Debug::print(LOG_SUMMARY,"Failed to verify Pressure Sensor\r\n");
		//return false;
	}

	//気圧計算用の係数を取得
	mA0 = val2float(wiringPiI2CReadReg16BE(mFileHandle,0x04),16,3,0);
	mB1 = val2float(wiringPiI2CReadReg16BE(mFileHandle,0x06),16,13,0);
	mB2 = val2float(wiringPiI2CReadReg16BE(mFileHandle,0x08),16,14,0);
	mC12 = val2float(wiringPiI2CReadReg16BE(mFileHandle,0x0A),14,13,9);

	//気圧取得要求
	requestSample();

	//気圧更新リクエスト
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
	//新しい気圧取得要求(3ms後に値が読み込まれてレジスタに格納される)
	wiringPiI2CWriteReg8(mFileHandle,0x12,0x01);
}
void PressureSensor::onUpdate(const struct timespec& time)
{
	if(Time::dt(time,mLastUpdateRequest) > 0.003)//前回のデータ要請から3ms以上経過している場合値を読み取って更新する
	{
		//気圧値計算
		unsigned int Padc = wiringPiI2CReadReg8(mFileHandle,0x00) << 2 | wiringPiI2CReadReg8(mFileHandle,0x01) >> 6;
		unsigned int Tadc = wiringPiI2CReadReg8(mFileHandle,0x02) << 2 | wiringPiI2CReadReg8(mFileHandle,0x03) >> 6;

		float Pcomp = mA0 + (mB1 + mC12 * Tadc) * Padc + mB2 * Tadc;
		mPressure = (Pcomp * (115 - 50) / 1023.0 + 50) * 10;

		//気圧更新要請
		requestSample();

		//気圧更新要請時刻を記録
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

	//座標を更新するように設定(一応2回書き込み)
	wiringPiI2CWriteReg8(mFileHandle, 0x01, 0x05); 
	wiringPiI2CWriteReg8(mFileHandle, 0x01, 0x05);

	//バージョン情報を表示
	Debug::print(LOG_SUMMARY,"GPS Firmware Version:%d\r\n",wiringPiI2CReadReg8(mFileHandle, 0x03));

	mPos.x = mPos.y = mPos.z = 0;
	mIsNewData = false;

	return true;
}
void GPSSensor::onClean()
{
	//動作を停止するコマンドを発行
	wiringPiI2CWriteReg8(mFileHandle, 0x01, 0x06); 

	close(mFileHandle);
}
void GPSSensor::onUpdate(const struct timespec& time)
{
	unsigned char status = wiringPiI2CReadReg8(mFileHandle, 0x00);
	if(status & 0x06)// Found Position
	{
		//座標を更新(読み取り時のデータ乱れ防止用に2回読み取って等しい値が取れた場合のみ採用する)

		//経度
		int read = (int)wiringPiI2CReadReg32LE(mFileHandle, 0x07);
		if(read ==  (int)wiringPiI2CReadReg32LE(mFileHandle, 0x07))mPos.x = (int)read / 10000000.0;

		//緯度
		read = (int)wiringPiI2CReadReg32LE(mFileHandle, 0x0B);
		if(read ==  (int)wiringPiI2CReadReg32LE(mFileHandle, 0x0B))mPos.y = (int)read / 10000000.0;

		//高度
		read = wiringPiI2CReadReg16LE(mFileHandle, 0x21);
		if(read == wiringPiI2CReadReg16LE(mFileHandle, 0x21))mPos.z = read;

		//新しいデータが届いたことを記録する
		if(status & 0x01)mIsNewData = true;
	}
	//衛星個数を更新(読み取り時のデータ乱れ防止用に2回読み取って等しい値が取れた場合のみ採用する)
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
		mIsNewData = false;//データを取得したことを記録
		pos = mPos;//引数のposに代入
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

	//ジャイロセンサーが正常動作中か確認
	if(wiringPiI2CReadReg8(mFileHandle,0x0F) != 0xD4)
	{
		close(mFileHandle);
		Debug::print(LOG_SUMMARY,"Failed to verify Gyro Sensor\r\n");
		return false;
	}
	//データサンプリング無効化
	wiringPiI2CWriteReg8(mFileHandle,0x20,0x00);

	//ビッグエンディアンでのデータ出力に設定&スケールを2000dpsに変更
	wiringPiI2CWriteReg8(mFileHandle,0x23,0x40 | 0x20);

	//FIFO有効化(ストリームモード)
	wiringPiI2CWriteReg8(mFileHandle,0x24,0x40);
	wiringPiI2CWriteReg8(mFileHandle,0x2E,0x40);

	//データサンプリング有効化
	wiringPiI2CWriteReg8(mFileHandle,0x20,0x0f);

	return true;
}

void GyroSensor::onClean()
{
	//データサンプリング無効化
	wiringPiI2CWriteReg8(mFileHandle,0x20,0x00);

	close(mFileHandle);
}

void GyroSensor::onUpdate(const struct timespec& time)
{
	int status_reg;
	int data_samples = 0;
	VECTOR3 newRv;

	//蓄えられたサンプルの平均値を現時点での速度とする
	while((status_reg = wiringPiI2CReadReg8(mFileHandle,0x27)) & 0x08)
	{
		//if(status_reg & 0x70)Debug::print(LOG_DETAIL,"Gyro Data Overrun!\r\n");

		//ジャイロのFIFO内のデータをすべて読み込み、和を取る
		VECTOR3 sample;
		sample.x = (short)wiringPiI2CReadReg16BE(mFileHandle,0x28) * 0.070;
		sample.y = (short)wiringPiI2CReadReg16BE(mFileHandle,0x2A) * 0.070;
		sample.z = (short)wiringPiI2CReadReg16BE(mFileHandle,0x2C) * 0.070;
		newRv += sample;

		//ドリフト誤差計算中であれば配列にデータを突っ込む
		if(mIsCalculatingOffset)
		{
			mRVelHistory.push_back(sample);
			if(mRVelHistory.size() >= GYRO_SAMPLE_COUNT_FOR_CALCULATE_OFFSET)//必要なサンプル数がそろった
			{
				//平均値を取ってみる
				std::list<VECTOR3>::iterator it = mRVelHistory.begin();
				while(it != mRVelHistory.end())
				{
					mRVelOffset += *it;
					++it;
				}
				mRVelOffset /= mRVelHistory.size();//ドリフト誤差補正量を適用
				mRVelHistory.clear();
				mIsCalculatingOffset = false;
				Debug::print(LOG_SUMMARY, "Gyro: offset is (%f %f %f)\r\n",mRVelOffset.x,mRVelOffset.y,mRVelOffset.z);
			}
		}

		//ドリフト誤差を補正
		newRv -= mRVelOffset;

		++data_samples;
	}
	
	//データが来ていたら現在の角速度と角度を更新
	if(data_samples != 0)
	{
		//平均
		newRv /= data_samples;

		//積分
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
// Webカメラ
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
	//Todo: 末尾が5秒くらい保存されない問題
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
