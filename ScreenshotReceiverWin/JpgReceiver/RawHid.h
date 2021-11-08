#pragma once

#define REPORT_ID 0x00

#include <vector>
#include <iostream>
#include <string>
#include "hidapi.h"

using namespace std;

class RawHid
{
private:
	string devive_id = "vid_16c0&pid_0486&mi_00";
	std::vector<std::string> paths;
	hid_device* connected_device = NULL;
	int res;

public:
	~RawHid() {}
	RawHid() {}


	bool isOpen()
	{
		if (connected_device != NULL)
			return true;
		else
			return false;
	}

	hid_device* getDevice()
	{
		return connected_device;
	}

	string receivedString(int* res) {
		static string s;
		string str;
		unsigned char buf[64];
		memset(buf, 0, sizeof(buf));
		*res = hid_read(connected_device, buf, sizeof(buf));
		char txt[64];
		static int packet_receive_count = 0, packet_transmit_count = 0;
		if (*res > 0)
		{
			int i = 0, j = 0;
			memset(txt, 0, sizeof(txt));
			while (i < *res)
			{
				if (buf[i] != 0)
				{
					txt[j] = (char)buf[i]; // add char to txt string
					j++;
				}
				i++;
			}
			memset(buf, 0, sizeof(buf));
			packet_receive_count = packet_receive_count + 1;
		}

		if (packet_receive_count != packet_transmit_count)
		{

			int len = strlen(txt);


			if (txt[len - 1] == '\n')
			{
				s += txt;
				str = s;
				s.clear();
			}
			else
			{
				s += txt;
			}

			packet_transmit_count = packet_receive_count;
		}
		return str;
	}

	int scanDevices() {
		paths.clear();
		paths.push_back("None");

		if (hid_init() != 0)
			printf("error");

		struct hid_device_info* cur_dev;
		hid_free_enumeration(NULL);
		cur_dev = hid_enumerate(0x0, 0x0);

		while (cur_dev) {
			// Add it to the List Box.
			string s;
			paths.push_back(cur_dev->path);
			cur_dev = cur_dev->next;
		}
		int count = 0;
		for (string str : paths) {
			size_t found = str.find(devive_id);
			if (found != std::string::npos)
			{
				return count;
			}
			count++;
		}
		return -1;
	}

	string connect() {

		string status = "";
		int selected_index = scanDevices();

		if (connected_device)
			hid_close(connected_device);
		connected_device = NULL;

		if (paths.size() > 0)
		{

			if (selected_index != -1)
			{
				connected_device = hid_open_path(paths[selected_index].c_str());
				if (!connected_device) {
					printf("Device Error Unable To Connect to Device");
					status = "Device Error Unable To Connect to Device";
				}
				else
				{
					hid_set_nonblocking(connected_device, 1);
					printf("Connected to Device:");
					status = "Connected to Device";
					cout << paths[selected_index];
				}
			}
			else
			{
				status = "No device to Connect";
			}
		}
		return status;
	}

};