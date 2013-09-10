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
const static int PIN_PWM_A = 5;		//���[�^PWM Right
const static int PIN_PWM_B = 4;
const static int PIN_PULSE_A = 7;	//���[�^�G���R�[�_ Right
const static int PIN_PULSE_B = 0;
const static int PIN_INVERT_MOTOR_A = 3;	//���[�^���]�s�� Right
const static int PIN_INVERT_MOTOR_B = 2;
const static int PIN_BUZZER = 12;			//�u�U�[
const static int PIN_XBEE_SLEEP = 13;		//XBee�X���[�v�s��
const static int PIN_LIGHT_SENSOR = 14;		//Cds�Z���T�s��
const static int PIN_SERVO = 1;				//�T�[�{�s��
const static int PIN_DISTANCE = 6;	//�����Z���T�[

//���[�^�ݒ�
const static int MOTOR_MAX_POWER = 100;
const static double MOTOR_MAX_POWER_CHANGE =(double)5;//���[�^�o�͂̍ő�ω���

//�T�[�{�ݒ�
const static int SERVO_RANGE = 9000;//�p���X�Ԋu
const static int SERVO_MOVABLE_RANGE = 1200;//�p���X���ύX�͈�
const static int SERVO_BASE_VALUE = 910 - SERVO_MOVABLE_RANGE / 2;//�ŏ��p���X��

//�W���C���ݒ�
const static unsigned int GYRO_SAMPLE_COUNT_FOR_CALCULATE_OFFSET = 100;//�h���t�g�덷�␳���ɗp����T���v����

//////////////////////////////////////////////
// �V�[�P���X�n�ݒ�
//////////////////////////////////////////////
const static unsigned int WAITING_LIGHT_COUNT = 3000;//����A���Ō����Ă���Ɣ��肳�ꂽ�Ƃ��ɕ��o����Ƃ��邩
const static unsigned int WAITING_ABORT_TIME =7200;//�����I�ɕ��o����Ƃ��鎞�ԁi�b�j

const static unsigned int FALLING_DELTA_PRESSURE_THRESHOLD = 2;//�O��Ƃ̋C���̍�������ȓ��Ȃ��~���ƃJ�E���g(1�b�Ԋu�ŃT���v�����O)
const static unsigned int FALLING_PRESSURE_COUNT = 5;//�C���ω��ʂ�臒l�ȉ��̏�Ԃ����ꂾ���������璅�n�Ɣ���
const static double FALLING_GYRO_THRESHOLD = 10;//�p���x�����̒l�ȉ��Ȃ��~���ƃJ�E���g
const static unsigned int FALLING_GYRO_COUNT = 1000;//�p���x�̒l��臒l�ȉ��̃T���v�������ꂾ���A�������璅�n�Ɣ���
const static unsigned int FALLING_ABORT_TIME = 1800;//������Ԃ������I�����鎞��
const static unsigned int FALLING_MOTOR_PULSE_THRESHOLD = 1000;//�P�b�ӂ�̃p���X�ω��ʂ�����ȏ�Ȃ�^�C����]���ƃJ�E���g
const static unsigned int FALLING_MOTOR_PULSE_COUNT = 5;//���[�^�p���X�̒l��臒l�ȉ��̃T���v�������ꂾ���A�������璅�n�Ɣ���

const static double SEPARATING_SERVO_INTERVAL = 0.8;//�T�[�{�̌�����ς���Ԋu(�b)
const static unsigned int SEPARATING_SERVO_COUNT = 12;//�T�[�{�̌�����ς����
const static double SEPARATING_PARA_DETECT_THRESHOLD = 0.005;//���̊����ȏ�p���V���[�g�F�����o���ꂽ��p�������݂�����̂Ƃ���

const static double NAVIGATING_GOAL_DISTANCE_THRESHOLD = 3 / 111111.1;//�S�[������Ƃ���S�[������̋���(�x)
const static double NAVIGATING_GOAL_APPROACH_DISTANCE_THRESHOLD = 10 / 111111.1;//�ړ����x����������S�[������̋���(�߂Â����ꍇ�A�s���߂��h�~�̂��ߌ�������)
const static double NAVIGATING_GOAL_APPROACH_POWER_RATE = 0.5;//�S�[���ڋߎ��̑��x(�ő��)
const static double NAVIGATING_DIRECTION_UPDATE_INTERVAL = 5;//�i�s������ύX����Ԋu(�b)
const static double NAVIGATING_MAX_DELTA_DIRECTION = 90;//���̑���ŕ����]������ő�̊p�x
const static double NAVIGATING_STUCK_JUDGEMENT_THRESHOLD = 1.0 / 111111.1; //NAVIGATING_DIRECTION_UPDATE_INTERVAL�̊ԂɈړ���������������臒l�ȉ��Ȃ�X�^�b�N����Ƃ���

const static double WAKING_THRESHOLD = 200;
const static unsigned int WAKING_RETRY_COUNT = 5;

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
//���̑�
//////////////////////////////////////////////
const static double DEGREE_2_METER = 111111.111111;//�����x�Ɋ|����ƃ��[�g���ɕϊ��ł���
const static char INITIALIZE_SCRIPT_FILENAME[] = "initialize.txt";
