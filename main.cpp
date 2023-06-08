#include <opencv2/opencv.hpp>

#include <opencv2/imgproc.hpp> // Gaussian blur
#include <opencv2/highgui.hpp> // Trackbars
//#include <opencv2/photo.hpp> // Denoising

#include <iostream> // cout
#include <fstream> // output in file

#include "tools.h" // my big functions toLines()

using namespace cv;
using namespace std;

/** --- CONTROLS:
  ESC     - exit
  SPACE   - stop video on current frame (pause)
  S       - save image as JPG
  V       - save image as SVG (you can open it in browser)
  F       - save contours to file (one line - one contour: <x1> <y1> <x2> <y2> <x3> <y3> ...)
**/

#define IMAGE_FILE_NAME       "images/RK6.jpg"

#define SETTINGS_WINDOW_NAME  "Tools"
#define RESULT_IMAGE_FILENAME "results/RESULT.jpg"
#define RESULT_SVG_FILENAME   "results/RESULT.svg"
#define RESULT_TXT_FILENAME   "results/RESULT.txt"

#define DEFAULT_BLUR_AMOUNT     1
#define DEFAULT_MAX_THRESHOLD   50
#define DEFAULT_MIN_THRESHOLD   5
#define DEFAULT_MIN_LENGTH      5
#define DEFAULT_MIN_DISTANCE    8
#define DEFAULT_MIN_ANGLE       8

#define MAX_SCALE_BLUR_AMOUNT   10
#define MAX_SCALE_THRESHOLD     200
#define MAX_SCALE_MIN_LENGTH    100
#define MAX_SCALE_MIN_DISTANCE  100
#define MAX_SCALE_MIN_ANGLE     90


bool toggle(bool* var) {
    return *var = *var ? false : true;
}

Scalar getRandomColor() {
    static RNG rng(12345); // random generator
    return Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
}

// ---------- Trackbars
int blurAmount = DEFAULT_BLUR_AMOUNT;
void onTrackbar_blurAmount ( int, void* ) {
    blurAmount = blurAmount * 2 + 1; // blur amount must be odd
}

const int cannyKernelSize = 3;
int cannyMaxThreshold = DEFAULT_MAX_THRESHOLD;
int cannyMinThreshold = DEFAULT_MIN_THRESHOLD;
void onTrackbar_cannyTreshold ( int, void* ) {
    if (cannyMaxThreshold < cannyMinThreshold) {
        cannyMaxThreshold = cannyMinThreshold;
        setTrackbarPos("Max threshold", SETTINGS_WINDOW_NAME, cannyMaxThreshold);
    }
}

int contourMinLength = DEFAULT_MIN_LENGTH;
void onTrackbar_contourLength( int, void* ) {
}

int minDistance = DEFAULT_MIN_DISTANCE;
void onTrackbar_minDistance( int, void* ) {
}

int minAngle = DEFAULT_MIN_ANGLE;
void onTrackbar_minAngle( int, void* ) {
}

