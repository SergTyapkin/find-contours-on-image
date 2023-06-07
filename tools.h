#include <iostream> // cout
#include <opencv2/opencv.hpp> // Point

using namespace std;
using namespace cv;

bool isContourAdded = false;
size_t baseContour = 0;

void toLines(vector<vector<Point>>& contours, size_t now, int minDistance) {
    int newContourBegin = -1;
    bool isNextContourAdded = false;

    if (isContourAdded) {
        for (int last = 1; last < contours[now].size(); last++) {
            for (int check = contours[baseContour].size() - 1; check >= 0; check--) {
                if (norm(contours[now][last] - contours[baseContour][check]) < minDistance) { // need to delete point
                    contours[now][last] = contours[baseContour][check]; // move last in found position
                    check = -1; // break for
                }
            }
        }
    } else {
        contours[now].push_back(contours[now][0]); // loop contours[now]
    }

    newContourBegin = -1;
    for (int last = 1; last < contours[now].size(); last++) {
        bool isDeleted = false;
        for (int check = last-1; check >= 0; check--) {
            if (norm(contours[now][last] - contours[now][check]) < minDistance) { // need to delete point
                if ((last - check) <= 2) {
                    if (last - check == 2) {
                        newContourBegin = last - 1;
                    }
                    contours[now].erase(contours[now].begin() + last); // remove last
                    last--;
                } else {
                    if (newContourBegin == -1) {
                        contours[now][last] = contours[now][check]; // move last in found position
                        newContourBegin = last;
                    } else {
                        contours[now].erase(contours[now].begin() + last); // remove current
                        last--;
                        newContourBegin = check;
                    }
                }
                check = -1; // break for
                isDeleted = true;
            }
        }

        if (!isDeleted && newContourBegin != -1) { // Create new contours[now] and finish current
            vector<Point> newContour = {contours[now][newContourBegin]};
            newContour.insert(newContour.end(), contours[now].begin() + last, contours[now].end());
            contours.insert(contours.begin() + now + 1, newContour);

            contours[now].erase(contours[now].begin() + last, contours[now].end());
            if (!isContourAdded) {
                baseContour = now;
            }
            isNextContourAdded = true; // set next (created now) contours[now] as "added"
            break;
        }
        if (!isDeleted) {
            newContourBegin = -1;
        }
    }

    isContourAdded = isNextContourAdded;
}


float getAngle(Point center, Point point) {
    if (point.x - center.x == 0)
        if (point.y > center.y)
            return CV_PI / 2;
        else
            return -CV_PI / 2;
    else
        if (point.x < center.x)
            return atan((float)(point.y - center.y) / (float)(point.x - center.x)) + CV_PI;
        else
            return atan((float)(point.y - center.y) / (float)(point.x - center.x));
}

void degreeFilter(vector<Point>& contour, float minAngle) {
    float radAngle = minAngle / 180 * CV_PI;

    for (int last = 2; last < contour.size(); last++) {
        if (abs(getAngle(contour[last - 2], contour[last - 1]) - getAngle(contour[last - 1], contour[last])) < radAngle) {
            contour.erase(contour.begin() + last - 1); // remove middle
            last--;
        }
    }
}

void cvtRGBtoGray(Mat& image) {
    image.forEach<Point3_<uint8_t>>([&](Point3_<uint8_t>& pixel, const int position[]) -> void {
        pixel.x = pixel.y = pixel.z = (pixel.x + pixel.y + pixel.z) / 3;
    });
}

void sobel(Mat& image) {
    vector<vector<float>> gradients(image.cols, vector<float>(image.rows));
    float maxGrad, curGrad;
    image.forEach<uint8_t>([&](uint8_t& pixel, const int position[]) -> void {
        int nextXpos[2] {position[0] + 1, position[1]};
        int nextYpos[2] {position[0], position[1] + 1};
        curGrad = norm(Point((image.at<uint8_t>(nextXpos) - pixel), (image.at<uint8_t>(nextYpos) - pixel)));
        if (curGrad > maxGrad) {
            maxGrad = curGrad;
        }
        gradients[position[1]][position[0]] = curGrad;
    });

    for (int y = 0; y < gradients.size(); y++) {
        for (int x = 0; x < gradients[y].size(); x++) {
            image.at<uint8_t>({y, x}) = gradients[y][x] * 255 / maxGrad;
        }
    }
}

void canny(Mat& image) {
    size_t cols = image.cols;
    size_t rows = image.rows;
    vector<vector<pair<float, int>>> gradients(rows, vector<pair<float, int>>(cols));

    image.forEach<uint8_t>([&](uint8_t& pixel, const int position[]) -> void {
        if ((position[0] >= rows-1) || (position[1] >= cols-1)) {
            gradients[position[0]][position[1]].first = 0;
            gradients[position[0]][position[1]].second = 9;
            return;
        }
        int nextXpos[2] {position[0], position[1] + 1};
        int nextYpos[2] {position[0] + 1, position[1]};
        int dx =  image.at<uint8_t>(nextXpos) - pixel;
        int dy = -image.at<uint8_t>(nextYpos) + pixel;
        gradients[position[0]][position[1]].first = sqrt(dx*dx + dy*dy);

        if (dx == 0) {
            gradients[position[0]][position[1]].second = 2;
            return;
        }
        gradients[position[0]][position[1]].second = round((atan(dy / float(dx)) + (CV_PI / 8)) / (CV_PI / 4));
        if (gradients[position[0]][position[1]].second < 0) {
            gradients[position[0]][position[1]].second += 4;
        }
    });

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            int dir = gradients[y][x].second;
            float grad = gradients[y][x].first;
            if (
                (dir == 0 && ((x > 0 && gradients[y][x-1].first > grad) || (x < cols-1 && gradients[y][x+1].first > grad))) ||
                (dir == 1 && ((x > 0 && y < rows-1 && gradients[y+1][x-1].first > grad) || (x < cols-1 && y > 0 && gradients[y-1][x+1].first > grad))) ||
                (dir == 2 && ((y > 0 && gradients[y-1][x].first > grad) || (y < rows-1 && gradients[y+1][x].first > grad))) ||
                (dir == 3 && ((x > 0 && y > 0 && gradients[y-1][x-1].first > grad) || (x < cols-1 && y < rows-1 && gradients[y+1][x+1].first > grad))) ||
                (dir >= 4)
               ) {
                    image.at<uint8_t>({x, y}) = 0;
            } else {
                image.at<uint8_t>({x, y}) = 255;
            }
        }
    }
}
