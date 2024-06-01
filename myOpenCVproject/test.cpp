#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\core\core.hpp>
#include <opencv2\imgproc\imgproc.hpp>
#include <opencv2\features2d\features2d.hpp>
#include <opencv2\opencv.hpp> 
#include <opencv2/video/background_segm.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <iostream>
#include <stdio.h>

//cursor 
#include <chrono>
#include <thread>
#include <Windows.h>
#include <string>

using namespace cv;
using namespace std;

void track(int, void*);
void leftClick();
void releaseLeftClick();
void rightClick();
void releaseRightClick();
void scrollUp();
void scrollDown();

Mat originalMask;
Mat fgMaskMOG2;
Mat grayImage, eyelash, or2, edges, mirrored;
int thresh = 250, maxVal = 255; //150, 240
int type = 10, value = 8; //4, 8

Point oldPos;
Point smoothedPos(0, 0);


Point lerp(const Point& start, const Point& end, float t);


void main()
{

    Ptr<BackgroundSubtractorMOG2> pMOG2 = createBackgroundSubtractorMOG2(/*500, 34, false*/); // increase history 


    Rect myRoi(288, 12, 288, 288);
    Mat element = getStructuringElement(MORPH_RECT, Size(3, 3), Point(1, 1));
    Mat frame;
    Mat resizeF;
    VideoCapture cap;
    cap.open(2);




    while (1)
    {
        int n = 100;
        Mat aynali2;
        cap >> originalMask;
        cv::flip(originalMask, mirrored, 1);

        cv::rectangle(mirrored, myRoi, cv::Scalar(0, 0, 255));
        eyelash = mirrored(myRoi);
        cvtColor(eyelash, grayImage, cv::COLOR_BGR2RGB);

        GaussianBlur(grayImage, grayImage, Size(35, 35), 0); //35,35

        pMOG2->apply(eyelash, fgMaskMOG2);

        cv::rectangle(fgMaskMOG2, myRoi, cv::Scalar(0, 0, 255));




        track(0, 0);
        //creating windows
        imshow("ORIGINAL Image", mirrored);
        imshow("Background Removed", fgMaskMOG2);
        imshow("Gray", grayImage);

        //moving windows
        moveWindow("ORIGINAL Image", 110 + 500, 50);
        moveWindow("Background Removed", 750 + 500, 50);
        moveWindow("Gray", 750 + 500, 350);


        char key = waitKey(24);
        if (key == 27) break;
    }
}

