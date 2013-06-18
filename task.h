/*
	�^�X�N�N���X

	��{�I�ɂ�����K�v�͂���܂���
	����
		�E�R�}���h�����s
		�E������/�J���𐧌�
		�E���Ԋu���Ƃɍs���K�v�̂��鏈�������s
			���ȈՃ^�X�N�V�X�e��
	
	�g����
		�ETaskBase���p�������N���X���C���X�^���X��������TaskManager�Ɏ����œo�^�����
		�ETaskBase��setRunMode���\�b�h���Ăяo���ƃ^�X�N���L���ɂȂ�A�����������
		�ETaskManager�̊e���\�b�h���Ăяo�����ƂŁATaskBase�̊e���\�b�h���K�؂ɌĂяo�����
*/
#pragma once
#include <map>
#include <string>
#include <limits.h>
#include <vector>

class TaskManager;

//�^�X�N���N���X
class TaskBase
{
private:
	friend class TaskManager;
	TaskBase(const TaskBase& obj);

	//�^�X�N�̏�Ԃ�\���ϐ�
	std::string mName;//�^�X�N��
	unsigned int mPriority,mInterval;//�^�X�N���s�ݒ�(�D��x�A���s�Ԋu)
	unsigned int mSlept;//���s���X�L�b�v���ꂽ��
	bool mIsRunning;//���s��
protected:
	//���̃^�X�N�ɖ��O��ݒ肷�邱�ƂŃR�}���h���󂯕t����悤�ɂ���
	void setName(const char* name);

	//���̃^�X�N�ɗD��x(�������قǐ�Ɏ��s�����)�Ǝ��s�Ԋu(�������قǂ���������s����)��ݒ肷��
	void setPriority(unsigned int pri,unsigned int interval);

	///////////////////////////////////////////////////
	//�e�^�X�N����������֐�
	///////////////////////////////////////////////////
	//���̃^�X�N������������
	virtual bool init();
	//���̃^�X�N���J������
	virtual void clean();
	//�w�肳�ꂽ�R�}���h�����s����
	virtual bool command(const std::vector<std::string> args);
	//������x�̎��Ԃ��ƂɌĂяo�����֐�
	virtual void update();
	///////////////////////////////////////////////////

public:
	//���̃^�X�N�̎��s��Ԃ�ύX����(�K�v�ɉ�����init/clean���Ă΂��)
	void setRunMode(bool running);

	TaskBase();
	~TaskBase();
};

//�^�X�N�}�l�[�W���N���X(�V���O���g��)
class TaskManager
{
private:
	TaskManager(const TaskManager& obj);
	//�Ǘ����̃^�X�N
	std::vector<TaskBase*> mTasks;

	class TaskSoter {
	public:
		bool operator()(const TaskBase* riLeft, const TaskBase* riRight) const {
			return riLeft->mPriority > riRight->mPriority;
		}
	};
	void sortByPriority();
	TaskManager();
public:
	//�C���X�^���X���擾
	static TaskManager* getInstance();

	//������
	bool init();
	//�J��
	void clean();
	//�w�肳�ꂽ�R�}���h�����s����
	bool command(const std::vector<std::string> args);
	//������x�̎��Ԃ��ƂɌĂяo������
	void update();

	//�w�肳�ꂽ�^�X�N��o�^
	void add(TaskBase* pTask);
	void del(TaskBase* pTask);

	~TaskManager();
};
