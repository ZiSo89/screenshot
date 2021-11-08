#include <cstdio>
#include <opencv2/opencv.hpp>
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include <stdio.h>
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <wiringPi.h>
#include <wiringSerial.h>
#include "base64.h"
#include <unistd.h>
#include <pthread.h>
#include <chrono>

using namespace cv;
using namespace std;

#define MAX_SIZE 64

int serial_port;
const int char_buf_size = MAX_SIZE - 1;
char out_str_64_buf[MAX_SIZE];
char uartInput[MAX_SIZE];

struct outData {
	int id;
	char buf[MAX_SIZE];
};
vector<outData> out_data;
int count_pos = 0;
string input_str = "";

unsigned int millis() {
	return chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
}

unsigned int prev_millis = 0;

void* worker_thread(void* arg)
{
	while (true) {

		if (millis() - prev_millis > 30) {
			prev_millis = millis();
			int packets_size = out_data.size();
			int read_result_size = 0;
			if (!out_data.empty()) {
				for (outData i : out_data) {
					//qDebug() << i.buf;
				}


				serialPrintf(serial_port, out_data[count_pos].buf);
				printf("cur packet num %.04d packet size: %u data %s \n", count_pos, (unsigned)strlen(out_data[count_pos].buf), out_data[count_pos].buf);

				count_pos++;

				if (count_pos >= packets_size) {
					out_data.clear();
					count_pos = 0;
					//serialPutchar(serial_port,'\n');
				}
			}
			else if (input_str.length() > 0) {

				try {
					string dec_jpg = base64_decode(input_str);

					std::vector<uchar> data(dec_jpg.begin(), dec_jpg.end());
					cv::Mat img = cv::imdecode(cv::Mat(data), 1);

					if (img.empty())
						cout << "ERROR! blank frame grabbed\n";
					else {

						imshow("encoded", img);

					}
				}
				catch (const std::exception& e) {

				}

				input_str = "";
			}
		}
	}
	pthread_exit(NULL);
}

int openSerial() {

	if ((serial_port = serialOpen("/dev/ttyAMA0", 115200)) < 0)	// open serial port 460800  2000000 115200
	{
		std::cout << "Unable to open serial device: %s\n";
		return 1;
	}
	else
	{
		printf("open serial communication: Rpi<->Teensy \n");
	}

	if (wiringPiSetup() == -1)					/* initializes wiringPi setup */
	{
		std::cout << "Unable to start wiringPi: %s\n";
		return 1;
	}

	return serial_port;
}

int main(int argc, char** argv)
{
	pthread_t my_thread;
	int ret;

	ret = pthread_create(&my_thread, NULL, &worker_thread, NULL);

	if (ret != 0) {
		printf("Error: pthread_create() failed\n");
		exit(EXIT_FAILURE);
	}



	String windowName = "ScreenShot"; //Name of the window
	namedWindow(windowName); // Create a window
	openSerial();
	// Read the image file
	Mat image = imread("frame.jpg");
	// Check for failure
	if (image.empty())
	{
		cout << "Could not open or find the image" << endl;
		cin.get(); //wait for any key press
		return -1;
	}

	imshow(windowName, image);

	while (true) {

		int uartInputIndex = 0;

		memset(uartInput, '\0', sizeof(uartInput));

		while (serialDataAvail(serial_port))
		{
			uartInput[uartInputIndex] = serialGetchar(serial_port);
			if ('\0' == uartInput[uartInputIndex]) break; // terminator received
			++uartInputIndex;
		}

		if (waitKey(30) == 'c' || (strncmp("take_screenshot", uartInput, 15) == 0)) {
			cout << "Take Screenshot\n";
			std::vector<uchar> buf;

			cv::imencode("frame.jpg", image, buf);
			uchar* enc_msg = new uchar[buf.size()];
			for (int i = 0; i < buf.size(); i++)
				enc_msg[i] = buf[i];
			string encoded = base64_encode(enc_msg, buf.size());

			const char* encoded_str = encoded.c_str();

			//cout << encoded<<endl;

			int packet_count = 0;

			string incom_str = "";

			for (int i = 0; i < encoded.length(); i = i + char_buf_size)
			{
				int j = 0;
				memset(out_str_64_buf, '\0', sizeof(out_str_64_buf));
				for (int ch = i; ch < i + char_buf_size && ch < encoded.length(); ch++)
				{
					out_str_64_buf[j] = encoded_str[ch];
					//cout << out_str_64_buf[j];
					j++;
				}
				out_str_64_buf[j] = '\0';
				//cout << '\n';

				outData data;
				data.id = packet_count;
				strcpy(data.buf, out_str_64_buf);
				out_data.push_back(data);

				// actually send the packet

				//printf("%d -> cur packet num %.04d packet size: %d data %s \n", same, packet_count, uartInputIndex, uartInput);
				//
				//incom_str += uartInput;

				packet_count++;
			}
			outData data;
			strcpy(data.buf, "\n");
			out_data.push_back(data);

			imshow(windowName, image); // Show our image inside the created window.
		}
	}
	pthread_exit(NULL);
	return 0;
}
