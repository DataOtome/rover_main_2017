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

	cvReleaseImage(&tmp);
	double ratio = (double)pixelCount / tmp->height / tmp->width;
	Debug::print(LOG_SUMMARY, "Para ratio: %f\r\n",ratio);
	return ratio > SEPARATING_PARA_DETECT_THRESHOLD;
}
bool ImageProc::isSky(IplImage* pImage)
{
	if(pImage == NULL)
	{
		Debug::print(LOG_SUMMARY, "Sky detection: Unable to get Image\r\n");
		return false;
	}
	const static int PIC_SIZE_W = 320;
	const static int PIC_SIZE_H = 240;
	const static int SKY_THRESHOLD = 200;		// 臒l�ȏ�ł͂���΋󂾂Ɣ���
	const static int DIV_NUM_HSV = 120;			// HSV�̃T���v�����O�̊Ԋu
	const static double COVER_RATE = 0.6;		// �摜����߂Ă����̊��������̒l�ȏゾ�Ƌ󂾂Ɣ���
	const static int SKY_DETECT_COUNT = 3;

	//Temporary Images
	IplImage* pHsvImage = cvCreateImage(cvSize(PIC_SIZE_W,PIC_SIZE_H),IPL_DEPTH_8U, 3);//HSV(8bits*3channels)	

	//Temporary Matrixes (for mixChannels)
	cv::Mat hsv_mat = cv::cvarrToMat(pHsvImage);
	
	//BGR->HSV
	cvCvtColor(pImage,pHsvImage,CV_BGR2HSV);

	int newValue_h = 0, newValue_s = 0, newValue_v = 0, newValue_sum = 0, sky = 0;
	int detect_count = 0;

	for(int i = 0;i < PIC_SIZE_H - 1;++i)
	{
		for(int j = 0; j < DIV_NUM_HSV;++j){
			int div_width = PIC_SIZE_W / DIV_NUM_HSV;
			newValue_h += (unsigned char)pHsvImage->imageData[pHsvImage->widthStep * i + j * div_width * 3];        // H
			newValue_s += (unsigned char)pHsvImage->imageData[pHsvImage->widthStep * i + j * div_width* 3 + 1];    // S
			newValue_v += (unsigned char)pHsvImage->imageData[pHsvImage->widthStep * i + j * div_width* 3 + 2];    // V
		}

		newValue_h /= DIV_NUM_HSV;
		newValue_s /= DIV_NUM_HSV;
		newValue_v /= DIV_NUM_HSV;
	
		if(newValue_h < 80 || newValue_h > 320){
			newValue_h = 0;
		}

		newValue_sum = newValue_h + newValue_v;

		if(newValue_sum > SKY_THRESHOLD){
			detect_count++;
			if(detect_count > SKY_DETECT_COUNT){
				sky = i;
			}
		}
		else{
			detect_count = 0;
		}

		newValue_h = 0;
		newValue_s = 0;
		newValue_v = 0;
		newValue_sum = 0;
	}
	
	bool isSky = sky > PIC_SIZE_H * COVER_RATE;
	Debug::print(LOG_SUMMARY, "%d / %f (%s)\r\n", sky, PIC_SIZE_H * COVER_RATE,isSky ? "found sky" : "sky not found");
	

	//Release resources
	cvReleaseImage(&pHsvImage);

	return isSky;
}
bool ImageProc::isWadachiExist(IplImage* pImage)
{
	if(pImage == NULL)
	{
		Debug::print(LOG_SUMMARY, "Wadachi predicting: Unable to get Image\r\n");
		return true;
	}
	const static int DIV_NUM = 20;
	const static int PIC_SIZE_W = 320;
	const static int PIC_SIZE_H = 240;
	const static int DELETE_H_THRESHOLD = 80;
	const static double RATE = 2;
	const static double PIC_CUT_RATE = 0.5;
	const static int DIV_HOR_NUM = 5;

	IplImage *src_img, *dst_img1, *tmp_img;
	double risk[DIV_NUM], risk_rate[DIV_NUM];
	CvSize pic_size = cvSize(PIC_SIZE_W, PIC_SIZE_H);

	src_img = cvCreateImage(pic_size, IPL_DEPTH_8U, 1);
	cvCvtColor(pImage, src_img, CV_BGR2GRAY);
		
	//cvRectangle(src_img, cvPoint(0, 0),cvPoint(PIC_SIZE_W, PIC_SIZE_H * PIC_CUT_RATE) ,cvScalar(0), CV_FILLED, CV_AA);
	CvPoint pt[(DIV_HOR_NUM+1)*2+1];
	cutSky(pImage,src_img,pt);

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

	// ����
	for(int i=DIV_NUM - 1; i>=0; --i){
		risk_rate[i] = risk[i] / risk_sum;
	}

	//Draw graph
	for(int i= DIV_NUM-1; i>=0; --i){
		if(i>0){
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
}
int ImageProc::wadachiExiting(IplImage* pImage)
{
	/*
	IplImage* src_img = pImage;
	const static int DIV_NUM = 5;
	const static int MEDIAN = 5;
	IplImage *gray_img, *dst_img1, *tmp_img;
	double risk[DIV_NUM];

	if(src_img == NULL)
	{
		Debug::print(LOG_SUMMARY, "Escaping: Unable to get Image for Camera Escaping!\r\n");
		return INT_MAX;
	}
	CvSize size = cvSize(src_img->width,src_img->height);

	gray_img = cvCreateImage(size, IPL_DEPTH_8U, 1);
	cvCvtColor(src_img, gray_img, CV_BGR2GRAY);
	cvRectangle(gray_img, cvPoint(0, 0),cvPoint(src_img->width, src_img->height * 2 / 5),cvScalar(0), CV_FILLED, CV_AA);

	// Median�t�B���^
	cvSmooth (gray_img, gray_img, CV_MEDIAN, MEDIAN, 0, 0, 0);
		
	tmp_img = cvCreateImage(size, IPL_DEPTH_16S, 1);
	dst_img1 = cvCreateImage(size, IPL_DEPTH_8U, 1);
		
	// Sobel�t�B���^X����
	cvSobel(gray_img, tmp_img, 1, 0, 3);
	cvConvertScaleAbs (tmp_img, dst_img1);
	cvThreshold (dst_img1, dst_img1, 50, 255, CV_THRESH_BINARY);

	//Sum
	int width = src_img->width / DIV_NUM;
	double risksum = 0;
    
	for(int i = 0;i < DIV_NUM;++i)
	{
		cvSetImageROI(dst_img1, cvRect(width * i,0,width,src_img->height));//Set image part
		risksum += risk[i] = sum(cv::cvarrToMat(dst_img1))[0];
		cvResetImageROI(dst_img1);//Reset image part (normal)
	}

	//Draw graph
	//for(int i = 0;i < DIV_NUM;++i){
	//	cvRectangle(dst_img1, cvPoint(width * i,src_img->height - risk[i] / risksum * src_img->height),cvPoint(width * (i + 1),src_img->height),cvScalar(255), 2, CV_AA);
	//}

    
	int min_id = 0;
    int shikiiMin = 70000;
    int shikiiMax = 130000;
    int shikiiMinCount = 0;
    int shikiiMaxCount = 0;
    
    for(int i=0; i<DIV_NUM; ++i){
        if(risk[i] < shikiiMin)
            shikiiMinCount++;
        if(risk[i] > shikiiMax)
            shikiiMaxCount++;
    }
    
    if(shikiiMinCount >= 3){
        min_id = 5;
    }else if(shikiiMaxCount >= 3){
		Debug::print(LOG_SUMMARY, "kabe\n");
        min_id = (risk[0] > risk[DIV_NUM - 1]) ? DIV_NUM - 1 : 0;
    }else{
		int i;
        for(i=1; i<DIV_NUM; ++i){
            if(risk[min_id] > risk[i]){
                min_id = i;
            }
        }
    }
    
    for(i=0; i<DIV_NUM; i++){
        Debug::print(LOG_SUMMARY, " area %d : %f\n" ,i,risk[i]);
    }
    
    Debug::print(LOG_SUMMARY, " min id : %d\n",min_id);
    
	cvReleaseImage (&dst_img1);
	cvReleaseImage (&tmp_img);

	switch(min_id){
		case 0:
			Debug::print(LOG_SUMMARY, "Wadachi kaihi:Turn Left\r\n");
			return -1;
		case DIV_NUM - 1:
			Debug::print(LOG_SUMMARY, "Wadachi kaihi:Turn Right\r\n");
			return 1;
		default:
            Debug::print(LOG_SUMMARY, "Wadachi kaihi:Go Straight\r\n");
			return 0;
	}
	*/


	// �P�������
	// �^�񒆂��J���Ă���ꍇ�ȊO�͉�]
	
	const static int DIV_HOR_NUM = 5;
	const static int MEDIAN = 5;
	const static int DELETE_H_THRESHOLD = 50;
	const static int THRESHOLD_COUNT = 3;               // �m�C�Y���̃u���b�N���̉���
	const static double THRESHOLD_MIN = 70000;          // �m�C�Y���̉���

	if(pImage == NULL)
	{
		Debug::print(LOG_SUMMARY, "Escaping: Unable to get Image for Camera Escaping!\r\n");
		return INT_MAX;
	}

	CvSize size = cvSize(pImage->width,pImage->height);

	IplImage *pCaptureFrame = cvCreateImage(size, IPL_DEPTH_8U, 3);
	cvResize(pImage, pCaptureFrame, CV_INTER_LINEAR);

	CvPoint pt[(DIV_HOR_NUM+1)*2+1];
	cutSky(pCaptureFrame,pCaptureFrame,pt);
	
	// binarization
	cvThreshold(pCaptureFrame, pCaptureFrame, DELETE_H_THRESHOLD, 255, CV_THRESH_BINARY);

	//Sum
	double risk[DIV_HOR_NUM], new_risk[DIV_HOR_NUM];
	double risk_sum = 0;
	int div_width  = size.width  / DIV_HOR_NUM;
	for(int i=0; i<DIV_HOR_NUM; ++i){
		// set image part
		cvSetImageROI(pCaptureFrame, cvRect(div_width * i, 0, div_width, size.height));
		risk_sum += risk[i] = sum(cv::cvarrToMat(pCaptureFrame))[0];
		// reset image part (normal)
		cvResetImageROI(pCaptureFrame);
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
	
	/*for(int i=0; i<DIV_HOR_NUM; ++i){
		Debug::print(LOG_SUMMARY, "[%d] risk %.0f : height %.5f -> risk %.0f\r\n", i, risk[i], heights[i], new_risk[i]);
	}*/
	
	int count = 0;
	for(int i=0; i<DIV_HOR_NUM; ++i){
		if(risk[i] < THRESHOLD_MIN){
			count++;
		}
	}
    
	cvReleaseImage (&pCaptureFrame);

	// ��������
	if(count >= THRESHOLD_COUNT){
		Debug::print(LOG_SUMMARY, "Go straight\r\n");
		return 0;
	}else{
		int ave_left = 0, ave_right = 0;
		for(int i=0; i<DIV_HOR_NUM; ++i){
			if(i < DIV_HOR_NUM/2){
				ave_left += risk[i];
			}
			if(i > DIV_HOR_NUM/2){
				ave_right += risk[i];
			}
		}
		ave_left /= 2; ave_right /= 2;
		if(ave_left < ave_right){
			Debug::print(LOG_SUMMARY, "Turn left\r\n");
			return -1;
		}else{
			Debug::print(LOG_SUMMARY, "Turn right\r\n");
			return 1;
		}
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
	const static int MEDIAN = 5;						// Median�t�B���^�̂ڂ�����i��j
	const static int DELETE_H_THRESHOLD_LOW = 150;		// ���H�i�F���j�͈͉̔���
	const static int DELETE_H_THRESHOLD_HIGH = 270;		// ���H�i�F���j�͈̔͏��
	const static int DELETE_V_THRESHOLD_LOW = 100;      // ���V�i�F���j�͈͉̔���
	const static int DELETE_V_THRESHOLD_HIGH = 200;     // ���V�i�F���j�͈̔͏��
	CvSize capSize = {320,240};
	
	int div_width  = capSize.width  / DIV_HOR_NUM;
	int div_height = capSize.height / DIV_VER_NUM;

	// Median Filter
	IplImage* pSrc2 = cvCreateImage(capSize,IPL_DEPTH_8U, 3);
	cvSmooth (pSrc, pSrc2, CV_MEDIAN, MEDIAN, 0, 0, 0);
	
	//BGR->HSV
	IplImage* pHsvImage = cvCreateImage(capSize,IPL_DEPTH_8U, 3); //HSV(8bits*3channels)
	cvCvtColor(pSrc2, pHsvImage,CV_BGR2HSV);
	
	bool flag;                       // ��t���O
	pt[0] = cvPoint(0,0);            //����[�̍��W���i�[

	for(int i = 0; i <= DIV_HOR_NUM; ++i)
	{
		int find_count = 0;	// ��s�N�Z���̐��̃J�E���g�p
		for(int j = DIV_VER_NUM-1; j >= 0; --j){
			int x = i * div_width;  // �擾�ʒu��x���W
			int y = j * div_height; // �擾�ʒu��y���W
			if(x == 0) x = 1;       // �摜���[�𐳏�ɏ������邽��
			
			// H�l��V�l�擾
			int value_h = (unsigned char)pHsvImage->imageData[pHsvImage->widthStep * y + (x - 1) * 3    ] * 2;   // H
			int value_v = (unsigned char)pHsvImage->imageData[pHsvImage->widthStep * y + (x - 1) * 3 + 2];     // V
			
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
		
		// �󂪔��肳��Ȃ��������͏�[�̍��W���i�[
		if(find_count == 0){
			pt[2*i+1] = cvPoint(i*div_width, 0);
			pt[2*i+2] = cvPoint((i+1)*div_width, 0);
			//printf("pt[%d]=(%2d, %2d)\n", 2*i, pt[2*i].x, pt[2*i].y);
			//printf("pt[%d]=(%2d, %2d)\n", 2*i+1, pt[2*i+1].x, pt[2*i+1].y);
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
	cvReleaseImage(&pHsvImage);
	cvReleaseImage(&pSrc2);
}
ImageProc::ImageProc()
{
	setName("image");
	setPriority(UINT_MAX,UINT_MAX);
}
ImageProc::~ImageProc()
{
}
