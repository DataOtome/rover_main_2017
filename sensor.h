/*
	�Z���T����v���O����

	���[�^�ȊO�̎����E��������擾���郂�W���[���𑀍삵�܂�
	task.h���Q��
*/
#pragma once
#include "task.h"

typedef struct
{
	double x,y,z;
}VECTOR3;

//MPL115A2����f�[�^���擾����N���X
//�C���̒l��hPa�P�ʂ�+-10hPa�̌덷����
class PressureSensor : public TaskBase
{
private:
	float mA0,mB1,mB2,mC12;//�C���v�Z�p�̌W��
	int mPressure;//�Ō�Ɏ擾�����C��
	int mFileHandle;//winringPi i2c�@�̃t�@�C���n���h��

	struct timespec mLastUpdateRequest;//�Ō�ɋC���̍X�V��MPL115A2�Ɏw����������

	float val2float(unsigned int val, int total_bits, int fractional_bits, int zero_pad);
	void requestSample();
protected:
	//�C���Z���T��������
	virtual bool init();
	//�Z���T�̎g�p���I������
	virtual void clean();

	//���Ԋu���ƂɋC�����A�b�v�f�[�g����
	virtual void update();

	//�R�}���h����������
	virtual bool command(const std::vector<std::string> args);
public:
	//�Ō�ɃA�b�v�f�[�g���ꂽ�C����Ԃ�
	int get();

	PressureSensor();
	~PressureSensor();
};

//Navigatron v2����f�[�^���擾����N���X
class GPSSensor : public TaskBase
{
private:
	int mFileHandle;//winringPi i2c�@�̃t�@�C���n���h��
	VECTOR3 mPos;//���W(�o�x�A�ܓx�A���x)
	int mSatelites;//�⑫�����q���̐�
protected:
	//GPS��������
	virtual bool init();
	//�Z���T�̎g�p���I������
	virtual void clean();
	//���݂̍��W���A�b�v�f�[�g����
	virtual void update();
	//�R�}���h����������
	virtual bool command(const std::vector<std::string> args);

public:
	//���݂̍��W���擾����(false��Ԃ����ꍇ�͏ꏊ���s��)
	bool get(VECTOR3& pos);

	GPSSensor();
	~GPSSensor();
};

//L3GD20����f�[�^���擾����N���X
class GyroSensor : public TaskBase
{
private:
	int mFileHandle;//winringPi i2c�@�̃t�@�C���n���h��
	VECTOR3 mRVel;//�p���x
	VECTOR3 mRAngle;//�p�x
	struct timespec mLastSampleTime;
protected:
	//�W���C���Z���T��������
	virtual bool init();
	//�Z���T�̎g�p���I������
	virtual void clean();

	//���Ԋu���ƂɃf�[�^���A�b�v�f�[�g����
	virtual void update();

	//�R�}���h����������
	virtual bool command(const std::vector<std::string> args);
public:
	//�Ō�ɃA�b�v�f�[�g���ꂽ�f�[�^��Ԃ�
	void getRVel(VECTOR3& vel);
	double getRvx();
	double getRvy();
	double getRvz();

	//////////////////////////////////////////////////
	//�p���x����v�Z���ꂽ�p�x����������֐�

	//���݂̊p�x����Ƃ���
	void setZero();

	//���݂̊p�x��Ԃ�(-180�`+180)
	void getRPos(VECTOR3& pos);
	double getRx();
	double getRy();
	double getRz();

	//�����̃x�N�g����(-180�`+180)�͈̔͂ɏC��
	static void normalize(VECTOR3& pos);
	static double normalize(double pos);

	GyroSensor();
	~GyroSensor();
};

//Cds����f�[�^���擾����N���X
class LightSensor : public TaskBase
{
private:
	int mPin;
protected:
	//������
	virtual bool init();
	//�Z���T�̎g�p���I������
	virtual void clean();
	//�R�}���h����������
	virtual bool command(const std::vector<std::string> args);

public:
	//���݂̖��邳���擾����
	bool get();

	LightSensor();
	~LightSensor();
};
extern GyroSensor gGyroSensor;
extern GPSSensor gGPSSensor;
extern PressureSensor gPressureSensor;
extern LightSensor gLightSensor;

