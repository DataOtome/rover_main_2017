#include <algorithm>
#include "utils.h"
#include "task.h"


TaskBase::TaskBase() : mName(""),mPriority(UINT_MAX),mInterval(UINT_MAX),mSlept(0),mIsRunning(false),mNewRunningState(false)
{
	TaskManager* pManager = TaskManager::getInstance();
	pManager->add(this);
}
TaskBase::~TaskBase()
{
	TaskManager* pManager = TaskManager::getInstance();
	pManager->del(this);
}


void TaskBase::setRunMode(bool running)
{
	//新しい状態を設定する(TaskManagerのupdate時に実際に変更される)
	mNewRunningState = running;
}
void TaskBase::setName(const char* name)
{
	if(name == NULL)
	{
		Debug::print(LOG_SUMMARY ,"Task name is NOT specified!\r\n");
		return;
	}
	mName = std::string(name);
}
void TaskBase::setPriority(unsigned int pri,unsigned int interval)
{
	mPriority = pri;
	mInterval = interval;
}
bool TaskBase::onInit(const struct timespec& time)
{
	return true;
}

void TaskBase::onClean()
{
}

bool TaskBase::onCommand(const std::vector<std::string> args)
{
	Debug::print(LOG_DETAIL,"Command Not Available!\r\n");
	return true;
}

void TaskBase::onUpdate(const struct timespec& time)
{
}

TaskManager::TaskManager()
{
}
TaskManager::~TaskManager()
{
	clean();
}
TaskManager* TaskManager::getInstance()
{
	static TaskManager signleton;
	return &signleton;
}

bool TaskManager::init()
{
	mTasks.clear();
	return true;
}
void TaskManager::clean()
{
	std::vector<TaskBase*>::iterator it = mTasks.begin();
	while(it != mTasks.end())
	{
		if(*it != NULL)if((**it).mIsRunning)(*it)->onClean();
		++it;
	}
	mTasks.clear();
}

bool TaskManager::command(std::string arg)
{
	std::vector<std::string> args;
	String::split(arg,args);
	if(args.size() != 0)
	{
		Debug::print(LOG_SUMMARY, "> %s\r\n",arg.c_str());
		TaskBase* pTask = get(args[0]);
		if(pTask != NULL)
		{
			//コマンド実行対象のタスクが見つかったらコマンドを実行
			if(pTask->mName.compare(args[0]) == 0)
			{
				if(pTask->mIsRunning)//アクティブなタスクの場合コマンド実行
				{
					if(pTask->onCommand(args))return true;
				}else
				{
					//アクティブではないタスクの場合
					Debug::print(LOG_PRINT, "This task is not working.\r\n");
					return false;
				}
			}
		}
		Debug::print(LOG_SUMMARY, "Command Not Found\r\n");
	}else
	{
		Debug::print(LOG_PRINT, " W Priority Interval Name\r\n");

		//すべてのタスクとその状態を列挙して表示
		std::vector<TaskBase*>::iterator it = mTasks.begin();
		while(it != mTasks.end())
		{
			TaskBase* pTask = *it;
			if(pTask != NULL)
			{
				Debug::print(LOG_SUMMARY, " %c %8d %8d %s\r\n",pTask->mIsRunning ? 'W' : ' ',pTask->mPriority,pTask->mInterval,pTask->mName.c_str());
			}
			++it;
		}
	}
	return false;
}
void TaskManager::update()
{
	struct timespec newTime;
	if(clock_gettime(CLOCK_MONOTONIC_RAW,&newTime) != 0)
	{
		Debug::print(LOG_DETAIL, "FAILED to get time!\r\n");
	}
	std::vector<TaskBase*>::iterator it = mTasks.begin();
	while(it != mTasks.end())
	{
		TaskBase* pTask = *it;
		if(pTask != NULL)
		{
			if(pTask->mIsRunning != pTask->mNewRunningState)
			{
				//実行状態を変更する必要がある場合変更する
				if(pTask->mIsRunning == false)pTask->onInit(newTime);
				else if(pTask->mIsRunning == true)pTask->onClean();

				pTask->mIsRunning = pTask->mNewRunningState;
				pTask->mSlept = 0;
			}else if(pTask->mInterval != UINT_MAX && pTask->mIsRunning && (pTask->mInterval <= pTask->mSlept++))
			{
				//実行するタイミングであれば処理を行う(mIntervalがUINT_MAXならupdate不要なタスク)
				pTask->onUpdate(newTime);
				pTask->mSlept = 0;
			}
		}
		++it;
	}
}

TaskBase* TaskManager::get(const std::string name)
{
	std::vector<TaskBase*>::iterator it = mTasks.begin();
	while(it != mTasks.end())
	{
		TaskBase* pTask = *it;
		if(pTask != NULL)
		{
			if(pTask->mName.compare(name) == 0)
			{
				return pTask;
			}
		}
		++it;
	}
	return NULL;
}
void TaskManager::setRunMode(bool running)
{
	std::vector<TaskBase*>::iterator it = mTasks.begin();
	while(it != mTasks.end())
	{
		TaskBase* pTask = *it;
		if(pTask != NULL)
		{
			pTask->setRunMode(running);
		}
		++it;
	}
}
void TaskManager::add(TaskBase* pTask)
{
	if(pTask == NULL)
	{
		Debug::print(LOG_DETAIL ,"TaskManager(add): No Task Specified!\r\n");
		return;
	}
	if(find(mTasks.begin(),mTasks.end(), pTask) == mTasks.end())
	{
		//すでに追加されていなければタスクを追加する
		mTasks.push_back(pTask);
		sortByPriority();
	}
}
void TaskManager::del(TaskBase* pTask)
{
	if(pTask == NULL)
	{
		Debug::print(LOG_SUMMARY ,"TaskManager(del): No Task Specified!\r\n");
		return;
	}
	std::vector<TaskBase*>::iterator it = mTasks.begin();
	while(it != mTasks.end())
	{
		if(pTask == *it)
		{
			*it = NULL;//vectorを直接deleteしてしまうとイテレータが使えなくなるため、NULLを代入しておく
			Debug::print(LOG_DETAIL ,"TaskManager(del): Succeeded!\r\n");
			return;
		}
		++it;
	}
	Debug::print(LOG_DETAIL ,"TaskManager(del): Task Not Found!\r\n");
}
void  TaskManager::sortByPriority()
{
	std::sort(mTasks.begin(),mTasks.end(),TaskSoter());
}
