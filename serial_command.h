/*
	�V���A��������͂��ꂽ�R�}���h����������N���X

	���̃N���X��TaskManager��command���\�b�h���Ăяo���܂�
	task.h���Q��
*/
#pragma once
#include <string>
#include <vector>
#include <termios.h>
#include <list>
#include "task.h"

class SerialCommand : public TaskBase
{
private:
	std::string mCommandBuffer;
	std::list<std::string> mHistory;
	std::list<std::string>::iterator mHistoryIterator;
	int mCursorPos;
	int mEscapeBeginPos;//�G�X�P�[�v�V�[�P���X�̊J�n�ʒu(-1�F�G�X�P�[�v�Ȃ��@-2�F�G�X�P�[�v����)
	struct termios mOldTermios,mNewTermios;
public:
	virtual void onUpdate(const struct timespec& time);//�V���A���|�[�g�ɓ��������R�}���h���m�F����

	SerialCommand();
	~SerialCommand();
};
extern SerialCommand gSerialCommand;