void track(int, void*)
{
    int count = 0;
    char a[40];
    char position[40];

    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    GaussianBlur(fgMaskMOG2, fgMaskMOG2, Size(27, 27), 3.5, 3.5);
    threshold(fgMaskMOG2, fgMaskMOG2, thresh, maxVal, type);

    Canny(fgMaskMOG2, edges, value, value * 2, 3); //edges -> eyelash
    findContours(fgMaskMOG2, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));
    Mat drawing = Mat::zeros(edges.size(), CV_8UC3); //edges -> eyelash

    if (!contours.empty()) {
        size_t indexOfBiggestContour = -1;
        size_t sizeOfBiggestContour = 0;
        for (size_t i = 0; i < contours.size(); i++) {
            if (contours[i].size() > sizeOfBiggestContour) {
                sizeOfBiggestContour = contours[i].size();
                indexOfBiggestContour = i;
            }
        }

        vector<vector<int>> hull(contours.size());
        vector<vector<Point>> hullPoint(contours.size());
        vector<vector<Vec4i>> defects(contours.size());
        vector<vector<Point>> defectPoint(contours.size());
        vector<vector<Point>> contours_poly(contours.size());

        Point2f rect_point[4];
        vector<RotatedRect> minRect(contours.size());
        vector<Rect> boundRect(contours.size());

        for (size_t i = 0; i < contours.size(); i++)
        {
            if (contourArea(contours[i]) > 5000)
            {
                convexHull(contours[i], hull[i], true);
                convexityDefects(contours[i], hull[i], defects[i]);
                if (indexOfBiggestContour == i)
                {
                    minRect[i] = minAreaRect(contours[i]);
                    for (size_t k = 0; k < hull[i].size(); k++)
                    {
                        int ind = hull[i][k];
                        hullPoint[i].push_back(contours[i][ind]);
                    }
                    count = 0;
                    Point sumOfFingertips(0, 0);
                    vector<Point> myFingertipsCoords;

                    for (size_t k = 0; k < defects[i].size(); k++)
                    {
                        if (defects[i][k][3] > 3500)
                        {
                            int p_start = defects[i][k][0];
                            int p_end = defects[i][k][1];
                            int p_far = defects[i][k][2];
                            defectPoint[i].push_back(contours[i][p_far]);
                            circle(grayImage, contours[i][p_end], 3, Scalar(0, 255, 0), 2);

                            myFingertipsCoords.push_back(contours[i][p_end]); //positions of fingertips
                            sumOfFingertips += contours[i][p_end];
                            count++;
                        }

                    }
                    cout << "\033[1;31m"; //red color
                    cout << "-----------------" << endl;
                    for (const auto& fingertip : myFingertipsCoords)
                    {
                        cout << "Fingertip at: (" << fingertip.x << ", " << fingertip.y << ")" << endl;
                    }
                    cout << count << endl;
                    cout << "-----------------" << endl;

                    int realFingersCount = 0;

                    sumOfFingertips += Point(1, 1);
                    Point midOfEdges = sumOfFingertips / (count + 1);

                    vector<Point> realFingertipsCoords;

                    sumOfFingertips = Point(1, 1);

                    for (const auto& fingertip : myFingertipsCoords) 
                    {
                        if (fingertip.y > midOfEdges.y - (midOfEdges.y * 0.9) && true == true)
                        { 
                            realFingertipsCoords.push_back(fingertip);
                            sumOfFingertips += fingertip;
                            realFingersCount++;
                            cout << "good: " << fingertip.y << endl;
                        }
                    }
                    myFingertipsCoords.clear();
                    cout << "\033[1;32m"; //green color
                    cout << "-----------------" << endl;
                    for (const auto& fingertip : realFingertipsCoords)
                    {
                        cout << "Fingertip at: (" << fingertip.x << ", " << fingertip.y << ")" << endl;
                    }
                    cout << realFingersCount << endl;
                    cout << "-----------------" << endl;
                    cout << "\033[0m"; //normal color

                    if (count == 1)
                        strcpy_s(a, "1");
                    else if (count == 2)
                        strcpy_s(a, "2");
                    else if (count == 3)
                        strcpy_s(a, "3");
                    else if (count == 4)
                        strcpy_s(a, "4");
                    else if (count >= 5)
                        strcpy_s(a, "5");
                    else
                        strcpy_s(a, "Nothing");

                    


                    Point midpoint = Point(1, 1);
                    if (realFingersCount > 0)
                    {
                        midpoint = sumOfFingertips / realFingersCount;
                        snprintf(position, sizeof(position), "(%d, %d)", midpoint.x, midpoint.y);
                    }
                    else
                    {
                        strcpy_s(position, "No fingertips detected");
                    }

                    putText(mirrored, a, Point(75, 450), 0, 3, Scalar(0, 255, 0), 3, 8, false);
                    putText(mirrored, position, Point(125, 450), 0, 1, Scalar(0, 255, 0), 1, 8, false);

                    //-----------------MOUSE ACTIONS--------------------------------
                    //*
                    if (realFingersCount >= 3 && realFingersCount <= 7)
                    {
                        float alpha = 0.05f;
                        smoothedPos = lerp(smoothedPos, midpoint, alpha);

                        if (abs(midpoint.x - oldPos.x) > 1 || abs(midpoint.y - oldPos.y) > 1)
                        {
                            POINT myCurPosition;
                            GetCursorPos(&myCurPosition);

                            int deltaX = smoothedPos.x - oldPos.x;
                            int deltaY = smoothedPos.y - oldPos.y;

                            SetCursorPos(myCurPosition.x + deltaX * 8, myCurPosition.y + deltaY * 8);

                            oldPos = smoothedPos;
                        }
                        if (realFingersCount == 3 || realFingersCount == 4)
                        {
                            leftClick();
                        }
                        else
                        {
                            releaseLeftClick();
                        }
                    }
                    else if (realFingersCount == 2)
                    {
                        if (realFingertipsCoords.size() == 2)
                        {
                            Point fingertip1 = realFingertipsCoords[0];
                            Point fingertip2 = realFingertipsCoords[1];

                            // Calculate the Euclidean distance between the two fingertips
                            double distance = sqrt(pow(fingertip1.x - fingertip2.x, 2) + pow(fingertip1.y - fingertip2.y, 2));

                            if (distance < 50)
                            {
                                releaseLeftClick();
                                scrollDown();
                            }
                            else
                            {
                                releaseLeftClick();
                                rightClick();
                                releaseRightClick();
                            }
                        }
                    }
                    else if (realFingersCount == 1)
                    {
                        releaseLeftClick();
                        scrollUp();
                    }

                    //*/



                    drawContours(drawing, contours, i, Scalar(255, 255, 0), 2, 8, vector<Vec4i>(), 0, Point());
                    drawContours(drawing, hullPoint, i, Scalar(255, 255, 0), 1, 8, vector<Vec4i>(), 0, Point());
                    drawContours(grayImage, hullPoint, i, Scalar(0, 0, 255), 2, 8, vector<Vec4i>(), 0, Point());
                    approxPolyDP(contours[i], contours_poly[i], 3, false);
                    boundRect[i] = boundingRect(contours_poly[i]);
                    rectangle(grayImage, boundRect[i].tl(), boundRect[i].br(), Scalar(255, 0, 0), 2, 8, 0);
                    minRect[i].points(rect_point);
                    for (size_t k = 0; k < 4; k++) {
                        line(grayImage, rect_point[k], rect_point[(k + 1) % 4], Scalar(0, 255, 0), 2, 8);
                    }
                }
            }
        }
    }

    imshow("Result", drawing);
    moveWindow("Result", 750 + 500, 650);
}
Point lerp(const Point& start, const Point& end, float t)
{
    return Point(start.x + t * (end.x - start.x), start.y + t * (end.y - start.y));
}

void leftClick()
{
    INPUT input;
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN; // Press left button
    SendInput(1, &input, sizeof(INPUT));
}

void releaseLeftClick()
{
    INPUT input;
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN; // Release left button
    SendInput(1, &input, sizeof(INPUT));
}

void rightClick()
{
    INPUT input;
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN; // Press right button
    SendInput(1, &input, sizeof(INPUT));
}

void releaseRightClick()
{
    INPUT input;
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_RIGHTUP; // Release right button
    SendInput(1, &input, sizeof(INPUT));
}

void scrollUp()
{
    INPUT input;
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_WHEEL;
    input.mi.mouseData = WHEEL_DELTA;
    SendInput(1, &input, sizeof(INPUT));
}

void scrollDown()
{
    INPUT input;
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_WHEEL;
    input.mi.mouseData = -WHEEL_DELTA;
    SendInput(1, &input, sizeof(INPUT));
}


