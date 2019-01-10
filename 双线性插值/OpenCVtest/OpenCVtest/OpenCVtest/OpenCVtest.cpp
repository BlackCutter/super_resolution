
#include "stdafx.h"
#include "core/core.hpp"
#include "highgui/highgui.hpp"
#include "imgproc/imgproc.hpp"
#include "video/tracking.hpp"
#include<iostream>
#include <highgui.h>
#include <opencv2/opencv.hpp>





//-----------------------------------��ͷ�ļ��������֡�---------------------------------------
//     ����������������������ͷ�ļ�
//----------------------------------------------------------------------------------------------
#include <opencv2/core/core.hpp>
#include<opencv2/highgui/highgui.hpp>
#include"opencv2/imgproc/imgproc.hpp"
#include <iostream>

//-----------------------------------�������ռ��������֡�---------------------------------------
//     ����������������ʹ�õ������ռ�
//-----------------------------------------------------------------------------------------------  
using namespace std;
using namespace cv;


//-----------------------------------��ȫ�ֺ����������֡�--------------------------------------
//     ������ȫ�ֺ�������
//-----------------------------------------------------------------------------------------------
static void ContrastAndBright(int, void *);

//-----------------------------------��ȫ�ֱ����������֡�--------------------------------------
//     ������ȫ�ֱ�������
//-----------------------------------------------------------------------------------------------
int g_threshold1; //����ֵ
int g_threshold2;  //����ֵ
Mat g_srcImage, g_dstImage;//���� ���ͼ��

cv::Size2f a;
const decltype(a.width) minwidth = 30;  //if you dont know the type of width 
#define minheight  30                   
#define maxwidth   100
#define maxheight  100


int apertureSize = 3;//aperture size for the Sobel() operator.
int point_num_min = 100; //least point in a contour

vector<Vec3f> circles;//����Բ����� ��Բ��+�뾶��


vector<vector<Point> > contours1;
vector<RotatedRect> contours01;
//-----------------------------------��main( )������--------------------------------------------
//     ����������̨Ӧ�ó������ں��������ǵĳ�������￪ʼ
//-----------------------------------------------------------------------------------------------
int main()
{


	//�����û��ṩ��ͼ��
	g_srcImage = imread("demo4.jpg");
	//��ֵ�˲�
	medianBlur(g_srcImage, g_srcImage, 3);
	//��˹�˲�
	GaussianBlur(g_srcImage, g_srcImage, cv::Size(7, 7), 1.5, 1.5);
	if (!g_srcImage.data) { printf("Oh��no����ȡg_srcImageͼƬ����~��\n"); return false; }
	g_dstImage = Mat::zeros(g_srcImage.size(), g_srcImage.type());



	//�趨�ԱȶȺ����ȵĳ�ֵ
	g_threshold1 = 50;
	g_threshold2 = 150;

	//��������
	namedWindow("��Ч��ͼ���ڡ�", 1);

	//�����켣��
	createTrackbar("��ֵ1��", "��Ч��ͼ���ڡ�", &g_threshold1, 300, ContrastAndBright);
	createTrackbar("��ֵ2��", "��Ч��ͼ���ڡ�", &g_threshold2, 300, ContrastAndBright);

	//���ûص�����
	ContrastAndBright(g_threshold1, 0);
	ContrastAndBright(g_threshold2, 0);

	//���һЩ������Ϣ
	cout << endl << "\t�š����ˣ�������������۲�ͼ��Ч��~\n\n"
		<< "\t���¡�q����ʱ�������˳�~!\n"
		;

	//���¡�q����ʱ�������˳�
	while (char(waitKey(1)) != 'q') {}
	return 0;
}


