/*
	�A�N�`���G�[�^����v���O����

	���[�^�ȊO�̎����E�ɓ��������郂�W���[���𑀍삵�܂�
	task.h���Q��
*/

#pragma once
#include "task.h"

// �u�U�[����N���X
class Buzzer : public TaskBase
{
private:
	int mPin;
	int mPeriod;//0�ȏ�Ȃ�炷�A���Ȃ�炳�Ȃ�
protected:
	virtual bool onInit();
	virtual void onClean();
	virtual bool onCommand(const std::vector<std::string> args);
	virtual void onUpdate();

public:
	//�u�U�[��period[ms]�����炷(�����͌����ł͂���܂���I)
	void start(int period);
	//�u�U�[���~�߂�
	void stop();

	Buzzer();
	~Buzzer();
};

// �T�[�{����N���X(�n�[�h�E�F�APWM���g��)
class Servo : public TaskBase
{
private:
	int mPin;
protected:
	virtual bool onInit();
	virtual void onClean();
	virtual bool onCommand(const std::vector<std::string> args);
public:
	//�T�[�{���w�肳�ꂽangle[0-1]�ɂȂ�悤�ɐ�����J�n����
	void start(double angle);
	//�T�[�{�̐�����I������
	void stop();

	Servo();
	~Servo();
};

// XBee�X���[�v����N���X
class XBeeSleep : public TaskBase
{
private:
	int mPin;
protected:
	virtual bool onInit();
	virtual void onClean();
	virtual bool onCommand(const std::vector<std::string> args);

public:
	void setState(bool sleep);

	XBeeSleep();
	~XBeeSleep();
};

extern Buzzer gBuzzer;
extern Servo gServo;
extern XBeeSleep gXbeeSleep;

