#include <stdarg.h>
#include <stdio.h>
#include <fstream>
#include "debug.h"

const static unsigned int MAX_STRING_LENGTH = 1024;//Print�p�̃o�b�t�@�T�C�Y

void Debug::print(LOG_LEVEL level, const char* fmt, ... )
{
#ifndef _DEBUG
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
