/*
	�e��萔
*/
#pragma once

//�{�Ԃ̓R�����g�A�E�g���邱�ƁI�I�iCtrl-C�ɂ��v���O�����I���������ɂȂ�܂��j
#define _DEBUG 1

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
const static double MOTOR_MAX_POWER_CHANGE = 0.5;//���[�^�o�͂̍ő�ω���
const static int MOTOR_MAX_POWER = 100;

//�T�[�{�ݒ�
const static int SERVO_RANGE = 9000;
const static int SERVO_MOVABLE_RANGE = 1200;
const static int SERVO_BASE_VALUE = 900 - SERVO_MOVABLE_RANGE / 2;


//////////////////////////////////////////////
//�^�X�N�n�ݒ�
//////////////////////////////////////////////
//�^�X�N�D�揇��(�Ⴂ�قǐ�Ɏ��s�����)
const static unsigned int TASK_PRIORITY_SENSOR = 10;
const static unsigned int TASK_PRIORITY_MOTOR = 100;
const static unsigned int TASK_PRIORITY_COMMUNICATION = 0;
const static unsigned int TASK_PRIORITY_ACTUATOR = 10000;
//�^�X�N���s�Ԋu(�Ⴂ�قǑ������s�����)
const static unsigned int TASK_INTERVAL_GYRO = 0;
const static unsigned int TASK_INTERVAL_SENSOR = 10;
const static unsigned int TASK_INTERVAL_MOTOR = 0;
const static unsigned int TASK_INTERVAL_COMMUNICATION = 1;
const static unsigned int TASK_INTERVAL_ACTUATOR = 0;
