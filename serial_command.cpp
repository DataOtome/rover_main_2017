#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <iterator>
#include <sstream>
#include <iostream>
#include <string>
#include "utils.h"
#include "serial_command.h"
#include "motor.h"

SerialCommand gSerialCommand;

void SerialCommand::onUpdate(const struct timespec& time)
{
	int c;
	while((c = getchar()) != EOF)
	{
		mCommandBuffer.push_back(c);
		bool need2update = true;
		if(!mHistory.empty() && mCommandBuffer.size() >= 3)
		{
			if(mCommandBuffer[mCommandBuffer.size() - 3] == '\033')
			{
				//�R�}���h��������
				if(mCommandBuffer[mCommandBuffer.size() - 2] == '[' && mCommandBuffer[mCommandBuffer.size() - 1] == 'A')
				{
					//����L�[
					mCommandBuffer = *mHistoryIterator;

					std::list<std::string>::iterator lastIterator = mHistoryIterator++;
					if(mHistoryIterator == mHistory.end())mHistoryIterator = lastIterator;

					Debug::print(LOG_PRINT, "\r\033[2K%s",mCommandBuffer.c_str());
					need2update = false;
				}else if(mCommandBuffer[mCommandBuffer.size() - 2] == '[' && mCommandBuffer[mCommandBuffer.size() - 1] == 'B')
				{
					//��󉺃L�[
					if(mHistoryIterator != mHistory.begin())
					{
						--mHistoryIterator;
						mCommandBuffer = *mHistoryIterator;
					}else mCommandBuffer.clear();
				
					Debug::print(LOG_PRINT, "\r\033[2K%s",mCommandBuffer.c_str());
					need2update = false;
				}
			}
		}
		if(need2update)Debug::print(LOG_PRINT, "%c", c);//������\��
		
		if(c == '\n')
		{
			mCommandBuffer.erase(mCommandBuffer.size()-1);//���s�������폜

			//�R�}���h�����s
			TaskManager::getInstance()->command(mCommandBuffer);

			if(mCommandBuffer.length() != 0)mHistory.push_front(mCommandBuffer);
			mHistoryIterator = mHistory.begin();
			mCommandBuffer.clear();
			break;
		}
	}
}
SerialCommand::SerialCommand()
{
	//���݂̃^�[�~�i���ݒ��ۑ����A�ύX����
	tcgetattr( STDIN_FILENO, &mOldTermios );
	mNewTermios = mOldTermios;
	mNewTermios.c_lflag &= ~( ICANON | ECHO );
	tcsetattr( STDIN_FILENO, TCSANOW, &mNewTermios );
	fcntl(STDIN_FILENO ,F_SETFL,O_NONBLOCK);

	//�^�X�N�ݒ�
	setPriority(TASK_PRIORITY_COMMUNICATION,TASK_INTERVAL_COMMUNICATION);
}
SerialCommand::~SerialCommand()
{
	//�^�[�~�i���ݒ�����ɖ߂�
	Debug::print(LOG_DETAIL,"Restoreing Terminal Settings\r\n");
	tcsetattr( STDIN_FILENO, TCSANOW, &mOldTermios );
}

