#pragma once
#include <time.h>
#include <list>
#include <opencv2/opencv.hpp>
#include <opencv/cvaux.h>
#include <opencv/highgui.h>
#include "task.h"

class ImageProc : public TaskBase
{
protected:
	virtual bool onCommand(const std::vector<std::string> args);
public:
	bool isParaExist(IplImage* pImage);//�摜���Ƀp���V���[�g�����݂��邩�m�F����
	bool isWadachiExist(IplImage* pImage);//�Q���O���m
	bool isSky(IplImage* pImage);//��̊��������ȏ�Ȃ�^
	int wadachiExiting(IplImage* pImage);//-1:�� 0:���i 1:�E

	ImageProc();
	~ImageProc();
};

extern ImageProc gImageProc;
