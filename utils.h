/*
	���̑��֐��Ȃ�

	�f�o�b�O�p�}�N����print�֐���p�ӂ��Ă���܂�
	�Eprint�֐��͉�ʂƃt�@�C�������ɏo�͂��܂�
	�E���O���x���͏d�v�ł͂Ȃ����O�ŉ�ʂ����ߐs������Ȃ��悤�ɐݒ肵�܂�
	�Estatic�N���X�̂��ߒP����Debug::print()�̂悤�ɌĂяo���Ă�������
*/

#pragma once
#include <vector>
#include <string>
#include "constants.h"

#ifdef _DEBUG
	#include <assert.h>
	//x��0�Ȃ�abort
	#define ASSERT(x) assert(x);
	//x����0�Ȃ�abort
	#define VERIFY(x) assert(!(x));
#else
	#define ASSERT(x)
	#define VERIFY(x)
#endif

typedef enum
{
	LOG_DETAIL = 0,	//�f�o�b�O���O(�o�O���o���Ƃ��̏󋵊m�F�p)
	LOG_SUMMARY,	//�t�@�C���ۑ��A��ʕ\�����ɂ������
	LOG_PRINT		//��ʂɂ̂ݕ\���������
}LOG_LEVEL;			//���O���x��(Apache�Ƃ��Ǝ���������)

class Debug
{
public:
	static void print(LOG_LEVEL level, const char* fmt, ... );//�X�g���[���ʓ|������printf�^�C�v�ł������
	Debug();
};

class Time
{
public:
	//���Ԃ̕ω��ʂ��v�Z(�b)
	static double dt(const struct timespec& now,const struct timespec& last);
};

class String
{
public:
	//��������󔒂ŕ���
	static void split(const std::string& input,std::vector<std::string>& outputs);
};

class Filename
{
	std::string mPrefix,mSuffix;
	unsigned int mIndex;
public:
	void get(std::string& name);
	Filename(const std::string& prefix,const std::string& suffix);
};

class VECTOR3
{
public:
	double x,y,z;

	VECTOR3 operator+() const;
	VECTOR3 operator-() const;
	VECTOR3& operator+=(const VECTOR3& v);
	VECTOR3& operator-=(const VECTOR3& v);
	VECTOR3 operator+(const VECTOR3& u) const;
	VECTOR3 operator-(const VECTOR3& u) const;
	VECTOR3& operator*=(const double v);
	VECTOR3& operator/=(const double v);
	VECTOR3 operator*(const double v) const;
	VECTOR3 operator/(const double v) const;
	bool operator==(const VECTOR3& v)const;
	bool operator!=(const VECTOR3& v)const;

	VECTOR3();
	VECTOR3(double tx, double ty, double tz);

	//XY���ʏ��2�_�����Ԓ����̊p�x(�k��0�x��+180�x(��)�`-180�x(��))�B�W���C���̊p�x�Ƃ͐������t�ł�
	static double calcAngleXY(const VECTOR3& current,const VECTOR3& target);
	//2�_�Ԃ̋������v�Z
	static double calcDistanceXY(const VECTOR3& current,const VECTOR3& target);
};
