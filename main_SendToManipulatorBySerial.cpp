#include <iostream>
#include <fstream> // input/output with file
#include <string>
#include <windows.h>
using namespace std;

#define SERIAL_PORT_NAME "\\\\.\\COM5"
#define BAUDRATE CBR_57600
#define BYTE_SIZE 8
#define STOP_BITS ONESTOPBIT
#define PARITY NOPARITY
#define BUFFER_SIZE 10240

#define INPUT_TXT_FILENAME "results/MANIPULATOR_PATH.txt"

#define ANSWER_BYTE_SIZE 15
#define POINTS_WITHOUT_ANSWER 10


HANDLE openSerialPort() {
  // Open serial port
  HANDLE serialHandle = CreateFile(SERIAL_PORT_NAME, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (!serialHandle) {
    cout << "Error occurred while creating serial" << endl;
    exit(-1);
  }

  // Do some basic settings
  DCB serialParams = {0};
  serialParams.DCBlength = sizeof(serialParams);

  if (!GetCommState(serialHandle, &serialParams)) {
    cout << "Error occurred while getting comm state" << endl;
    exit(-2);
  }
  serialParams.BaudRate = BAUDRATE;
  serialParams.ByteSize = BYTE_SIZE;
  serialParams.StopBits = STOP_BITS;
  serialParams.Parity = PARITY;
  if (!SetCommState(serialHandle, &serialParams)) {
    cout << "Error occurred while setting base serial parameters" << endl;
    exit(-3);
  }

  // Set timeouts
  COMMTIMEOUTS timeout = {0};
  timeout.ReadIntervalTimeout = 50;
  timeout.ReadTotalTimeoutConstant = 50;
  timeout.ReadTotalTimeoutMultiplier = 50;
  timeout.WriteTotalTimeoutConstant = 50;
  timeout.WriteTotalTimeoutMultiplier = 10;

  if (!SetCommTimeouts(serialHandle, &timeout)) {
    cout << "Error occurred while setting comm timeouts" << endl;
    exit(-4);
  }

  return serialHandle;
}

void closeSerialPort(HANDLE serialPort) {
  CloseHandle(serialPort);
}

string readFromSerialPort(HANDLE serialPort, size_t readBytes = 10240) {
  char szBuff[readBytes + 1] = {0};
  DWORD dwBytesRead;
  if (!ReadFile(serialPort, szBuff, readBytes, &dwBytesRead, NULL)) {
    cout << "Error occurred while reading from port" << endl;
    exit(-6);
  }
  cout << "Bytes read: " << dwBytesRead << endl;
  return string(szBuff);
}
//string readFromSerialPortIfAvailable(HANDLE serialPort) {
//
//}

void sendToSerialPort(HANDLE serialPort, string message) {
  const char *str = message.c_str();
  DWORD dwSize = message.length();
  DWORD dwBytesWritten;
  cout << "String: " << str << endl;
  BOOL res = FALSE;
  size_t attempts = 0;
  while (attempts < 5) {
    res = WriteFile(serialPort, str, dwSize, &dwBytesWritten, NULL);
    if (res) {
      cout << "Bytes written: " << dwBytesWritten << "/" << dwSize << endl;
//      WriteFile(serialPort, "C", 1, &dwBytesWritten, NULL);
//      cout << "Written C. Response:" << endl;
//      cout << readFromSerialPort(serialPort) << endl;
      break;
    }
    cout << "Error occurred while writing in port. Retrying..." << endl;
    attempts += 1;
    Sleep(1000);
  }
  if (!res) {
    cout << "Error occurred 5 times while writing in port" << endl;
    exit(-5);
  }
}



int pointsWithoutAnswer = POINTS_WITHOUT_ANSWER;
int main() {
  HANDLE serialPort = openSerialPort();
  cout << "Port opened." << endl;

//  cout << readFromSerialPort(serialPort) << endl;
  //sendToSerialPort(serialPort, "Q");

  // „итаем из файла построчно и закидываем в последовательный порт
  ifstream in(INPUT_TXT_FILENAME);
  if (!in.is_open()) {
    cout << "File not opened" << endl;
    exit(-1);
  }
  size_t linesWritten = 0;
  while (in.good()) {
    string line;
    getline(in, line);
    sendToSerialPort(serialPort, line);
    sendToSerialPort(serialPort, "\n");
    linesWritten++;
    cout << "Lines written: " << linesWritten << endl;
    if (pointsWithoutAnswer <= 0) {
      cout << "Wait for 'POINT REACHED' from port..." << endl;
      cout << readFromSerialPort(serialPort) << endl;
    } else {
      pointsWithoutAnswer--;
      cout << "NO ANSWER" << endl;
    }
  }
  in.close();
  // —читали из файла
//  sendToSerialPort(serialPort, END_INPUT_SYMBOL);
//  sendToSerialPort(serialPort, "\n");

  closeSerialPort(serialPort);
}
