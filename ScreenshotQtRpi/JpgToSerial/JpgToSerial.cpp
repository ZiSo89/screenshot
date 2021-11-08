#include "JpgToSerial.h"

JpgToSerial::JpgToSerial(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(updateWindow()));
	timer->start(30);

	timer_read_write = new QTimer(this);
	connect(timer_read_write, SIGNAL(timeout()), this, SLOT(writeData()));
	timer_read_write->start(50);

	m_serial = new QSerialPort(this);

	connect(ui.screenshot, SIGNAL(clicked()), this, SLOT(screenshotBtnPressed()));

	rs2::config cfg;
	//cfg.enable_stream(RS2_STREAM_DEPTH, 848, 100, RS2_FORMAT_Z16, 100);
	cfg.enable_stream(RS2_STREAM_INFRARED, 848, 100, RS2_FORMAT_Y8, 100);
	pipe.start(cfg);

	//
#ifdef __linux__
	if ((serial_port = serialOpen("/dev/ttyAMA0", 115200)) < 0)	// open serial port 460800  2000000 115200
	{
		std::cout << "Unable to open serial device: %s\n";
		return;
	}
	else
	{
		printf("open serial communication: Rpi<->Teensy \n");
	}

	if (wiringPiSetup() == -1)					/* initializes wiringPi setup */
	{
		std::cout << "Unable to start wiringPi: %s\n";
		return;
	}

#elif _WIN32
	openSerialPort();
#endif

}


void JpgToSerial::writeData()
{

	int packets_size = out_data.size();
	int read_result_size = 0;
	if (!out_data.empty()) {
		for (outData i : out_data) {
			//qDebug() << i.buf;
		}


#ifdef __linux__
		serialPrintf(serial_port, out_data[count_pos].buf);
		printf("cur packet num %.04d packet size: %u data %s \n", count_pos, (unsigned)strlen(out_data[count_pos].buf), out_data[count_pos].buf);

#elif _WIN32
		int s = m_serial->write(out_data[count_pos].buf, MAX_SIZE);
		while (m_serial->waitForReadyRead(3)) {
			memset(incomingData, 0, sizeof(incomingData));
			read_result_size = m_serial->read(incomingData, MAX_SIZE);
		}
		input_str += incomingData;

		bool same = false;
		if (strcmp(out_data[count_pos].buf, incomingData) == 0)
			same = true;

		qDebug("total size: %d count %d read_bytes_size %d  %s %d", packets_size, count_pos, read_result_size, incomingData, same);

#endif

		count_pos++;

		if (count_pos >= packets_size) {
			out_data.clear();
			count_pos = 0;
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

void JpgToSerial::openSerialPort()
{
	m_serial->setPortName("COM11");

	m_serial->setBaudRate(2000000); //QSerialPort::Baud9600 
	if (m_serial->open(QIODevice::ReadWrite)) {
		QMessageBox::information(this, "USB Status", "Connected");
	}
	else {
		QMessageBox::critical(this, tr("Error"), m_serial->errorString());
	}
}

void JpgToSerial::updateWindow()
{
	rs2::frameset data = pipe.wait_for_frames(); // Wait for next set of frames from the camera
	//rs2::frame depth = data.get_depth_frame().apply_filter(color_map);
	rs2::frame ir_frame = data.get_infrared_frame();

	// Query frame size (width and height)
	//const int w = depth.as<rs2::video_frame>().get_width();
	//const int h = depth.as<rs2::video_frame>().get_height();

	// Create OpenCV matrix of size (w,h) from the colorized depth data
	frame = cv::Mat(cv::Size(848, 100), CV_8UC1, (void*)ir_frame.get_data(), cv::Mat::AUTO_STEP);

	//cv::cvtColor(frame, frame, cv::COLOR_RGB2BGR);
	//frame.convertTo(frame, CV_8UC3); // CV_8U should work as well
	//cvtColor(frame, frame, cv::COLOR_RGB2GRAY);

	int uartInputIndex = 0;

	memset(uartInput, '\0', sizeof(uartInput));

#ifdef __linux__
	while (serialDataAvail(serial_port))
	{
		uartInput[uartInputIndex] = serialGetchar(serial_port);
		if ('\0' == uartInput[uartInputIndex]) break; // terminator received
		++uartInputIndex;
	}
#endif

	if ((strncmp("take_screenshot", uartInput, 15) == 0)) {
		take_screenshot = true;
	}

	if (take_screenshot) {

		cout << "Take Screenshot\n";

		imwrite(filename, frame);

		std::vector<uchar> buf;
		cv::imencode(filename, frame, buf);
		uchar* enc_msg = new uchar[buf.size()];
		for (int i = 0; i < buf.size(); i++)
			enc_msg[i] = buf[i];

		string encoded = base64_encode(enc_msg, buf.size());

		const char* encoded_str = encoded.c_str();

		int packet_count = 0;
		string incom_str="";
		out_data.clear();
		for (int i = 0; i < encoded.length(); i = i + char_buf_size)
		{
			int j = 0;
			char out_str_64_buf[MAX_SIZE];
			for (int ch = i; ch < i + char_buf_size && ch < encoded.length(); ch++)
			{
				out_str_64_buf[j] = encoded_str[ch];
				j++;
			}
			out_str_64_buf[j] = '\0';
			// actually send the packet

			outData data;
			data.id = packet_count;
			strcpy(data.buf, out_str_64_buf);
			out_data.push_back(data);

			packet_count++;

		}
		outData data;
		strcpy(data.buf, "\n");
		out_data.push_back(data);

		//string dec_jpg = base64_decode(encoded);
		//std::vector<uchar> data(dec_jpg.begin(), dec_jpg.end());
		//cv::Mat img = cv::imdecode(cv::Mat(data), 1);
		//
		//if (img.empty())
		//	qDebug() << "ERROR! blank frame grabbed\n";
		//else {
		//	imshow(filename, img);
		//}

		take_screenshot = false;
	}

	const uchar* qImageBuffer = (const uchar*)frame.data;	// Copy input Mat
	QImage qt_draw_img = QImage(qImageBuffer, frame.cols, frame.rows, frame.step, QImage::Format_Grayscale8); //Format_ARGB32 Format_RGB888
	ui.camera->setPixmap(QPixmap::fromImage(qt_draw_img));

}

void JpgToSerial::screenshotBtnPressed()
{
	take_screenshot = true;
}