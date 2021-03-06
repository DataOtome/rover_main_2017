/*
	アクチュエータ制御プログラム

	モータ以外の実世界に働きかけるモジュールを操作します
	task.hも参照
*/

#pragma once
#include "task.h"

// ブザー制御クラス
class Buzzer : public TaskBase
{
private:
	int mPin;
	int mOnPeriodMemory;	//鳴らす時間を保持
	int mOnPeriod;			//0以上なら鳴らす、負なら鳴らさない
	int mOffPeriodMemory;	//鳴らさない時間を保持
	int mOffPeriod;			//0以上なら鳴らさない
	int mCount;				//ブザーを鳴らす回数
protected:
	virtual bool onInit(const struct timespec& time);
	virtual void onClean();
	virtual bool onCommand(const std::vector<std::string>& args);
	virtual void onUpdate(const struct timespec& time);
	virtual void restart();

public:
	//特に指定しない場合のブザーの間隔
	const static int DEFAULT_OFF_PERIOD = 500;
	
	//ブザーをperiod[ms]だけ鳴らす(長さは厳密ではありません！)
	void start(int period);

	//(鳴らす時間[ms], 鳴らす回数) ブザーを複数回数鳴らしたい場合に使用
	void start(int on_period, int count);	

	//(鳴らす時間[ms], 鳴らさない時間[ms], 鳴らす回数)
	void start(int on_period, int off_period, int count);	
	//ブザーを止める
	void stop();

	Buzzer();
	~Buzzer();
};

// パラシュートサーボ制御クラス(ソフトウェアPWMを使う)
class ParaServo : public TaskBase
{
private:
	const static int SERVO_MIN_RANGE = 6;	//そのうちconstants.hに移す
	const static int SERVO_MAX_RANGE = 25;	//そのうちconstants.hに移す
	const static int SERVO_RANGE = 100;		//そのうちconstants.hに移す

	//POSITION_RELEASE: ピンが抜ける位置, POSITION_HOLD: ピンが刺さった状態の位置
	//2015 high-ballバージョン
	enum POSITION {POSITION_RELEASE = 6, POSITION_HOLD = 25};

	int mPin;
protected:
	virtual bool onInit(const struct timespec& time);
	virtual void onClean();
	virtual bool onCommand(const std::vector<std::string>& args);

	//サーボを指定されたangle[0-SERVO_MAX_RANGE]になるように制御を開始する
	//(※2014verはSoftware PWM使用のため細かい角度の調整は難しい)
	virtual void start(int angle);
	virtual void start(POSITION p);
public:
	//サーボの制御を終了する
	void stop();

	void moveRelease();//パラシュート切り離し
	void moveHold();//ピンが刺さった状態の位置に移動

	ParaServo();
	~ParaServo();
};

// スタビサーボ制御クラス(ハードウェアPWMを使う)
class StabiServo : public TaskBase
{
private:
	int mPin;
protected:
	virtual bool onInit(const struct timespec& time);
	virtual void onClean();
	virtual bool onCommand(const std::vector<std::string>& args);
public:
	//サーボを指定されたangle[0-1]になるように制御を開始する
	void start(double angle);
	//サーボの制御を終了する
	void stop();
	//サーボをしまう
	void close();

	StabiServo();
	~StabiServo();
};

// カメラサーボ制御クラス(ハードウェアPWMを使う)　仲田
/*
class CameraServo : public TaskBase
{
private:
	int mPin;
protected:
	virtual bool onInit(const struct timespec& time);
	virtual void onClean();
	virtual bool onCommand(const std::vector<std::string>& args);
public:
	//サーボを指定されたangle[0-1]になるように制御を開始する
	void start(double angle);
	//サーボの制御を終了する
	void stop();
	//サーボをしまう
	void close();

	CameraServo();
	~CameraServo();
};
*/

//カメラサーボ制御クラス(ソフトPWM)
//ParaServoの丸移しです
//7/19の審査会のためのしのぎです
class SoftCameraServo : public TaskBase
{
private:
	const static int SERVO_MIN_RANGE = 6;	//そのうちconstants.hに移す
	const static int SERVO_MAX_RANGE = 25;	//そのうちconstants.hに移す
	const static int SERVO_RANGE = 100;		//そのうちconstants.hに移す

	//POSITION_RELEASE: ピンが抜ける位置, POSITION_HOLD: ピンが刺さった状態の位置
	enum POSITION {POSITION_RELEASE = 25, POSITION_HOLD =6};

	int mPin;
protected:
	virtual bool onInit(const struct timespec& time);
	virtual void onClean();
	virtual bool onCommand(const std::vector<std::string>& args);

	//サーボを指定されたangle[0-SERVO_MAX_RANGE]になるように制御を開始する
	//(※2014verはSoftware PWM使用のため細かい角度の調整は難しい)
	
public:
	//サーボの制御を終了する
	void stop();
	virtual void start(int angle);
	virtual void start(POSITION p);
	void moveRelease();//パラシュート切り離し
	void moveHold();//ピンが刺さった状態の位置に移動

	SoftCameraServo();
	~SoftCameraServo();
};

// 後ろスタビ制御クラス(ソフトウェアPWM)
class BackStabiServo : public TaskBase
{
private:
	const static int SERVO_MIN_RANGE = 6;	//そのうちconstants.hに移す
	const static int SERVO_MAX_RANGE = 25;	//そのうちconstants.hに移す
	const static int SERVO_RANGE = 100;		//そのうちconstants.hに移す

	//POSITION_RELEASE: ピンが抜ける位置, POSITION_HOLD: ピンが刺さった状態の位置
	enum POSITION {POSITION_RELEASE = 14, POSITION_HOLD = 25,POSITION_GO=6};

	int mPin;
protected:
	virtual bool onInit(const struct timespec& time);
	virtual void onClean();
	virtual bool onCommand(const std::vector<std::string>& args);

	//サーボを指定されたangle[0-SERVO_MAX_RANGE]になるように制御を開始する
	//(※2014verはSoftware PWM使用のため細かい角度の調整は難しい)
	virtual void start(int angle);
	virtual void start(POSITION p);
public:
	//サーボの制御を終了する
	void stop();

	void moveRelease();//パラシュート切り離し
	void moveHold();//ピンが刺さった状態の位置に移動
	void moveGo();
	BackStabiServo();
	~BackStabiServo();
};

//// XBeeスリープ制御クラス
//class XBeeSleep : public TaskBase
//{
//private:
//	int mPin;
//protected:
//	virtual bool onInit(const struct timespec& time);
//	virtual void onClean();
//	virtual bool onCommand(const std::vector<std::string>& args);
//
//public:
//	void setState(bool sleep);
//
//	XBeeSleep();
//	~XBeeSleep();
//};

extern Buzzer gBuzzer;
extern ParaServo gParaServo;
extern StabiServo gStabiServo;
extern BackStabiServo gBackStabiServo;
//extern XBeeSleep gXbeeSleep;
extern SoftCameraServo gSoftCameraServo;
//extern CameraServo gCameraServo
