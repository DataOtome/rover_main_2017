#include <stdarg.h>
#include <stdio.h>
#include <fstream>
#include <iterator>
#include <sstream>
#include <iostream>
#include <math.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "utils.h"

const static unsigned int MAX_STRING_LENGTH = 1024;//Print�p�̃o�b�t�@�T�C�Y

void Debug::print(LOG_LEVEL level, const char* fmt, ... )
{
#ifndef _LOG_DETAIL
	if(level == LOG_DETAIL)return; //�f�o�b�O���[�h�łȂ���΃��O�o�͂��Ȃ�
#endif

	char buf[MAX_STRING_LENGTH];

	va_list argp;
	va_start(argp, fmt);
	vsprintf(buf, fmt, argp);
	
	//��ʂɏo��
	printf(buf);
	//���O�t�@�C���ɏo��
	if(level != LOG_PRINT)
	{
		std::ofstream of("log.txt",std::ios::out | std::ios::app);
		of << buf;
	}
}
void Filename::get(std::string& name)
{
	std::stringstream filename;
	filename << mPrefix << ++mIndex << mSuffix;
	name.assign(filename.str());
}
Filename::Filename(const std::string& prefix,const std::string& suffix) : mPrefix(prefix),mSuffix(suffix),mIndex(0)
{
	//�B�e�C���f�b�N�X�������̃t�@�C���ɏ㏑�����Ȃ��悤�ɕύX
	std::string filename;
	struct stat st;
	do
	{
		get(filename);
	}while(stat(filename.c_str(), &st) == 0);
	--mIndex;
}
double Time::dt(const struct timespec& now,const struct timespec& last)
{
	return ((double)(now.tv_sec - last.tv_sec) * 1000000000 + now.tv_nsec - last.tv_nsec) / 1000000000.0;
}
void String::split(const std::string& input,std::vector<std::string>& outputs)
{
	//��������󔒕����ŕ�������vector�Ɋi�[
	outputs.clear();
	std::istringstream iss(input);
	std::copy(std::istream_iterator<std::string>(iss),  std::istream_iterator<std::string>(), std::back_inserter(outputs));
}

double VECTOR3::calcAngleXY(const VECTOR3& current,const VECTOR3& target)
{
	return atan2(target.y - current.y,target.x - current.x) / M_PI * 180;
}
double VECTOR3::calcDistanceXY(const VECTOR3& current,const VECTOR3& target)
{
	VECTOR3 dif = current - target;
	return sqrt(pow(dif.x,2) + pow(dif.y,2));
}

VECTOR3 VECTOR3::operator+() const
{
	return *this;
}
VECTOR3 VECTOR3::operator-() const
{
	return VECTOR3(-x,-y,-z);
}
VECTOR3& VECTOR3::operator+=(const VECTOR3& v)
{
	x += v.x;
	y += v.y;	
	z += v.z;
	return *this;
}
VECTOR3& VECTOR3::operator-=(const VECTOR3& v)
{
	x -= v.x;
	y -= v.y;	
	z -= v.z;
	return *this;
}
VECTOR3 VECTOR3::operator+(const VECTOR3& v) const
{
	return VECTOR3(x + v.x,y + v.y,z + v.z);
}
VECTOR3 VECTOR3::operator-(const VECTOR3& v) const
{
	return VECTOR3(x - v.x,y - v.y,z - v.z);
}
VECTOR3& VECTOR3::operator*=(const double v)
{
	x *= v;
	y *= v;	
	z *= v;
	return *this;
}
VECTOR3& VECTOR3::operator/=(const double v)
{
	x /= v;
	y /= v;	
	z /= v;
	return *this;
}
VECTOR3 VECTOR3::operator*(const double v) const
{
	return VECTOR3(x * v,y * v,z * v);
}
VECTOR3 VECTOR3::operator/(const double v) const
{
	return VECTOR3(x / v,y / v,z / v);
}
bool VECTOR3::operator==(const VECTOR3& v) const
{
	return (x == v.x) && (y == v.y) && (z == v.z);
}
bool VECTOR3::operator!=(const VECTOR3& v) const
{
	return (x != v.x) || (y != v.y) || (z != v.z);
}

VECTOR3::VECTOR3() : x(0),y(0),z(0){}
VECTOR3::VECTOR3(double tx, double ty, double tz) : x(tx),y(ty),z(tz){}
