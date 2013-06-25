#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <fstream>
#include <string>
#include <iostream>
#include "sequence.h"
#include "utils.h"

void sigHandler(int p_signame);
bool setSighandle(int p_signame);

//���s���t���O(false�ŏI��)
static bool gIsRunning = true;

//initialize.txt��ǂݍ���ŏ����ݒ���s���֐�
// 1�s�ځ@�@�F�g�p����^�X�N���X�y�[�X�ŋ�؂��ė�
// 2�s�ڈȍ~�F���s����R�}���h���s���Ƃɗ�
bool parseInitializer()
{
	TaskManager* pTaskMan = TaskManager::getInstance();
	Debug::print(LOG_SUMMARY, "Reading initialize.txt...");
	std::ifstream ifs( "initialize.txt" );
	std::string str;
	if(ifs.good())
	{
		//initialize.txt�����݂���ꍇ
		
		//�擪�s�ɗ񋓂��ꂽ�^�X�N���g�p����悤�ɐݒ肷��
		std::getline(ifs,str);
		Debug::print(LOG_SUMMARY, "OK!\r\n");

		std::vector<std::string> tasks;
		String::split(str,tasks);

		std::vector<std::string>::iterator it = tasks.begin();
		while(it != tasks.end())
		{
			TaskBase* pTask = pTaskMan->get(*it);
			if(pTask != NULL)
			{
				Debug::print(LOG_SUMMARY, "Loading %s...\r\n",it->c_str());
				pTask->setRunMode(true);
			}else Debug::print(LOG_SUMMARY, "%s is not Available!\r\n",it->c_str());
			++it;
		}

		//�g�p����^�X�N������������
		pTaskMan->update();

		Debug::print(LOG_SUMMARY, "Executing Initializing Commands...\r\n");
		//2�s�ڈȍ~�̃R�}���h�����ׂĎ��s����
		while(!ifs.eof() && !ifs.fail() && !ifs.bad())
		{
			std::getline(ifs,str);
			pTaskMan->command(str);
		}
		return true;
	}
	return false;
}

int main(int argc, char** argv)
{
	time_t timer;
	timer = time(NULL);
	Debug::print(LOG_SUMMARY,"%s\r\n2013 Takadama-lab ARLISS\r\n* Rivai Team *\r\n",ctime(&timer));

	//�L�[�{�[�h�ɂ��I����j�~
	if(!(setSighandle(SIGINT) && setSighandle(SIGQUIT)))
	{
		Debug::print(LOG_SUMMARY,"Failed to set signal!\r\n");
	}

	//wiring pi������
    if(wiringPiSetup() != 0)
	{
		Debug::print(LOG_SUMMARY,"Failed to setup wiringPi!\r\n");
		return -1;
	}

	//�^�X�N�ݒ�
	TaskManager* pTaskMan = TaskManager::getInstance();

	///////////////////////////////////////////
	// �^�X�N���g�p����悤�ɐݒ�
	if(!parseInitializer())
	{
		//initialize.txt���ǂݍ��߂Ȃ��������߁A�W����ԂŋN������
		Debug::print(LOG_SUMMARY, "Not Found.\r\nLoading Default Task...\r\n");
		gTestingState.setRunMode(true);
	}
	Debug::print(LOG_SUMMARY, "Ready.\r\n");

	
	////////////////////////////////////////////
	//���C�����[�v(�u���b�N�Ȃǂ����ɒZ���Ԃŏ�����Ԃ�����)
	while(gIsRunning)
	{
		//�^�X�N����(���̊֐��ЂƂŃ^�X�N�����s�����)
		pTaskMan->update();
		
		//CPU�������L���Ȃ��悤��Wait���͂���
		delay(1);
	}

	pTaskMan->clean();
	return 0;
}

//�V�O�i�������n���h���ݒ�
bool setSighandle(int p_signame)
{
	return signal(p_signame, sigHandler) != SIG_ERR;
}

//�V�O�i������(Ctrl-C�ɑ΂�����S���m��)
void sigHandler(int p_signame)
{
	
	switch(p_signame)
	{
	case 2:	//Ctrl-C�𖳎�
		{
			static time_t last_pressed = 0;
			time_t cur_time;
			time(&cur_time);
			if(last_pressed + 3 > cur_time)
			{

#ifdef _DEBUG
				Debug::print(LOG_SUMMARY,"Shutting down...\r\n");
				gIsRunning = false;
				return;
#endif

			}else Debug::print(LOG_SUMMARY,"Ctrl-C is disabled\r\n");

#ifdef _DEBUG
			Debug::print(LOG_SUMMARY,"To quit, Press Ctrl-C again!\r\n");
#endif

			last_pressed = cur_time;
			break;
		}
	case 3: //�L�[�{�[�h�ɂ�钆�~�𖳎�
		Debug::print(LOG_SUMMARY,"Quit by keyboard is disabled\r\n");
		break;
	}
}

