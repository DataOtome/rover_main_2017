#include <stdarg.h>
#include <stdio.h>
#include <fstream>
#include <iterator>
#include <sstream>
#include <iostream>
#include "utils.h"

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
