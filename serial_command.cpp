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
		mCommandBuffer.insert(mCursorPos,(char*)(&c),1);
		bool need2update = false;
		if(mCommandBuffer[mCursorPos] == '\033')mEscapeBeginPos = mCursorPos;
		if(mCursorPos - mEscapeBeginPos >= 2 && mEscapeBeginPos >= 0)
		{
			//�R�}���h��������
			if(mCommandBuffer[mCursorPos - 1] == '[' && mCommandBuffer[mCursorPos - 0] == 'A' && !mHistory.empty())
			{
				//����L�[
				mCommandBuffer = *mHistoryIterator;

				std::list<std::string>::iterator lastIterator = mHistoryIterator++;
				if(mHistoryIterator == mHistory.end())mHistoryIterator = lastIterator;
				mCursorPos = mCommandBuffer.size();

				Debug::print(LOG_PRINT, "\r\033[2K%s",mCommandBuffer.c_str());
				need2update = true;
				--mCursorPos;
			}else if(mCommandBuffer[mCursorPos - 1] == '[' && mCommandBuffer[mCursorPos - 0] == 'B' && !mHistory.empty())
			{
				//��󉺃L�[
				if(mHistoryIterator != mHistory.begin())
				{
					--mHistoryIterator;
					mCommandBuffer = *mHistoryIterator;
				}else mCommandBuffer.clear();
				mCursorPos = mCommandBuffer.size();
			
				Debug::print(LOG_PRINT, "\r\033[2K%s",mCommandBuffer.c_str());
				need2update = true;
				--mCursorPos;
			}else if(mCommandBuffer[mCursorPos - 1] == '[' && mCommandBuffer[mCursorPos - 0] == 'D')
			{
				//��󍶃L�[
				mCommandBuffer.erase(mCursorPos - 2,3);
				mCursorPos = std::max(mCursorPos - 2 - 1,0) - 1;
				Debug::print(LOG_PRINT, "\033[D");
			}else if(mCommandBuffer[mCursorPos - 1] == '[' && mCommandBuffer[mCursorPos - 0] == 'C')
			{
				//���E�L�[
				mCommandBuffer.erase(mCursorPos - 2,3);
				if((unsigned int)mCursorPos < mCommandBuffer.size() + 2)Debug::print(LOG_PRINT, "\033[C");
				mCursorPos = std::min(mCursorPos - 2 + 1,(int)mCommandBuffer.size()) - 1;
			}
			mEscapeBeginPos = -2;
		}
		if(mCommandBuffer[mCursorPos] == '\b' && mCursorPos >= 1)
		{
			//�o�b�N�X�y�[�X
			mCommandBuffer.erase(mCursorPos - 1,2);
			mCursorPos -= 2;
		}
		++mCursorPos;
		if(mEscapeBeginPos == -1 && c != '\n')
		{
			if((unsigned int)mCursorPos != mCommandBuffer.size() || need2update)
			{
				//�J�[�\�����s�̓r���̏ꍇ�͍ĕ`��
				Debug::print(LOG_PRINT, "\r\033[2K%s",mCommandBuffer.c_str());
				Debug::print(LOG_PRINT, "\033[%dD",mCommandBuffer.size() - mCursorPos);
			}else
			{
				Debug::print(LOG_PRINT, "%c", c);//������\��
			}
		}else if(mEscapeBeginPos == -2)mEscapeBeginPos = -1;
		if(c == '\n')
		{
			if(mCommandBuffer[mCursorPos - 1] == '\n')mCommandBuffer.erase(mCursorPos - 1,1);//���s�������폜
			Debug::print(LOG_PRINT, "\r");

			//�R�}���h�����s
			TaskManager::getInstance()->command(mCommandBuffer);

			if(mCommandBuffer.length() != 0)mHistory.push_front(mCommandBuffer);
			mHistoryIterator = mHistory.begin();
			mCommandBuffer.clear();
			mCursorPos = 0;
			mEscapeBeginPos = -1;
			break;
		}
	}
}
SerialCommand::SerialCommand() : mCursorPos(0),mEscapeBeginPos(-1)
{
	//���݂̃^�[�~�i���ݒ��ۑ����A�ύX����
	tcgetattr( STDIN_FILENO, &mOldTermios );
	mNewTermios = mOldTermios;
	mNewTermios.c_lflag &= ~( ICANON | ECHO );
	tcsetattr( STDIN_FILENO, TCSANOW, &mNewTermios );
	fcntl(STDIN_FILENO ,F_SETFL,O_NONBLOCK);

	//�^�X�N�ݒ�
	setName("serial");
	setPriority(TASK_PRIORITY_COMMUNICATION,TASK_INTERVAL_COMMUNICATION);
}
SerialCommand::~SerialCommand()
{
	//�^�[�~�i���ݒ�����ɖ߂�
	Debug::print(LOG_DETAIL,"Restoreing Terminal Settings\r\n");
	tcsetattr( STDIN_FILENO, TCSANOW, &mOldTermios );
}

