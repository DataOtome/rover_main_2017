#include <stdarg.h>
#include <stdio.h>
#include <fstream>
#include "debug.h"

const static unsigned int MAX_STRING_LENGTH = 1024;//Print�p�̃o�b�t�@�T�C�Y
LOG_LEVEL Debug::mLogLevel = LOG_SUMMARY;				//�f�t�H���g�̃��O�o�̓��x��

void Debug::print(LOG_LEVEL level, const char* fmt, ... )
{
	if(level < mLogLevel)return;//���O�o�͂��Ȃ�
	char buf[MAX_STRING_LENGTH];

	va_list argp;
	va_start(argp, fmt);
	vsprintf(buf, fmt, argp);
	
	//��ʂɏo��
	printf(buf);
	//���O�t�@�C���ɏo��
	std::ofstream of("log.txt",std::ios::out | std::ios::app);
	of << buf;
}
