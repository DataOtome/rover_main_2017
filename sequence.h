#pragma once
#include <time.h>
#include <list>
#include "task.h"
#include "utils.h"

//�e�X�g�p���
class Testing : public TaskBase
{
protected:
	virtual bool onInit(const struct timespec& time);
	virtual bool onCommand(const std::vector<std::string> args);

	//���̏�ԂɈڍs
	void nextState();

public:
	Testing();
	~Testing();
};

//���̒��ɓ����Ă�����
class Waiting : public TaskBase
{
	struct timespec mStartTime;//��ԊJ�n����
	unsigned int mContinuousLightCount;//�����Ă��邱�Ƃ����m���ꂽ��
protected:
	virtual bool onInit(const struct timespec& time);
	virtual void onUpdate(const struct timespec& time);

	//���̏�ԂɈڍs
	void nextState();

public:
	Waiting();
	~Waiting();
};

//�������Ă�����
class Falling : public TaskBase
{
private:
	struct timespec mStartTime;//��ԊJ�n����
	struct timespec mLastCheckTime;//�O��̃`�F�b�N����
	int mLastPressure;//�O��̋C��
	unsigned int mContinuousPressureCount;//�C����臒l�ȉ��̏�Ԃ���������
	unsigned int mCoutinuousGyroCount;//�p���x��臒l�ȉ��̏�Ԃ���������
protected:
	virtual bool onInit(const struct timespec& time);
	virtual void onUpdate(const struct timespec& time);

	//���̏�ԂɈڍs
	void nextState();
public:
	Falling();
	~Falling();
};

//�p���������(�T�[�{�𓮂����ăp����؂藣��)
class Separating : public TaskBase
{
private:
	struct timespec mLastUpdateTime;//�O��T�[�{�̌������X�V��������
	bool mCurServoState;			//���݂̃T�[�{�̌���(true = 1,false = 0)
	unsigned int mServoCount;		//�T�[�{�̌�����ύX������
protected:
	virtual bool onInit(const struct timespec& time);
	virtual void onUpdate(const struct timespec& time);

	//���̏�ԂɈڍs
	void nextState();
public:
	Separating();
	~Separating();
};

//�S�[���ւ̈ړ���
class Navigating : public TaskBase
{
private:
	struct timespec mLastCheckTime;//�O��̃`�F�b�N����

	//�S�[���ʒu
	VECTOR3 mGoalPos;
	bool mIsGoalPos;

	//�X�^�b�N����֘A
	enum STUCK_MOVE{STUCK_NONE = 0,STUCK_RANDOM,STUCK_BACKWORD,STUCK_FORWORD,STUCK_CAMERA};
	enum STUCK_MOVE mIsStucked;//�X�^�b�N��
	unsigned int mCurStuckMoveCount;//�X�^�b�N�s�����p�̃J�E���^
	struct timespec mLastStuckMoveUpdateTime;//�O��̐i�s�����ύX����@�X�^�b�N

	//GPS���W����v�Z���ꂽ�ߋ����񕪂̈ʒu
	std::list<VECTOR3> mLastPos;
protected:
	virtual bool onInit(const struct timespec& time);
	virtual void onUpdate(const struct timespec& time);
	virtual bool onCommand(const std::vector<std::string> args);

	void stuckMoveRandom();//�X�^�b�N���̈ړ�����
	void stuckMoveCamera(IplImage* pImage);//�J������p�����X�^�b�N���̈ړ�����

	void navigationMove(double distance) const; //�ʏ펞�̈ړ�����
	bool isStuck() const;//�X�^�b�N����

	//���̏�ԂɈڍs
	void nextState();
public:
	void setGoal(const VECTOR3& pos);

	Navigating();
	~Navigating();
};

extern Testing gTestingState;
extern Waiting gWaitingState;
extern Falling gFallingState;
extern Separating gSeparatingState;
extern Navigating gNavigatingState;
