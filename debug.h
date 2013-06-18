/*
	�f�o�b�O�v���O����

	�f�o�b�O�p�}�N����print�֐���p�ӂ��Ă���܂�
	�Eprint�֐��͉�ʂƃt�@�C�������ɏo�͂��܂�
	�E���O���x���͏d�v�ł͂Ȃ����O�ŉ�ʂ����ߐs������Ȃ��悤�ɐݒ肵�܂�
	�@���s�ɑ債���e�����Ȃ����O��LOG_DETAIL�A����Ȃ�ɉe�������郍�O��LOG_SUMMARY�A��ɕ\�����郍�O��LOG_MINIMUM��ݒ肵�Ă�������
	�Estatic�N���X�̂��ߒP����Debug::print()�̂悤�ɌĂяo���Ă�������
*/

#pragma once
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
	LOG_DETAIL = 0,	//���S�ȃf�o�b�O���O
	LOG_SUMMARY,	//�������ɕ\���������
	LOG_MINIMUM		//�{�Ԃɕ\���������
}LOG_LEVEL;			//���O���x��(Apache�Ƃ��Ǝ���������)

class Debug
{
public:
	static LOG_LEVEL mLogLevel;

	static void print(LOG_LEVEL level, const char* fmt, ... );
	Debug();
};

//�X�g���[���ʓ|������printf�^�C�v�ł������
