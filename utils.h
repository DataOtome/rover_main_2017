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
