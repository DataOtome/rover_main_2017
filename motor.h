/*
	���[�^���R���g���[�����邽�߂̃N���X
*/

#pragma once
#include "task.h"


//���[�^1���R���g���[������N���X
class Motor
{
private:
        // Pin Numbers
        int mPowerPin;
        int mReversePin;

        // Current Power
        double mCurPower;

		//�o�͕ω��p
		// �ڕW�o��
		int mTargetPower;
		//�o�͕ω��ʌW��
		double mCoeff;
public:
        // Initialize Motor by 2 pins
        bool init(int powPin,int revPin);

		void clean();

		// ���[�^�o�͂��X�V(ratio�͂ق��̃��[�^�Ƃ̌̍��z���p)
		void update(double elapsedSeconds);

        // Controle motor power. Negative value means reverse
		// range: [-MOTOR_MAX_POWER MOTOR_MAX_POWER]
        void set(int pow);

		//���[�^�̏o�͂�ω��������(MOTOR_MAX_POWER_CHANGE)�Ɋ|������W����ݒ�
		//����ɂ���č��E�̃��[�^�Ԃ̌̍����z�����āA�܂������ɉ���/�������ł���
		void setCoeff(double coeff);

        // Get Current power
        int getPower();

        Motor();
        ~Motor();
};

//�G���R�[�_2���Ǘ�����N���X(�V���O���g��)
//wiringPi�̎����̓s����A�����̃C���X�^���X���쐬����Ɛ��������삵�܂���B

/////�Ď��X���b�h�̊Ǘ���������������A�R�[���o�b�N�֐��Ɉ��������Ȃ�����WiringPi�̎���������
class MotorEncoder
{
private:
	int mEncoderPinL, mEncoderPinR;//�G���R�[�_�̃s���ԍ�
	long long mPulseCountL, mPulseCountR;//�p���X��

	MotorEncoder();

	static void pulseLCallback();//�G���R�[�_�[�p���X�����������Ƃ��ɌĂ΂�銄�荞�݊֐�
	static void pulseRCallback();
public:
	static MotorEncoder* getInstance();
	//�p���X�`�F�b�N�J�n
	bool init();
	//�p���X�`�F�b�N�I��
	void clean();

	//���[�^�̃p���X����Ԃ�
	long long getL();
	long long getR();
	
	~MotorEncoder();
};

//���[�^2�ƃZ���T�[��g�ݍ��킹�đ��s����N���X
class MotorDrive : public TaskBase
{
private:
        Motor mMotorL,mMotorR;
		MotorEncoder* mpMotorEncoder;

		typedef enum{//���䃂�[�h
			DRIVE_RATIO, //���V�I�w��Ő��䂷��
			DRIVE_PID,//PID����ɂ��p�x�w��Ő��䂷��
			DRIVE_PID_TURN,
		}DRIVE_MODE;
		DRIVE_MODE mDriveMode;
		int mRatioL,mRatioR;//���V�I��

		//PID�p�p�����[�^
		double mP,mI,mD;//PID�p�����^
		double mDiff1,mDiff2,mDiff3;//PID�p�ߋ��̂���
		double mAngle;//�ڕW�p�x
		double mControlPower;//�O��̑����
		int mDrivePower;//���s���x

		//�Ō�Ƀ��[�^�o�͂��X�V��������
		struct timespec mLastUpdateTime;
protected:
		//������
        virtual bool onInit(const struct timespec& time);
		virtual void onClean();
		//���t���[���̏���
		virtual void onUpdate(const struct timespec& time);

		//�R�}���h��t
		virtual bool onCommand(const std::vector<std::string> args);

		void updatePIDState();
		void updatePIDMove();
		void updatePIDTurn();
public:
		//���[�^�̍��E���ݒ�
		void setRatio(int ratioL,int ratioR);

		//�w�肳�ꂽ�o�͂Ń��[�^����]������((100,100)�̏ꍇ�A���ۂ�pwm�o�͂�setRatio�ɐݒ肵���l�ɂȂ�)
        void drive(int powerL,int powerR);

		//PID����p�p�����[�^��ݒ�
		void set(double p,double i,double d);

		//PID������J�n����(�W���C����Z��]���̊p�x�����ɂȂ�悤�ɐ��䂷��)
		//����ڕW�p�x�́A���݂̌���+angle
		void startPID(double angle,int power);

		//PID����̐���ڕW�p�x��ύX����
		//����ڕW�p�x�́A���݂̐���ڕW�p�x+angle
		void drivePID(double angle,int power);

		//PID����ɂ���Ďw��p�x�ɂ��̏��]����
		void turnPID(double angle,int power);

        MotorDrive();
        ~MotorDrive();
};

extern MotorDrive gMotorDrive;
