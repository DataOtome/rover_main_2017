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
	bool mNewRunningState;//�V�������s���
protected:
	//���̃^�X�N�ɖ��O��ݒ肷�邱�ƂŃR�}���h���󂯕t����悤�ɂ���
	void setName(const char* name);

	//���̃^�X�N�ɗD��x(�������قǐ�Ɏ��s�����)�Ǝ��s�Ԋu(�������قǂ���������s����)��ݒ肷��
	void setPriority(unsigned int pri,unsigned int interval);

	//���̃^�X�N���Ǘ�����TaskManager�̃C���X�^���X��Ԃ�
	virtual TaskManager* getManager();

	///////////////////////////////////////////////////
	//�e�^�X�N����������֐�
	///////////////////////////////////////////////////
	/* ���̃^�X�N������������
	  �Ăяo���^�C�~���O��TaskManager��update���\�b�h���Ŏ��s���^�X�N��onUpdate�������I�������
	  false��Ԃ����ꍇ�ATaskManager��update���\�b�h���Ă΂�邽�тɍēx�Ăяo�����
	*/
	virtual bool onInit(const struct timespec& time);

	/* ���̃^�X�N���J������
	  �s�v�ȓd�͏����}���邽�߂ɁA�ɗ͍ŏ����̏�ԂɕύX���邱��
	*/
	virtual void onClean();

	/* �w�肳�ꂽ�R�}���h�����s����
	  ���̃��\�b�h�͎��s��Ԃɂ�����炸�Ăяo����邽�ߒ���
	  false��Ԃ��ƁA�R�}���h�̎��s�Ɏ��s�����|���\�������
	*/
	virtual bool onCommand(const std::vector<std::string> args);

	/* ������x�̎��Ԃ��ƂɌĂяo�����֐�
	  ��ms�ȓ��ɏ�����Ԃ����ƁI�I
	  ���s����Ԃ̏ꍇ�ɂ̂݌Ăяo�����
	*/
	virtual void onUpdate(const struct timespec& time);
	///////////////////////////////////////////////////

public:
	//���̃^�X�N�̎��s��Ԃ�Ԃ�
	bool isActive();

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

	class TaskSorter {
	public:
		bool operator()(const TaskBase* riLeft, const TaskBase* riRight) const {
			if(riLeft == NULL)return false;
			if(riRight == NULL)return true;
			return riLeft->mPriority < riRight->mPriority;
		}
	};
	TaskManager();
public:
	//�C���X�^���X���擾
	static TaskManager* getInstance();

	//������
	bool init();
	//�J��
	void clean();
	//�w�肳�ꂽ�R�}���h�����s����(�󔒕�����؂�)
	bool command(std::string arg);
	//������x�̎��Ԃ��ƂɌĂяo������
	void update();
	//�w�肳�ꂽ�^�X�N�ւ̃|�C���^��Ԃ�(NULL�̓G���[)
	TaskBase* get(const std::string name);

	//�S�^�X�N�̎��s��Ԃ�ύX����
	void setRunMode(bool running);

	//�w�肳�ꂽ�^�X�N��o�^/�폜(��{�I�ɌĂяo���K�v�Ȃ�)
	void add(TaskBase* pTask);
	void del(TaskBase* pTask);

	//�^�X�N���X�g��D��x���ɕ��ёւ���(��{�I�ɌĂяo���K�v�Ȃ�)
	void sortByPriority();

	~TaskManager();
};
