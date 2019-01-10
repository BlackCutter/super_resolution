
#include "stdafx.h"
#include "core/core.hpp"
#include "highgui/highgui.hpp"
#include "imgproc/imgproc.hpp"
#include "video/tracking.hpp"
#include<iostream>
#include <highgui.h>
#include <opencv2/opencv.hpp>





//-----------------------------------【头文件包含部分】---------------------------------------
//     描述：包含程序所依赖的头文件
//----------------------------------------------------------------------------------------------
#include <opencv2/core/core.hpp>
#include<opencv2/highgui/highgui.hpp>
#include"opencv2/imgproc/imgproc.hpp"
#include <iostream>

//-----------------------------------【命名空间声明部分】---------------------------------------
//     描述：包含程序所使用的命名空间
//-----------------------------------------------------------------------------------------------  
using namespace std;
using namespace cv;


//-----------------------------------【全局函数声明部分】--------------------------------------
//     描述：全局函数声明
//-----------------------------------------------------------------------------------------------
static void ContrastAndBright(int, void *);

//-----------------------------------【全局变量声明部分】--------------------------------------
//     描述：全局变量声明
//-----------------------------------------------------------------------------------------------
int g_threshold1; //低阈值
int g_threshold2;  //高阈值
Mat g_srcImage, g_dstImage;//输入 输出图像

cv::Size2f a;
const decltype(a.width) minwidth = 30;  //if you dont know the type of width 
#define minheight  30                   
#define maxwidth   100
#define maxheight  100


int apertureSize = 3;//aperture size for the Sobel() operator.
int point_num_min = 100; //least point in a contour

vector<Vec3f> circles;//霍夫圆检测结果 （圆心+半径）


vector<vector<Point> > contours1;
vector<RotatedRect> contours01;
//-----------------------------------【main( )函数】--------------------------------------------
//     描述：控制台应用程序的入口函数，我们的程序从这里开始
//-----------------------------------------------------------------------------------------------
int main()
{


	//读入用户提供的图像
	g_srcImage = imread("demo4.jpg");
	//中值滤波
	medianBlur(g_srcImage, g_srcImage, 3);
	//高斯滤波
	GaussianBlur(g_srcImage, g_srcImage, cv::Size(7, 7), 1.5, 1.5);
	if (!g_srcImage.data) { printf("Oh，no，读取g_srcImage图片错误~！\n"); return false; }
	g_dstImage = Mat::zeros(g_srcImage.size(), g_srcImage.type());



	//设定对比度和亮度的初值
	g_threshold1 = 50;
	g_threshold2 = 150;

	//创建窗口
	namedWindow("【效果图窗口】", 1);

	//创建轨迹条
	createTrackbar("阈值1：", "【效果图窗口】", &g_threshold1, 300, ContrastAndBright);
	createTrackbar("阈值2：", "【效果图窗口】", &g_threshold2, 300, ContrastAndBright);

	//调用回调函数
	ContrastAndBright(g_threshold1, 0);
	ContrastAndBright(g_threshold2, 0);

	//输出一些帮助信息
	cout << endl << "\t嗯。好了，请调整滚动条观察图像效果~\n\n"
		<< "\t按下“q”键时，程序退出~!\n"
		;

	//按下“q”键时，程序退出
	while (char(waitKey(1)) != 'q') {}
	return 0;
}


//-----------------------------【ContrastAndBright( )函数】------------------------------------
//     描述：改变图像Canny阈值的回调函数
//-----------------------------------------------------------------------------------------------
static void ContrastAndBright(int, void *)
{

	//创建窗口
	namedWindow("【原始图窗口】", 1);
	namedWindow("【椭圆检测窗口】", 1);
	namedWindow("【轮廓检测窗口】", 1);
	Canny(g_srcImage, g_dstImage, g_threshold1, g_threshold2, 3);//使用Canny算子
	Mat houghMatIn, houghMatOut;
	houghMatIn = g_dstImage.clone();
	houghMatOut = g_srcImage.clone();
	HoughCircles(houghMatIn, circles, CV_HOUGH_GRADIENT, 1, 100, g_threshold2, 150, 45, 220);//霍夫圆检测 
																							 //画圆
	for (size_t i = 0; i < circles.size(); i++)
	{
		Vec3f c = circles[i];
		circle(houghMatOut, Point(c[0], c[1]), c[2], Scalar(0, 255, 255), 3, CV_AA);
		//cout << "R:" << c[2] << endl;
	}/**/
	//cout << "num:" << circles.size() << endl;

	//轮廓检测 
	// 输入图像必须是单通道
	//contours1存储检测到的轮廓
	//RETR_LIST:提取所有轮廓，并放置在list中，检测的轮廓不建立等级关系
	//CHAIN_APPROX_NONE：获取每个轮廓的每个像素，相邻的两个点的像素位置差不超过1 
	Mat contoursInput = g_dstImage.clone();
	findContours(g_dstImage, contours1, CV_RETR_LIST, CV_CHAIN_APPROX_NONE, cvPoint(0, 0));

	Mat ellipse_contour;//椭圆区域
	Mat allcontours(contoursInput.size(), CV_8U, cv::Scalar(255));//获得所有图像轮廓
															   
	ellipse_contour.create(contoursInput.size(), contoursInput.type());          //
	
	contours01.reserve(contours1.size());//保留 检测到的轮廓数

	for (size_t k = 0; k < contours1.size(); k++)
	{

		int count = contours1[k].size(); // 检测到的轮廓的像素点数
		if (count > point_num_min)   //  the least points to form a contour
			contours01.push_back(fitEllipse(contours1[k]));//用椭圆拟合轮廓并放进 椭圆区域
	}

	//绘制轮廓
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
			//计算离心率  e=a/c  e<0.1 e2<0.01
			int x1, x2;
			double a, b;
			double e2, e;
			x1 = oneContour.size.height;
			x2 = oneContour.size.width;//保存长轴短轴值 a是长轴 b是短轴
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
				//画实心点
				circle(ellipse_contour, oneContour.center, 3, Scalar(255, 255, 255), -1); //第五个参数我设为-1，表明这是个实点。
				ellipse(houghMatOut, oneContour, CV_RGB(255, 0, 0));
				circle(houghMatOut, oneContour.center, 3, Scalar(0, 0, 255), -1);
				std::cout << "长短轴 = " << oneContour.size << " , "
					<< "中心点 =" << oneContour.center << std::endl;
				cout << "离心率： " << e << endl;
			}
			else {
				continue;
			}

		}

	}
	//cout << "all number:" << contours01.size();
	vector <RotatedRect>().swap(contours01);  //清除容器并最小化它的容量
	vector <vector<Point>>().swap(contours1);
	//显示图像
	imshow("【椭圆检测窗口】", ellipse_contour);
	imshow("【轮廓检测窗口】", allcontours);
	/**/
	imshow("【原始图窗口】", houghMatOut);
	imshow("【效果图窗口】", g_dstImage);
}


/*
void cv::drawContours(
cv::InputOutputArray image, // 用于绘制的输入图像
cv::InputArrayOfArrays contours, // 点的vectors的vector
int contourIdx, // 需要绘制的轮廓的指数 (-1 表示 "all")
const cv::Scalar& color, // 轮廓的颜色
int thickness = 1, // 轮廓线的宽度
int lineType = 8, //  轮廓线的邻域模式('4'邻域 或 '8'邻域)
cv::InputArray hierarchy = noArray(), // 可选 (从 findContours得到)
int maxLevel = INT_MAX, // 轮廓中的最大下降
cv::Point offset = cv::Point() // (可选) 所有点的偏移
)
*/