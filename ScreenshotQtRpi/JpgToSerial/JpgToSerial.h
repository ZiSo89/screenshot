#pragma once

#define MAX_SIZE 64
#include <QtWidgets/QMainWindow>
#include "ui_JpgToSerial.h"
#include <QTimer>
#include <QElapsedTimer>
#include <QDebug>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QSpinBox>
#include <QMessageBox>
#include <opencv2/opencv.hpp>   // Include OpenCV API
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include "base64.h"
//#include "hidapi.h"
#include <iostream>
#include <string>
#include <vector>

#ifdef __linux__
#include <wiringPi.h>
#include <wiringSerial.h>
#endif

using namespace std;

class JpgToSerial : public QMainWindow
{
	Q_OBJECT

public:
	JpgToSerial(QWidget* parent = Q_NULLPTR);
	QTimer* timer;
	QTimer* timer_read_write;
	QImage qt_image;
	cv::Mat frame; // this will contain the image from the webcam
	rs2::pipeline pipe;
	rs2::colorizer color_map;
	char filename[16] = { "frame.jpg" }; // For filename
	char incomingData[MAX_SIZE];
	int count_pos = 0;
	int serial_port;
	QElapsedTimer timer_out;
	QSerialPort* m_serial;

private:
	Ui::JpgToSerialClass ui;
	int m_transactionCount = 0;
	const int char_buf_size = MAX_SIZE-1;
	//QSerialPort* m_serial = nullptr;
	char uartInput[64];

	struct outData {
		int id;
		char buf[MAX_SIZE];
	};

	vector<outData> out_data;
	string input_str = "";
	bool take_screenshot = false;

private slots:
	void writeData();
	void updateWindow();
	void screenshotBtnPressed();
	void openSerialPort();
};
