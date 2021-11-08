// JpgReceiver.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include <opencv2/opencv.hpp>   // Include OpenCV API
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "hidapi.h"
#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include "RawHid.h"
#include "base64.h"

#define MAX_SIZE 64
using namespace std;
using namespace cv;


unsigned int millis() {
	return chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
}

void uncharToChar(unsigned char ar1[], char ar2[], int hm)
{
	for (int i = 0; i < hm; i++)
	{
		ar2[i] = static_cast<char>(ar1[i]);
	}
}


int main()
{
	bool do_compose = false;
	RawHid raw;
	string state_raw = raw.connect();
	std::cout << state_raw << endl;
	int res;
	string encoded = "";
	unsigned int prev_millis = 0;
	unsigned int start_millis = 0;
	unsigned char buf[MAX_SIZE];
	char txt[MAX_SIZE];
	Mat img = cv::Mat::zeros(cv::Size(848, 100), CV_64FC1);
	int packet_count = 0;
	namedWindow("encoded", cv::WINDOW_AUTOSIZE);// Create a window for display.
	start_millis - millis();

	while (true) {

		memset(txt, '\0', sizeof(txt));
		memset(buf, '\0', sizeof(buf));
		res = hid_read(raw.getDevice(), buf, sizeof(buf));


		//static int packet_receive_count = 0, packet_transmit_count = 0;

		if (res > 0)
		{
			prev_millis = millis();
			uncharToChar(buf, txt, MAX_SIZE);
			packet_count++;
			cout << buf << endl;

			char* output = NULL;
			do_compose = false;
			output = strstr(txt, "\n");
			if (output) {
				cout << "------------Compose---------------\n";
				do_compose = true;
			}
			encoded += txt;

			memset(buf, '\0', sizeof(buf));
		}

		if (do_compose && millis() - prev_millis > 100) {

			encoded.erase(std::remove(encoded.begin(), encoded.end(), '\n'), encoded.end());

			std::ofstream out_text("EncodedBase64ToJPG.txt");
			out_text << encoded;
			out_text.close();

			try {
				string dec_jpg = base64_decode(encoded);
				std::vector<uchar> data(dec_jpg.begin(), dec_jpg.end());
				img = cv::imdecode(cv::Mat(data), 1);

			}
			catch (const std::exception& e) {}

			if (img.empty()) {
				cout << "ERROR! blank frame grabbed\n";
			}


			printf("%d packets of 64bytes transferred,\n%d total bytes,\n%d in sec",
				packet_count, packet_count * 64, (millis() - start_millis) / 1000);

			encoded = "";
			packet_count = 0;
			start_millis = millis();
			do_compose = false;
		}

		imshow("encoded", img);

		if (waitKey(10) == 27)
		{
			cout << "Esc key is pressed by user. Stoppig the video" << endl;
			break;
		}

		if (waitKey(10) == 's')
		{
			cout << "Send command to take a screenshot\n" << endl;
			start_millis = millis();
			img = cv::Mat::zeros(cv::Size(848, 100), CV_64FC1);
			char ch_to_send[64] = { " take_screenshot" };
			hid_write(raw.getDevice(), (const unsigned char*)ch_to_send, sizeof(ch_to_send));
		}

	}
}