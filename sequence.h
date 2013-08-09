#pragma once
#include <time.h>
#include <list>
#include <opencv2/opencv.hpp>
#include <opencv/cvaux.h>
#include <opencv/highgui.h>
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
	enum STEP{STEP_SEPARATE = 0, STEP_PRE_PARA_JUDGE,STEP_PARA_JUDGE,STEP_PARA_DODGE};
	enum STEP mCurStep;
protected:
	virtual bool onInit(const struct timespec& time);
	virtual void onUpdate(const struct timespec& time);

	bool isParaExist(IplImage* pImage);//�摜���Ƀp���V���[�g�����݂��邩�m�F����

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

	//GPS���W����v�Z���ꂽ�ߋ����񕪂̈ʒu
	std::list<VECTOR3> mLastPos;
protected:
	virtual bool onInit(const struct timespec& time);
	virtual void onUpdate(const struct timespec& time);
	virtual bool onCommand(const std::vector<std::string> args);

	void navigationMove(double distance) const; //�ʏ펞�̈ړ�����
	bool isStuck() const;//�X�^�b�N����

	//���̏�ԂɈڍs
	void nextState();
public:
	void setGoal(const VECTOR3& pos);

	Navigating();
	~Navigating();
};

//�Q�E�o����
//���̃^�X�N���L���̊Ԃ̓i�r�Q�[�V�������܂���
class Escaping : public TaskBase
{
	struct timespec mLastUpdateTime;//�O��̍s������̕ω�����
	double mWaitTime;

	enum STEP{STEP_BACKWORD = 0, STEP_PRE_CAMERA, STEP_CAMERA, STEP_CAMERA_WAIT, STEP_RANDOM};
	enum STEP mCurStep;
protected:
	virtual bool onInit(const struct timespec& time);
	virtual void onUpdate(const struct timespec& time);

	void stuckMoveRandom();//�X�^�b�N���̈ړ�����
	double stuckMoveCamera(IplImage* pImage);//�J������p�����X�^�b�N���̈ړ�����
public:
	Escaping();
	~Escaping();
};

//���[�o�[�̎p������
//�p�����䂪��������ƃ^�X�N���I�����܂�
class Waking : public TaskBase
{
	struct timespec mStartTime;//�s���J�n����
	bool mIsWakingStarted;//�p������ɂ����
	double mAngleOnBegin;
protected:
	virtual bool onInit(const struct timespec& time);
	virtual void onUpdate(const struct timespec& time);
	virtual void onClean();
public:
	Waking();
	~Waking();
};

extern Testing gTestingState;
extern Waiting gWaitingState;
extern Falling gFallingState;
extern Separating gSeparatingState;
extern Navigating gNavigatingState;
extern Escaping gEscapingState;
extern Waking gWakingState;