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
	struct termios mOldTermios,mNewTermios;
public:
	virtual void onUpdate();//�V���A���|�[�g�ɓ��������R�}���h���m�F����

	SerialCommand();
	~SerialCommand();
};
extern SerialCommand gSerialCommand;
