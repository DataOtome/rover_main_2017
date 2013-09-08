#include "image_proc.h"
#include "constants.h"
#include "utils.h"
#include "actuator.h"
#include "sensor.h"

ImageProc gImageProc;

bool ImageProc::isParaExist(IplImage* src)
{
	if(src == NULL)
	{
		Debug::print(LOG_SUMMARY, "Para detection: Unable to get Image\r\n");
		return true;
	}
	unsigned long pixelCount = 0;
	int x = 0, y = 0;
	uchar H, S, V;
	uchar minH, minS, minV, maxH, maxS, maxV;
    
	CvPixelPosition8u pos_src;
	uchar* p_src;
	IplImage* tmp = cvCreateImage(cvGetSize(src), IPL_DEPTH_8U, 3);
    
	//HSV�ɕϊ�
	cvCvtColor(src, tmp, CV_BGR2HSV);
    
	CV_INIT_PIXEL_POS(pos_src, (unsigned char*) tmp->imageData,
                      tmp->widthStep,cvGetSize(tmp), x, y, tmp->origin);
    
	//orange para
	minH = 0;	maxH = 33;
	minS = 170;	maxS = 255;
	minV = 100;	maxV = 255;

	//yellow para
	/*minH = 20;	maxH = 33;
	minS = 70;	maxS = 255;
	minV = 100;	maxV = 255;*/

	for(y = 0; y < tmp->height; y++) {
		for(x = 0; x < tmp->width; x++) {
			p_src = CV_MOVE_TO(pos_src, x, y, 3);
            
			H = p_src[0];
			S = p_src[1];
			V = p_src[2];
            
			if( minH <= H && H <= maxH &&
               minS <= S && S <= maxS &&
               minV <= V && V <= maxV
               ) {
				++pixelCount;//臒l�͈͓��̃s�N�Z�������J�E���g
			}
		}
	}

	double ratio = (double)pixelCount / tmp->height / tmp->width;
	cvReleaseImage(&tmp);
	Debug::print(LOG_SUMMARY, "Para ratio: %f\r\n",ratio);
	return ratio > SEPARATING_PARA_DETECT_THRESHOLD;
}
bool ImageProc::isSky(IplImage* src)
{
	const static double SKY_DETECT_THRESHOLD = 0.9;
	if(src == NULL)
	{
		Debug::print(LOG_SUMMARY, "Sky detection: Unable to get Image\r\n");
		return false;
	}
	unsigned long pixelCount = 0;
	int x = 0, y = 0;
	uchar H, S, V;
	uchar minH, minS, minV, maxH, maxS, maxV;
    
	CvPixelPosition8u pos_src;
	uchar* p_src;
	IplImage* tmp = cvCreateImage(cvGetSize(src), IPL_DEPTH_8U, 3);
    
	//HSV�ɕϊ�
	cvCvtColor(src, tmp, CV_BGR2HSV);
    
	CV_INIT_PIXEL_POS(pos_src, (unsigned char*) tmp->imageData,
                      tmp->widthStep,cvGetSize(tmp), x, y, tmp->origin);
    
	//臒l
	minH = 100;	maxH = 145;
	minS = 0;	maxS = 255;
	minV = 50;	maxV = 255;

	for(y = 0; y < tmp->height; y++) {
		for(x = 0; x < tmp->width; x++) {
			p_src = CV_MOVE_TO(pos_src, x, y, 3);
            
			H = p_src[0];
			S = p_src[1];
			V = p_src[2];
            
			if( (minH <= H && H <= maxH &&
               minS <= S && S <= maxS &&
               minV <= V && V <= maxV)
               || (H == 0 && V > 200)
               ) {
				++pixelCount;//臒l�͈͓��̃s�N�Z�������J�E���g
			}
		}
	}

	double ratio = (double)pixelCount / tmp->height / tmp->width;
	cvReleaseImage(&tmp);
	Debug::print(LOG_SUMMARY, "Sky ratio: %f\r\n",ratio);
	return ratio >= SKY_DETECT_THRESHOLD;
}
bool ImageProc::isWadachiExist(IplImage* pImage)
{
	/*
	if(pImage == NULL)
	{
		Debug::print(LOG_SUMMARY, "Wadachi predicting: Unable to get Image\r\n");
		return false;
	}
	const static int DIV_NUM = 20;
	const static int PIC_SIZE_W = 320;
	const static int PIC_SIZE_H = 240;
	const static int DELETE_H_THRESHOLD = 80;
	const static double RATE = 1.7;
	const static double PIC_CUT_RATE = 0.55;
	const static int DIV_HOR_NUM = 5;

	IplImage *src_img, *dst_img1, *tmp_img;
	double risk[DIV_NUM], risk_rate[DIV_NUM];
	CvSize pic_size = cvSize(PIC_SIZE_W, PIC_SIZE_H);

	src_img = cvCreateImage(pic_size, IPL_DEPTH_8U, 1);
	cvCvtColor(pImage, src_img, CV_BGR2GRAY);
		
	cvRectangle(src_img, cvPoint(0, 0),cvPoint(PIC_SIZE_W, PIC_SIZE_H * PIC_CUT_RATE) ,cvScalar(0), CV_FILLED, CV_AA);

	tmp_img = cvCreateImage (cvGetSize (src_img), IPL_DEPTH_16S, 1);
	dst_img1 = cvCreateImage (cvGetSize (src_img), IPL_DEPTH_8U, 1);
		
	// Sobel�t�B���^X����
	cvSobel (src_img, tmp_img, 1, 0, 3);
	cvConvertScaleAbs (tmp_img, dst_img1);

	// 2�l��
	cvThreshold (dst_img1, dst_img1, DELETE_H_THRESHOLD, 255, CV_THRESH_BINARY);

	// ���������̃G�b�WSum
	int height = src_img->height / DIV_NUM;
	double risk_sum = 0, risk_ave = 0;
	bool wadachi_find = false;

	for(int i = 0;i < DIV_NUM;++i)
	{
		cvSetImageROI(dst_img1, cvRect(0, height * i, src_img->width, height));//Set image part
		risk_sum += risk[i] = sum(cv::cvarrToMat(dst_img1))[0];
		cvResetImageROI(dst_img1);//Reset image part (normal)
	}

	// ����
	risk_ave = risk_sum / DIV_NUM;
	if(risk_ave == 0)risk_ave = 1;

	if(risk_sum == 0)risk_sum = 1;
	// ����
	for(int i=DIV_NUM - 1; i>=0; --i){
		risk_rate[i] = risk[i] / risk_sum;
	}

	//Draw graph
	for(int i= DIV_NUM-1; i>0; --i){
		if(i>0){
			if(risk_rate[i] == 0)risk_rate[i] = 1;
			if(risk_rate[i-1] / risk_rate[i] > RATE && risk_rate[i] > risk_ave){
				wadachi_find = true;
			}
		}
	}

	if(wadachi_find){
		Debug::print(LOG_SUMMARY, "Wadachi Found\r\n");
		gBuzzer.start(100);
	}
	else{
		Debug::print(LOG_SUMMARY, "Wadachi Not Found\r\n");
	}

	cvReleaseImage (&src_img);
	cvReleaseImage (&dst_img1);
	cvReleaseImage (&tmp_img);

	return wadachi_find;
	*/
	
	if(pImage == NULL)
	{
		Debug::print(LOG_SUMMARY, "Wadachi predicting: Unable to get Image\r\n");
		return false;
	}
	const static int DIV_NUM = 15;
	const static int PIC_SIZE_W = 320;
	const static int PIC_SIZE_H = 240;
	const static int DELETE_H_THRESHOLD = 80;
	const static double RATE = 1.6;
	const static int DIV_HOR_NUM = 5;
	const static double RISK_AVE_RATE = 0.5;

	IplImage *src_img, *dst_img1, *tmp_img;
	double risk[DIV_NUM], risk_rate[DIV_NUM];
	CvSize pic_size = cvSize(PIC_SIZE_W, PIC_SIZE_H);

	src_img = cvCreateImage(pic_size, IPL_DEPTH_8U, 1);
	cvCvtColor(pImage, src_img, CV_BGR2GRAY);
		
	tmp_img = cvCreateImage (cvGetSize (src_img), IPL_DEPTH_16S, 1);
	dst_img1 = cvCreateImage (cvGetSize (src_img), IPL_DEPTH_8U, 1);
		
	// Sobel�t�B���^X����
	cvSobel (src_img, tmp_img, 1, 0, 3);
	cvConvertScaleAbs (tmp_img, dst_img1);

	// 2�l��
	cvThreshold (dst_img1, dst_img1, DELETE_H_THRESHOLD, 255, CV_THRESH_BINARY);
	
	//��J�b�g
	CvPoint pt[(DIV_HOR_NUM+1)*2+1];
	cutSky(pImage, dst_img1, pt);

	// ���������̃G�b�WSum
	int height = src_img->height / DIV_NUM;
	double risk_sum = 0, risk_ave = 0;
	bool wadachi_find = false;

	for(int i = 0;i < DIV_NUM;++i)
	{
		cvSetImageROI(dst_img1, cvRect(0, height * i, src_img->width, height));//Set image part
		risk_sum += risk[i] = sum(cv::cvarrToMat(dst_img1))[0];
		cvResetImageROI(dst_img1);//Reset image part (normal)
	}

	// ����
	risk_ave = risk_sum / DIV_NUM;
	if(risk_ave == 0)risk_ave = 1;

	if(risk_sum == 0)risk_sum = 1;
	// ����
	for(int i=DIV_NUM - 1; i>=0; --i){
		risk_rate[i] = risk[i] / risk_sum;
	}

	//Draw graph
	for(int i= DIV_NUM-1; i>0; --i){
		if(i>0){
			if(risk_rate[i] == 0)risk_rate[i] = 1;
			if(risk_rate[i-1] / risk_rate[i] > RATE && risk[i] > risk_ave * RISK_AVE_RATE){
				wadachi_find = true;
			}
		}
	}

	Debug::print(LOG_SUMMARY, "ave : %f\n",risk_ave * RISK_AVE_RATE);
	Debug::print(LOG_SUMMARY, "risk : \n");
	for(int i=0; i<DIV_NUM; i++){
		Debug::print(LOG_SUMMARY, "%f\n",risk[i]);
	}

	if(wadachi_find){
		Debug::print(LOG_SUMMARY, "Wadachi Found\r\n");
		gBuzzer.start(100);
	}
	else{
		Debug::print(LOG_SUMMARY, "Wadachi Not Found\r\n");
	}

	cvReleaseImage (&src_img);
	cvReleaseImage (&dst_img1);
	cvReleaseImage (&tmp_img);

	return wadachi_find;
}int ImageProc::wadachiExiting(IplImage* pImage)
{
	const static int DIV_HOR_NUM = 5;
	const static int MEDIAN = 5;
	const static int DELETE_H_THRESHOLD = 50;
	const static int THRESHOLD_COUNT = 3;// �m�C�Y���̃u���b�N���̉���
	const static double THRESHOLD_MIN = 700000;// �m�C�Y���̉���

	if(pImage == NULL)
	{
		Debug::print(LOG_SUMMARY, "Escaping: Unable to get Image for Camera Escaping!\r\n");
		return (rand() % 2 != 0) ? 1 : -1;//�����_���Ői�s����������
	}

	CvSize size = cvSize(pImage->width,pImage->height);

	IplImage *pResized = cvCreateImage(size, IPL_DEPTH_8U, 3);
	cvResize(pImage, pResized, CV_INTER_LINEAR);

	// Median Filter
	IplImage *pMedian = cvCreateImage(size,IPL_DEPTH_8U, 3);
	cvSmooth (pResized, pMedian, CV_MEDIAN, MEDIAN, 0, 0, 0);
	// Sobel Filter
	IplImage *pGray = cvCreateImage(size, IPL_DEPTH_8U, 1);
	cvCvtColor(pMedian, pGray, CV_RGB2GRAY);
	IplImage *pSobel = cvCreateImage(size, IPL_DEPTH_16S, 1);
	cvSobel(pGray, pSobel, 1, 0, 3);
	// binarization
	IplImage *pBin = cvCreateImage(size, IPL_DEPTH_8U, 1);
	cvConvertScaleAbs(pSobel, pBin);
	cvThreshold(pBin, pBin, DELETE_H_THRESHOLD, 255, CV_THRESH_BINARY);

	CvPoint pt[(DIV_HOR_NUM+1)*2+1];
	cutSky(pResized,pBin,pt);

	//Sum
	double risk[DIV_HOR_NUM], new_risk[DIV_HOR_NUM];
	double risk_sum = 0;
	int div_width  = size.width  / DIV_HOR_NUM;
	for(int i=0; i<DIV_HOR_NUM; ++i){
		// set image part
		cvSetImageROI(pBin, cvRect(div_width * i, 0, div_width, size.height));
		risk_sum += risk[i] = sum(cv::cvarrToMat(pBin))[0];
		// reset image part (normal)
		cvResetImageROI(pBin);
	}
    
	// calculate 5 heights
	double heights[DIV_HOR_NUM];
	for(int i=0; i<DIV_HOR_NUM; ++i){
		heights[i] = size.height - (pt[2*i+1].y + pt[2*(i+1)+1].y) / 2;
	}
	
	// normalize risk
	double ave_heights = 0;
	for(int i=0; i<DIV_HOR_NUM; ++i){
		ave_heights += heights[i];
	}
	ave_heights /= 5;
	for(int i=0; i<DIV_HOR_NUM; ++i){
		new_risk[i] = risk[i] * ave_heights / heights[i];
	}	
	for(int i=0; i<DIV_HOR_NUM; ++i){
		Debug::print(LOG_SUMMARY, "[%d] risk %.0f : height %.5f -> risk %.0f\r\n", i, risk[i], heights[i], new_risk[i]);
	}
	
	int minNum = 0;
	for(int i=1; i<DIV_HOR_NUM; ++i){
		if(new_risk[i] < new_risk[minNum]){
			minNum = i;
		}
	}
    
	cvReleaseImage(&pResized);
	cvReleaseImage(&pGray);
	cvReleaseImage(&pSobel);
	cvReleaseImage(&pBin);

	// ��������
	/*if(count >= THRESHOLD_COUNT){
		Debug::print(LOG_SUMMARY, "Go straight\r\n");
		return 0;
	}*/
	
/*	if(0 < minNum && minNum < DIV_HOR_NUM-1){
			Debug::print(LOG_SUMMARY, "Go straight\r\n");
			return 0;
	}else{
		int ave_left = 0, ave_right = 0;
		for(int i=0; i<DIV_HOR_NUM; ++i){
			if(i <= DIV_HOR_NUM/2){
				ave_left += new_risk[i];
			}
			if(i >= DIV_HOR_NUM/2){
				ave_right += new_risk[i];
			}
		}
		ave_left /= 3; ave_right /= 3;
		if(ave_left < ave_right){
			Debug::print(LOG_SUMMARY, "Turn left\r\n");
			return -1;
		}else{
			Debug::print(LOG_SUMMARY, "Turn right\r\n");
			return 1;
		}
	}*/
	if(new_risk[0] < new_risk[DIV_HOR_NUM-1]){
		Debug::print(LOG_SUMMARY, "Turn left\r\n");
		return -1;
	}
	else{
		Debug::print(LOG_SUMMARY, "Turn right\r\n");
		return 1;
	}

}
bool ImageProc::onCommand(const std::vector<std::string> args)
{
	if(args.size() == 2)
	{
		if(args[1].compare("predict") == 0)
		{
			isWadachiExist(gCameraCapture.getFrame());
			return true;
		}else if(args[1].compare("exit") == 0)
		{
			wadachiExiting(gCameraCapture.getFrame());
			return true;
		}else if(args[1].compare("sky") == 0)
		{
			isSky(gCameraCapture.getFrame());
			return true;
		}else if(args[1].compare("para") == 0)
		{
			isParaExist(gCameraCapture.getFrame());
			return true;
		}
		return false;
	}
	Debug::print(LOG_SUMMARY, "image [predict/exit/sky/para]  : test program\r\n");
	return true;
}
void ImageProc::cutSky(IplImage* pSrc,IplImage* pDest, CvPoint* pt)
{
	const static int DIV_VER_NUM = 80;                 // �c�ɓǂރs�N�Z����
	const static int DIV_HOR_NUM = 5;                   // ����ɗp�����
	const static int FIND_FLAG = 5;                     // ��̊J�n������
	const static int DELETE_H_THRESHOLD_LOW = 150;		// ���H�i�F���j�͈͉̔���
	const static int DELETE_H_THRESHOLD_HIGH = 270;		// ���H�i�F���j�͈̔͏��
	const static int DELETE_V_THRESHOLD_LOW = 100;      // ���V�i�F���j�͈͉̔���
	const static int DELETE_V_THRESHOLD_HIGH = 200;     // ���V�i�F���j�͈̔͏��
	CvSize size = cvSize(pSrc->width,pSrc->height);
	int div_width  = size.width  / DIV_HOR_NUM;
	int div_height = size.height / DIV_VER_NUM;
	
	//BGR->HSV
	IplImage* pHsv = cvCreateImage(size,IPL_DEPTH_8U, 3); //HSV(8bits*3channels)
	cvCvtColor(pSrc, pHsv,CV_BGR2HSV);
	
	bool flag;                       // ��t���O
	pt[0] = cvPoint(0,0);            //����[�̍��W���i�[

	for(int i = 0; i <= DIV_HOR_NUM; ++i)
	{
		int find_count = 0;	// ��s�N�Z���̐��̃J�E���g�p

		// �󂪔��肳��Ȃ��������͏�[�̍��W���i�[
		pt[2*i+1] = cvPoint(i*div_width, 0);
		pt[2*i+2] = cvPoint((i+1)*div_width, 0);

		for(int j = DIV_VER_NUM-1; j >= 0; --j){
			int x = i * div_width;  // �擾�ʒu��x���W
			int y = j * div_height; // �擾�ʒu��y���W
			if(x == 0) x = 1;       // �摜���[�𐳏�ɏ������邽��
			
			// H�l��V�l�擾
			int value_h = (unsigned char)pHsv->imageData[pHsv->widthStep * y + (x - 1) * 3    ] * 2;   // H
			int value_v = (unsigned char)pHsv->imageData[pHsv->widthStep * y + (x - 1) * 3 + 2];     // V
			
			// �󔻒�
			flag = true;
			if(value_h < DELETE_H_THRESHOLD_LOW || DELETE_H_THRESHOLD_HIGH < value_h) //�������l�O
				flag = false;
			if(value_v < DELETE_V_THRESHOLD_LOW) //�Â���
				flag = false;
			if(value_h == 0 && value_v > DELETE_V_THRESHOLD_HIGH) //�����Ė��邢
				flag = true;

			//printf("h=%3d v=%3d %d \n", value_h, value_v, flag);

			if(flag){ //��]�[�������Apt�z��ɍ��W���i�[
				find_count++;
				if(find_count > FIND_FLAG){
					pt[2*i+1] = cvPoint(x, y+FIND_FLAG*div_height); // ��̊J�n���W
					pt[2*i+2] = cvPoint((i+1)*div_width, 0);        // ���ɏ��������̏�[���W
					//printf("pt[%d]=(%2d, %2d)\n", 2*i, pt[2*i].x, pt[2*i].y);
					//printf("pt[%d]=(%2d, %2d)\n", 2*i+1, pt[2*i+1].x, pt[2*i+1].y);
					break;
				}
			}
			else{
				find_count = 0; // ��łȂ����߃J�E���g�����Z�b�g
			}

		}
	
		//cvShowImage( "origin", pImage );
		//cvWaitKey(0);
	}

	// ��J�b�g
	int npts[1] = {4};	// �h��Ԃ��}�`�̒��_��
	CvPoint pts[4];
	for(int i=0; i<DIV_HOR_NUM; ++i){
		pts[0] = pt[2*i];
		pts[1] = pt[2*i+1];
		pts[2] = pt[2*(i+1)+1];
		pts[3] = pt[2*(i+1)];
		CvPoint *ptss[1] = {&pts[0]};
		cvFillPoly(pDest, ptss, npts, 1, cvScalar(0), CV_AA, 0);
	}
	cvReleaseImage(&pHsv);
}
ImageProc::ImageProc()
{
	setName("image");
	setPriority(UINT_MAX,UINT_MAX);
}
ImageProc::~ImageProc()
{
}