// ---------- Main
int main() {
    Mat coloredImage, coloredImageCopy, grayImage, grayImageCopy, edgedImage, bluredImage, bluredEdgedImage, bluredEdgedImageCopy, bluredGrayImage, bluredGrayImageCopy, coloredBluredEdgedImage;
    Mat contouredImage, coloredContouredImage, contouredFilteredImage;
    Mat denoisedImage, denoisedGrayImage;
    VideoCapture cap(0);
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;

    bool stop = false;
    bool paused = false;
    int key;

    // --- Create trackbars' window
    namedWindow(SETTINGS_WINDOW_NAME);
    resizeWindow(SETTINGS_WINDOW_NAME, 500, 400);
    createTrackbar("Blur", SETTINGS_WINDOW_NAME, &blurAmount, MAX_SCALE_BLUR_AMOUNT, onTrackbar_blurAmount);
    createTrackbar("Min threshold", SETTINGS_WINDOW_NAME, &cannyMinThreshold, MAX_SCALE_THRESHOLD, onTrackbar_cannyTreshold);
    createTrackbar("Max threshold", SETTINGS_WINDOW_NAME, &cannyMaxThreshold, MAX_SCALE_THRESHOLD, onTrackbar_cannyTreshold);
    createTrackbar("Min length", SETTINGS_WINDOW_NAME, &contourMinLength, MAX_SCALE_MIN_LENGTH, onTrackbar_contourLength);
    createTrackbar("Min distance", SETTINGS_WINDOW_NAME, &minDistance, MAX_SCALE_MIN_DISTANCE, onTrackbar_minDistance);
    createTrackbar("Min degree", SETTINGS_WINDOW_NAME, &minAngle, MAX_SCALE_MIN_ANGLE, onTrackbar_minAngle);
    onTrackbar_blurAmount(0, nullptr);

    while (!stop) {
        // --- Get image from camera
        if (!paused) {
            #ifdef IMAGE_FILE_NAME
                coloredImage = imread(IMAGE_FILE_NAME);
            #else
                cap >> coloredImage;
                flip(coloredImage, coloredImage, 1); // mirror vertical
            #endif
        }

        // --- Image change logic
        imshow("Colored", coloredImage);

        cvtColor(coloredImage, grayImage, COLOR_BGR2GRAY);

        // - Gray test
        /*
        cvtRGBtoGray(coloredImageCopy);
        imshow("Gray", grayImage);
        imshow("My gray", coloredImageCopy);
        */

        blur(grayImage, bluredGrayImage, Size(blurAmount, blurAmount));
        //imshow("Blured gray", bluredGrayImage);

        // - Sobel test
        /*
        Mat grad_x, grad_y, grad;
        Mat abs_grad_x, abs_grad_y;
        Sobel(grayImage, grad_x, CV_16S, 1, 0,  1, 1, 0, BORDER_DEFAULT);
        Sobel(grayImage, grad_y, CV_16S, 0, 1,  1, 1, 0, BORDER_DEFAULT);
        // converting back to CV_8U
        convertScaleAbs(grad_x, abs_grad_x);
        convertScaleAbs(grad_y, abs_grad_y);
        addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, grad);
        imshow("Sobel OpenCV", grad);

        sobel(grayImageCopy);
        imshow("My Sobel", grayImageCopy);
        */

        Canny(bluredGrayImage, bluredEdgedImage, cannyMinThreshold, cannyMaxThreshold, cannyKernelSize);
        // - Canny test
        /*
        imshow("OpenCV canny", bluredEdgedImage);
        canny(bluredGrayImageCopy);
        imshow("My Canny", bluredGrayImageCopy);
        */
        coloredBluredEdgedImage = Scalar::all(0);
        coloredImage.copyTo(coloredBluredEdgedImage, bluredEdgedImage);
        imshow("Edged", coloredBluredEdgedImage);


        findContours(bluredEdgedImage, contours, hierarchy, RETR_TREE, CHAIN_APPROX_TC89_KCOS);
        // contours.erase(contours.end() - 1, contours.end());
        contouredFilteredImage = Mat::zeros(bluredEdgedImage.size(), CV_8UC3);
        contouredImage = Mat::zeros(bluredEdgedImage.size(), CV_8UC3);
        for (size_t i = 0; i < contours.size(); i++) {
            drawContours(contouredImage, contours, (int)i, getRandomColor(), 2, LINE_8, hierarchy, 0);
        }
        drawContours(contouredFilteredImage, contours, -1, Scalar(255, 255, 255), 1, LINE_8, hierarchy, 0);
        imshow("Contoured", contouredImage);

        for (size_t i = 0; i < contours.size(); i++) {
            // Create lines from thin contour
            toLines(contours, i, minDistance);

            // Creae lines from thin contour
            degreeFilter(contours[i], minAngle);

            // Filter by length
            float contourLength = 0;
            for (size_t j = 1; j < contours[i].size(); j++) {
                contourLength += norm(contours[i][j] - contours[i][j-1]);
            }
            if (contourLength < contourMinLength) {
                contours.erase(contours.begin() + i);
                i--;
                continue;
            }

            // Draw polyline. It's not looped contour
            Scalar contourColor = getRandomColor();
            circle(contouredFilteredImage, contours[i][0], 0, contourColor, 5);
            for (size_t j = 1; j < contours[i].size(); j++) {
                line(contouredFilteredImage, contours[i][j], contours[i][j-1], contourColor, 2);
                circle(contouredFilteredImage, contours[i][j], 0, contourColor, 5);
            }
        }
        //return -10;
        //drawContours(contouredImage, contours, -1, Scalar(255, 255, 255), 2, LINE_8, hierarchy, 0);
        //coloredContouredImage = Scalar::all(0);
        //coloredImage.copyTo(coloredContouredImage, contouredImage);
        imshow("Contoured filtered", contouredFilteredImage);

        /* // Alternative - Denoise image
        cvtColor(coloredImage, grayImage, COLOR_BGR2GRAY);
        imshow("Gray", grayImage);

        fastNlMeansDenoising(grayImage, denoisedGrayImage, blurAmount,  7, 21);
        imshow("Gray denoised", denoisedGrayImage);

        Canny(denoisedGrayImage, bluredEdgedImage, cannyMinThreshold, cannyMaxThreshold, cannyKernelSize);
        coloredBluredEdgedImage = Scalar::all(0);
        coloredImage.copyTo(coloredBluredEdgedImage, bluredEdgedImage);
        imshow("Deniosed Edged", coloredBluredEdgedImage);
        */

        // --- Control keys
        key = waitKey(10); // 10 - delay in millisecinds
        cout << "Pressed key:" << key << endl;
        switch(key) {
            case 27: { // escape
                stop = true;
                break;
            }
            case 32: { // space
            toggle(&paused);
            break;
            }
            case 251: // ы (russian)
            case 115: { // s
                // save as JPG
                imwrite(RESULT_IMAGE_FILENAME, coloredBluredEdgedImage);
                break;
            }
            case 236: // м (russian)
            case 118: { // v
                // save as SVG
                ofstream svg(RESULT_SVG_FILENAME);
                svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" stroke-width=\"1\" stroke=\"black\" fill=\"#FF000044\">" << endl;
                for (auto contour: contours) {
                    svg << "<polyline points=\""; // create new polyline
                    for (auto point: contour) {
                        svg << point.x << "," << point.y << " " << endl;
                    }
                    svg << "\"/>" << endl; // end polyline
                }
                svg << "</svg>" << endl;
                svg.close();
                break;
            }
            case 224: // а (rissuian)
            case 102: { // f
                // save as txt data-file
                // <x11> <y11> <x12> <y12> <x13> <y13> ...
                // <x21> <y21> <x22> <y22> <x23> <y23> ...
                // ...
                ofstream txt(RESULT_TXT_FILENAME);
                for (auto contour: contours) {
                    for (auto point: contour) {
                        txt << point.x << " " << point.y << " ";
                    }
                    txt << endl;
                }
                txt.close();
                break;
            }
                break;
            case 234: // к (rissuian)
            case 114: { // r
                break;
            }
            case 229: // е (rissuian)
            case 116: { // t
                break;
            }
            // Only for tests
            case 48: // 0
            case 49: // 1
            case 50: // 2
            case 51: // 3
            case 52: // 4
            case 53: // 5
            case 54: // 6
            case 55: // 7
            case 57: // 8
            case 58: { // 9
                //contoursApprox = key - 48;
            }
            default:
                break;
        }
    }

    destroyAllWindows();
    return EXIT_SUCCESS;
}
