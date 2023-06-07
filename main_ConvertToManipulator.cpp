#include "Kinematics/Kinematics.h"

#include <iostream> // cout
#include <fstream> // input/output with file
#include <string> // STL string
#include <vector> // STL vector
#include <sstream> // isstringstream

using namespace std;

#define INPUT_TXT_FILENAME "results/RESULT.txt"
#define OUTPUT_TXT_FILENAME "results/MANIPULATOR_PATH.txt"

#define DESK_DISTANCE_X -20 // Отступ до центра доски по X. Доска перпендикулярна X
#define DESK_MIN_Y -20 // Ограничения положения и размера доски
#define DESK_MAX_Y 20
#define DESK_MIN_Z 5
#define DESK_MAX_Z 45
#define DESK_DISTANCE_X_TO_MARKER -2 // Расстояние на которое отходит маркер от доски, чтобы не рисовать на ней

#define DESK_Y_SIZE (DESK_MAX_Y - DESK_MIN_Y)
#define DESK_Z_SIZE (DESK_MAX_Z - DESK_MIN_Z)

#define JOINTS_COUNT 6 // Кол-во вершин в манипуляторе


#define JOINT_POS_MIN_DEG 0.0
#define JOINT_POS_MAX_DEG 360.0
const float minPoses[] = {
    JOINT_POS_MIN_DEG, // min 1
    73, // min 2
    83, // min 3
    78, // min 4
    3,  // min 5
    120, // min 6
};
const float maxPoses[] = {
    JOINT_POS_MAX_DEG, // max 1
    283, // max 2
    275, // max 3
    253, // max 4
    245, // max 5
    193, // max 6
};
const float defaultPoses[] = {135, 155, 132, 164, 180, 170};


vector<float> getManipulatorPosesByPoint(float x, float y, float minX, float xSize, float minY, float ySize, float addDeskX = 0) {
    float tmpPoses[JOINTS_COUNT];
    float yDesk = float(x - minX) / xSize * DESK_Y_SIZE + DESK_MIN_Y; // Конвертируем координаты под размер доски
    float zDesk = float(y - minY) / ySize * DESK_Z_SIZE + DESK_MIN_Z;
    getAnglesByTargetPoint(DESK_DISTANCE_X + addDeskX, yDesk, zDesk, defaultPoses, tmpPoses, JOINTS_COUNT, minPoses, maxPoses); // Получаем положения вершин манипулятора для точки
    vector<float> res;
    for (size_t i = 0; i < JOINTS_COUNT; i++) { // Копируем в вектор
        res.push_back(tmpPoses[i]);
    }
    return res;
}


int main() {
    // Считываем значения из файла
    vector<vector<pair<int, int>>> contours;

    ifstream in(INPUT_TXT_FILENAME);
    if (!in.is_open()) {
      cout << "File not opened" << endl;
      exit(-1);
    }

    int minX, minY, maxX, maxY;
    bool firstValueGotten;
    while (in.good()) {
        string line;
        getline(in, line);

        istringstream streamLine(line);

        vector<pair<int, int>> contour;
        int x, y;
        while (streamLine >> x >> y) {
            contour.push_back(pair<int, int>(x, y));

            if (!firstValueGotten) { // Попутно считаем минимум и максимум по X и Y
                minX = maxX = x;
                minY = maxY = y;
                firstValueGotten = true;
            } else {
                minX = min(x, minX);
                maxX = max(x, maxX);
                minY = min(y, minY);
                maxY = max(y, maxY);
            }
        }
        if (contour.size() >= 2) {
            contours.push_back(contour);
        }
    }
    in.close();
    // Считали из файла

    cout << "Total contours:" << contours.size() << endl;

    cout << "X:" << minX << " - " << maxX << endl;
    cout << "Y:" << minY << " - " << maxY << endl;
    float xSize = maxX - minX;
    float ySize = maxY - minY;


    // Прогоняемся по контурам и генерируем положения манипулятора
    vector<vector<float>> manipulatorPath;

    for (auto contour: contours) {
        manipulatorPath.push_back(getManipulatorPosesByPoint(contour[0].first, contour[0].second, minX, xSize, minY, ySize, -DESK_DISTANCE_X_TO_MARKER)); // Добавляем точку "над" первой в плоскости доски
        for (auto point: contour) {
            manipulatorPath.push_back(getManipulatorPosesByPoint(point.first, point.second, minX, xSize, minY, ySize));
        }
        manipulatorPath.push_back(getManipulatorPosesByPoint(contour[contour.size() - 1].first, contour[contour.size() - 1].second, minX, xSize, minY, ySize, -DESK_DISTANCE_X_TO_MARKER)); // Добавляем точку "над" последней в плоскости доски, чтобы он убрал маркер
    }

    // Выводим результат в файл
    ofstream out(OUTPUT_TXT_FILENAME);
    for (auto pos: manipulatorPath) {
        for (auto val: pos) {
            out << val << " ";
        }
        out << endl;
    }
    out.close();
    return 0;
}
