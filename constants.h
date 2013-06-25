/*
	�e��萔
*/
#pragma once

//�{�Ԃ̓R�����g�A�E�g���邱�ƁI�I�iCtrl-C�ɂ��v���O�����I���������ɂȂ�܂��j
#define _DEBUG 1

//�ڍׂȃ��O�\�����K�v�ȏꍇ�͂����Ă�������
//#define _LOG_DETAIL 1



//////////////////////////////////////////////
// �n�[�h�E�F�A�n�ݒ�
//////////////////////////////////////////////
//�s���ԍ�(WiringPi�̃s���ԍ��AGPIO�Ƃ͈Ⴂ�܂�)
const static int PIN_PWM_A = 4;		//���[�^PWM Right
const static int PIN_PWM_B = 5;
const static int PIN_PULSE_A = 0;	//���[�^�G���R�[�_ Right
const static int PIN_PULSE_B = 7;
const static int PIN_INVERT_MOTOR_A = 3;	//���[�^���]�s�� Right
const static int PIN_INVERT_MOTOR_B = 2;
const static int PIN_BUZZER = 12;			//�u�U�[
const static int PIN_XBEE_SLEEP = 13;		//XBee�X���[�v�s��
const static int PIN_LIGHT_SENSOR = 14;		//Cds�Z���T�s��
const static int PIN_SERVO = 1;				//�T�[�{�s��

//���[�^�ݒ�
const static int MOTOR_MAX_POWER = 100;
const static double MOTOR_MAX_POWER_CHANGE = MOTOR_MAX_POWER * 2;//���[�^�o�͂�1�b������̍ő�ω���

//�T�[�{�ݒ�
const static int SERVO_RANGE = 9000;//�p���X�Ԋu
const static int SERVO_MOVABLE_RANGE = 1200;//�p���X���ύX�͈�
const static int SERVO_BASE_VALUE = 910 - SERVO_MOVABLE_RANGE / 2;//�ŏ��p���X��

//////////////////////////////////////////////
// �V�[�P���X�n�ݒ�
//////////////////////////////////////////////
const static unsigned int WAITING_LIGHT_COUNT = 100;//����A���Ō����Ă���Ɣ��肳�ꂽ�Ƃ��ɕ��o����Ƃ��邩
const static unsigned int WAITING_ABORT_TIME = 3600;//�����I�ɕ��o����Ƃ��鎞�ԁi�b�j

const static unsigned int FALLING_DELTA_PRESSURE_THRESHOLD = 4;//�O��Ƃ̋C���̍�������ȓ��Ȃ��~���ƃJ�E���g(1�b�Ԋu�ŃT���v�����O)
const static unsigned int FALLING_PRESSURE_COUNT = 5;//�C���ω��ʂ�臒l�ȉ��̏�Ԃ����ꂾ���������璅�n�Ɣ���
const static double FALLING_GYRO_THRESHOLD = 0.5;//�p���x�����̒l�ȉ��Ȃ��~���ƃJ�E���g
const static unsigned int FALLING_GYRO_COUNT = 100;//�p���x�̒l��臒l�ȉ��̃T���v�������ꂾ���A�������璅�n�Ɣ���
const static unsigned int FALLING_ABORT_TIME = 1800;//������Ԃ������I�����鎞��

const static double SEPARATING_SERVO_INTERVAL = 0.8;//�T�[�{�̌�����ς���Ԋu(�b)
const static unsigned int SEPARATING_SERVO_COUNT = 5;//�T�[�{�̌�����ς����

const static double NAVIGATING_GOAL_DISTANCE_THRESHOLD = 3 / 111111.1;//�S�[������Ƃ���S�[������̋���(�x)
const static double NAVIGATING_GOAL_APPROACH_DISTANCE_THRESHOLD = 10 / 111111.1;//�ړ����x����������S�[������̋���(�߂Â����ꍇ�A�s���߂��h�~�̂��ߌ�������)
const static double NAVIGATING_GOAL_APPROACH_POWER_RATE = 0.5;//�S�[���ڋߎ��̑��x(�ő��)
const static double NAVIGATING_DIRECTION_UPDATE_INTERVAL = 5;//�i�s������ύX����Ԋu(�b)

//////////////////////////////////////////////
//�^�X�N�n�ݒ�
//////////////////////////////////////////////
//�^�X�N�D�揇��(�Ⴂ�قǐ�Ɏ��s�����)
const static unsigned int TASK_PRIORITY_SENSOR = 10;
const static unsigned int TASK_PRIORITY_MOTOR = 100;
const static unsigned int TASK_PRIORITY_COMMUNICATION = 0;
const static unsigned int TASK_PRIORITY_ACTUATOR = 10000;
const static unsigned int TASK_PRIORITY_SEQUENCE = 1000;
//�^�X�N���s�Ԋu(�Ⴂ�قǑ������s�����)
const static unsigned int TASK_INTERVAL_GYRO = 0;
const static unsigned int TASK_INTERVAL_SENSOR = 10;
const static unsigned int TASK_INTERVAL_MOTOR = 0;
const static unsigned int TASK_INTERVAL_COMMUNICATION = 1;
const static unsigned int TASK_INTERVAL_ACTUATOR = 0;
const static unsigned int TASK_INTERVAL_SEQUENCE = 0;

//////////////////////////////////////////////
//���̑����l
//////////////////////////////////////////////
const static double degree2meter = 111111.111111;//�����x�Ɋ|����ƃ��[�g���ɕϊ��ł���