#include <iostream>
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <stdio.h>
#include <windows.h>

using namespace cv;
using namespace std;

#define red CV_RGB(255,0,0)
#define green CV_RGB(0,255,0)
#define blue CV_RGB(0,0,255)
#define white CV_RGB(255,255,255)
#define black CV_RGB(0,0,0)

char tipka;
int hueMin = 40, hueMax = 90, saturationMin = 50, saturationMax = 200, valueMin = 0, valueMax = 255;
Vec3b pixel_value;

void ClearScreen (Mat imgDrawing) {
	imgDrawing = Scalar(white);
}

void on_trackbar(int, void*) {

}

void createTrackbars() {
	namedWindow("Control Panel", 1);
	createTrackbar("H_MIN", "Control Panel", &hueMin, 255, on_trackbar);
	createTrackbar("H_MAX", "Control Panel", &hueMax, 255, on_trackbar);
	createTrackbar("S_MIN", "Control Panel", &saturationMin, 255, on_trackbar);
	createTrackbar("S_MAX", "Control Panel", &saturationMax, 255, on_trackbar);
	createTrackbar("V_MIN", "Control Panel", &valueMin, 255, on_trackbar);
	createTrackbar("V_MAX", "Control Panel", &valueMax, 255, on_trackbar);
}

int main(int, char**) {
	Mat imgScribble, imgHSV, imgThreshold, imgColorPanel= imread("cvPaint.png");
	CvFont font, fontbig;
	VideoCapture cap(1);

	double area_limit = 700;

	int lineThickness = 2, confirm_close = 10, confirm_clear = 20, posX = 0, posY = 0;
	char buffer [50];
	Scalar lineColor = blue;

	if (!cap.isOpened()) {
		cout << "Could not initialize capturing...\n";
		return -1;
	}

	createTrackbars();

	Mat imgDrawing(Size(cap.get(CV_CAP_PROP_FRAME_WIDTH), cap.get(CV_CAP_PROP_FRAME_HEIGHT)), CV_8UC3);

	imgDrawing = Scalar(white);

	for (;;) {
		Mat frame;
		cap >> frame;

		bool aSucces = cap.read(frame);

		if (!aSucces) {
			cout << "Cannot read a frame from video stream" << endl;
			break;
		}

		flip(frame, frame, +1);

		cvtColor(frame, imgHSV, COLOR_BGR2HSV);

		inRange(imgHSV, Scalar(hueMin, saturationMin, valueMin), Scalar(hueMax, saturationMax, valueMax), imgThreshold);
		//inRange(imgHSV, Scalar(87, 98, 0), Scalar(141, 200, 255), imgThreshold);

		erode(imgThreshold, imgThreshold, getStructuringElement(MORPH_ELLIPSE, Size(5,5)));
		dilate(imgThreshold, imgThreshold, getStructuringElement(MORPH_ELLIPSE, Size(5,5)));

		dilate(imgThreshold, imgThreshold, getStructuringElement(MORPH_ELLIPSE, Size(5,5)));
		erode(imgThreshold, imgThreshold, getStructuringElement(MORPH_ELLIPSE, Size(5,5)));

		Moments oMoments = moments(imgThreshold);

		double dM01 = oMoments.m01;
		double dM10 = oMoments.m10;
		double dArea = oMoments.m00;

		int lastX = posX, lastY = posY;
		posX = 0; posY = 0;

		if(dM10 / dArea >= 0 && dM10 / dArea < 1280 && dM01 / dArea >= 0 && dM01 / dArea < 1280 && dArea > area_limit) {
			posX = dM10 / dArea;
			posY = dM01 / dArea;
		}

		Point point = Point(150, 30);
		if(posX < 90 && posY > 400) {
			lineColor = white;
			putText(frame, "Eraser selected", point, CV_FONT_HERSHEY_COMPLEX, 1, white, 2, CV_AA);
			sprintf(buffer, "Clearing the screen in %d", confirm_clear);
			putText(frame, buffer, Point(150,70), CV_FONT_HERSHEY_COMPLEX, 1, red, 2, CV_AA);
			confirm_clear--;
			if(confirm_clear < 0) {
				confirm_clear = 20;
				ClearScreen(imgScribble, imgDrawing);
				putText(frame, "Cleared the screen", Point(150, 110), CV_FONT_HERSHEY_COMPLEX, 1, white, 2, CV_AA);
			}
		} else if(posX > 540 && posY > 360) {
			lineColor = blue;
			putText(frame, "Blue color selected", point, CV_FONT_HERSHEY_COMPLEX, 1, blue, 2, CV_AA);
		} else if(posX > 540 && posY > 200 && posY < 280) {
			lineColor = green;
			putText(frame, "Green color selected", point, CV_FONT_HERSHEY_COMPLEX, 1, green, 2, CV_AA);
		} else if(posX > 540 && posY < 120) {
			lineColor = red;
			putText(frame, "Red color selected", point, CV_FONT_HERSHEY_COMPLEX, 1, red, 2, CV_AA);
		} else if(posX > 0 && posX < 90 && posY > 0 && posY < 120) {
			sprintf(buffer, "EXITING in %d", confirm_close);
			putText(frame, buffer, point, CV_FONT_HERSHEY_COMPLEX, 1, red, 2, CV_AA);
			confirm_close--;
			if(confirm_close < 0) break;
		} else if(posX < 90 && posY > 130 && posY < 390) {
			lineThickness = 6 - (posY/60-1);
		}
		sprintf(buffer, "%d", lineThickness);
		putText(frame, buffer, Point(30, 265), CV_FONT_HERSHEY_COMPLEX, 3, lineColor, 3, CV_AA);

		double diff_X = lastX - posX;
		double diff_Y = lastY - posY;
		double magnitude = sqrt(pow(diff_X, 2) + pow(diff_Y, 2));

		if(magnitude > 0 && magnitude < 100 && posX > 120 && posX < 530) line(imgDrawing, Point(posX, posY), Point(lastX, lastY), lineColor, lineThickness, CV_AA);

		bitwise_and(frame, imgDrawing, frame);
		bitwise_and(imgColorPanel, frame, frame);

		pixel_value = frame.at<Vec3b>(Point(200, 400));//if you know the position of the pixel
		cout << "pixel_value: " << pixel_value << endl;

		imshow("Thresholding", imgThreshold);
		imshow("Drawing", imgDrawing);
		imshow("Video", frame);

		tipka = waitKey(30);
		if (tipka == 'q') {
			cout << "Terminating. . ." << endl;
			Sleep(10);
			break;
		}
	}
	return 0;
}