//-----------------------------��ContrastAndBright( )������------------------------------------
//     �������ı�ͼ��Canny��ֵ�Ļص�����
//-----------------------------------------------------------------------------------------------
static void ContrastAndBright(int, void *)
{

	//��������
	namedWindow("��ԭʼͼ���ڡ�", 1);
	namedWindow("����Բ��ⴰ�ڡ�", 1);
	namedWindow("��������ⴰ�ڡ�", 1);
	Canny(g_srcImage, g_dstImage, g_threshold1, g_threshold2, 3);//ʹ��Canny����
	Mat houghMatIn, houghMatOut;
	houghMatIn = g_dstImage.clone();
	houghMatOut = g_srcImage.clone();
	HoughCircles(houghMatIn, circles, CV_HOUGH_GRADIENT, 1, 100, g_threshold2, 150, 45, 220);//����Բ��� 
																							 //��Բ
	for (size_t i = 0; i < circles.size(); i++)
	{
		Vec3f c = circles[i];
		circle(houghMatOut, Point(c[0], c[1]), c[2], Scalar(0, 255, 255), 3, CV_AA);
		//cout << "R:" << c[2] << endl;
	}/**/
	//cout << "num:" << circles.size() << endl;

	//������� 
	// ����ͼ������ǵ�ͨ��
	//contours1�洢��⵽������
	//RETR_LIST:��ȡ������������������list�У����������������ȼ���ϵ
	//CHAIN_APPROX_NONE����ȡÿ��������ÿ�����أ����ڵ������������λ�ò����1 
	Mat contoursInput = g_dstImage.clone();
	findContours(g_dstImage, contours1, CV_RETR_LIST, CV_CHAIN_APPROX_NONE, cvPoint(0, 0));

	Mat ellipse_contour;//��Բ����
	Mat allcontours(contoursInput.size(), CV_8U, cv::Scalar(255));//�������ͼ������
															   
	ellipse_contour.create(contoursInput.size(), contoursInput.type());          //
	
	contours01.reserve(contours1.size());//���� ��⵽��������

	for (size_t k = 0; k < contours1.size(); k++)
	{

		int count = contours1[k].size(); // ��⵽�����������ص���
		if (count > point_num_min)   //  the least points to form a contour
			contours01.push_back(fitEllipse(contours1[k]));//����Բ����������Ž� ��Բ����
	}

	//��������
	drawContours(allcontours, contours1, -1, CV_RGB(0, 0, 0), 1, LINE_8, noArray(), contours1.size(), cvPoint(0, 0));
	
	
	Size2f min;
	Size2f max;
	min.width = minwidth;
	min.height = minheight;
	max.width = maxwidth;
	max.height = maxheight;

	for (auto oneContour : contours01)
	{
		auto ellipse_size = oneContour.size;
		if (
			ellipse_size.width < max.width && ellipse_size.height < max.height
			&&    ellipse_size.width > min.width && ellipse_size.height > min.height
			)
		{
			//����������  e=a/c  e<0.1 e2<0.01
			int x1, x2;
			double a, b;
			double e2, e;
			x1 = oneContour.size.height;
			x2 = oneContour.size.width;//���泤�����ֵ a�ǳ��� b�Ƕ���
			if (x1 >= x2) {
				a = x1;
				b = x2;
			}
			else {
				a = x2;
				b = x1;
			}
			e2 = (a*a - b*b) / (a*a);
			e = pow((double)e2, 0.5);
			
			if (e <= 0.3) {
				ellipse(ellipse_contour, oneContour, CV_RGB(255, 255, 255));
				//��ʵ�ĵ�
				circle(ellipse_contour, oneContour.center, 3, Scalar(255, 255, 255), -1); //�������������Ϊ-1���������Ǹ�ʵ�㡣
				ellipse(houghMatOut, oneContour, CV_RGB(255, 0, 0));
				circle(houghMatOut, oneContour.center, 3, Scalar(0, 0, 255), -1);
				std::cout << "������ = " << oneContour.size << " , "
					<< "���ĵ� =" << oneContour.center << std::endl;
				cout << "�����ʣ� " << e << endl;
			}
			else {
				continue;
			}

		}

	}
	//cout << "all number:" << contours01.size();
	vector <RotatedRect>().swap(contours01);  //�����������С����������
	vector <vector<Point>>().swap(contours1);
	//��ʾͼ��
	imshow("����Բ��ⴰ�ڡ�", ellipse_contour);
	imshow("��������ⴰ�ڡ�", allcontours);
	/**/
	imshow("��ԭʼͼ���ڡ�", houghMatOut);
	imshow("��Ч��ͼ���ڡ�", g_dstImage);
}


/*
void cv::drawContours(
cv::InputOutputArray image, // ���ڻ��Ƶ�����ͼ��
cv::InputArrayOfArrays contours, // ���vectors��vector
int contourIdx, // ��Ҫ���Ƶ�������ָ�� (-1 ��ʾ "all")
const cv::Scalar& color, // ��������ɫ
int thickness = 1, // �����ߵĿ��
int lineType = 8, //  �����ߵ�����ģʽ('4'���� �� '8'����)
cv::InputArray hierarchy = noArray(), // ��ѡ (�� findContours�õ�)
int maxLevel = INT_MAX, // �����е�����½�
cv::Point offset = cv::Point() // (��ѡ) ���е��ƫ��
)
*/