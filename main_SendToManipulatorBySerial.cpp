#include <iostream>
#include <string>
#include <windows.h>
using namespace std;

#define SERIAL_PORT_NAME "\\\\.\\COM1"
#define BAUDRATE CBR_57600
#define BYTE_SIZE 8
#define STOP_BITS ONESTOPBIT
#define PARITY NOPARITY
#define BUFFER_SIZE 1024

#define INPUT_TXT_FILENAME "MANIPULATOR_PATH.txt"
#define END_INPUT_SYMBOL "E"


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

void sendToSerialPort(HANDLE serialPort, string message) {
  const char *str = message.c_str();
  DWORD dwSize = sizeof(str);
  DWORD dwBytesWritten;
  if (!WriteFile(serialPort, str, dwSize, &dwBytesWritten, NULL)) {
    cout << "Error occurred while writing in port" << endl;
    exit(-5);
  }
  cout << "Bytes written: " << dwBytesWritten << "/" << dwSize << endl;
}
string readFromSerialPort(HANDLE serialPort) {
  char szBuff[BUFFER_SIZE + 1] = {0};
  DWORD dwBytesRead;
  if (!ReadFile(serialPort, szBuff, BUFFER_SIZE, &dwBytesRead, NULL)) {
    cout << "Error occurred while reading from port" << endl;
    exit(-6);
  }
  cout << "Bytes read: " << dwBytesRead << endl;
  return string(szBuff);
}


int main() {
  HANDLE serialPort = openSerialPort();

//  sendToSerialPort(serialPort, "Q");

//  // „итаем из файла построчно и закидываем в последовательный порт
//  ifstream in(INPUT_TXT_FILENAME);
//  while (in.good()) {
//    string line;
//    getline(in, line);
//    sendToSerialPort(serialPort, line);
//  }
//  in.close();
//  // —читали из файла
//  sendToSerialPort(serialPort, END_INPUT_SYMBOL);

  closeSerialPort(serialPort);
}
