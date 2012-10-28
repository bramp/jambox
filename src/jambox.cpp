/**
 * Jambox tool
 * by Andrew Brampton (bramp.net)
 * 
 * Interacts with your Jawbone Jambox
 * 
 * TODO
 *  Stop using libusb_open_device_with_vid_pid, so we get better error handling
 */
#include "usb.h"

#include <iostream>
#include <libusb.h>
#include <memory>
#include <cstring>

// Depend on log4cplus
#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>

using namespace std;

#define USB_TIMEOUT 1000

uint16_t vendor_id      = 0x1f6f;
uint16_t product_id_hid = 0x0023; // Product ID for the HID device
uint16_t product_id     = 0x8000;

using namespace log4cplus;
static Logger logger = Logger::getInstance("jambox");


libusb_device_handle* jambox_open(UsbContext &ctx) {

	UsbDevice dev(
		libusb_open_device_with_vid_pid(ctx, vendor_id, product_id_hid)
	);

	if (dev) {
		LOG4CPLUS_INFO(logger, "Switching jambox into update mode");

		// We need to send the mode switching command
		unsigned char* buf = (unsigned char*)"\x41\x54\x2b\x4a\x42\x44\x46\x55\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
		unsigned int buf_len = 0x23;

		//ret = usb_control_msg(devh, USB_TYPE_CLASS + USB_RECIP_INTERFACE, 0x0000009, 0x0000200, 0x0000000, buf, 0x0000023, 1000);

		libusb_control_transfer(dev,
			LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 
			0x09, 0x200, 0, buf, buf_len, USB_TIMEOUT
		);

		// No need to check for error, as the jambox quickly disconnects and
		// and control command fails, but has actually worked.

		usleep(1000000); // Wait 1 second
		// TODO Do this better
	}

	// Now find the real device
	return libusb_open_device_with_vid_pid(ctx, vendor_id, product_id);
}


void print_bytes(const unsigned char *bytes, int len) {
    int i;
    if (len > 0) {
	for (i=0; i<len; i++) {
	    printf("%02x ", (int)((unsigned char)bytes[i]));
	}
	printf("\"");
        for (i=0; i<len; i++) {
	    printf("%c", isprint(bytes[i]) ? bytes[i] : '.');
        }
        printf("\"");
    }
    printf("\n");
}

uint16_t seq = 0; 

struct jambox_header { // 22 bytes
	unsigned char a; // 0 always 0x00
	unsigned char b; // 1 always 0xfc
	uint8_t len;     // 2 len beyond this point
	unsigned char c; // 3 always 0xc2
	unsigned char d; // 4 always 0x00
	unsigned char e; // 5 always 0x00
	unsigned char f; // 6 number of bytes / 2 from d
	unsigned char g; // 7 always 0x00
	uint16_t seq;    // 8
	unsigned char tmp2[12]; // 10
};

/*
struct jambox_req_name {
	struct jambox_header header;
	char name[??];// shave 2 from the header and this works
}
*/
union jambox_req {
	unsigned char data[255];
	struct jambox_header header;
};

void create_request(jambox_req *req, uint8_t len) {
	memset(req, 0, sizeof(*req));
	req->header.a = 0x00;
	req->header.b = 0xfc;
	req->header.len = len;
	req->header.c = 0xc2;
	req->header.d = 0x00;
	req->header.e = 0x00;
	//req->header.f = 0x00;
	req->header.g = 0x00;
	req->header.seq = seq++;
}

void create_name_request(jambox_req *req) {
	create_request(req, 0x23);
	req->header.f = 0x11; // TODO Change
	req->header.tmp2[0] = 0x03;
	req->header.tmp2[1] = 0x70;
	req->header.tmp2[2] = 0x00;
	req->header.tmp2[3] = 0x00;
	req->header.tmp2[4] = 0x08;
	req->header.tmp2[5] = 0x01;
	req->header.tmp2[6] = 0x09;
}

int send_request(struct libusb_device_handle *dev, jambox_req * req) {
	const char *data = req->data;
	int len = req->header.len;
	return libusb_control_transfer(dev, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 0, 0, 0, data, len, USB_TIMEOUT);
}

/*
	\x00\xfc\x67\xc2\x00\x00\x33\x00\x13\x75\x03\x70\x00\x00\xb4
	\x02\x2b\x00\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00
	\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00
	\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00
	\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00
	\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00
	\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00
*/

void create_pair_request(jambox_req *req) {
	create_request(req, 0x67);
	req->header.f = 0x33; // TODO Change
	req->header.tmp2[0] = 0x03;
	req->header.tmp2[1] = 0x70;
	req->header.tmp2[2] = 0x00;
	req->header.tmp2[3] = 0x00;
	req->header.tmp2[4] = 0xb4;
	req->header.tmp2[5] = 0x02;
	req->header.tmp2[6] = 0x2b;
	req->header.tmp2[7] = 0x00;
	req->header.tmp2[8] = 0x80;
}


int main() {

	// Set up logging
    BasicConfigurator config;
    config.configure();
    logger.setLogLevel(TRACE_LOG_LEVEL);

	int r;
	UsbContext usb;
	UsbDevice dev (jambox_open(usb));

	if (!dev) {
		//LOG4CPLUS_TRACE_STR(logger, "Main::Main()");
		LOG4CPLUS_ERROR(logger, "Failed to find a jambox");
		return NULL;
	}

	


	/*
	unsigned char data[] = {0x00, 0xFC, 0x23, 0xC2, 0x00, 0x00, 0x11, 0x00,
	        0x3B, 0x71,/ 0x03, 0x70, 0x00, 0x00, 0x08, 0x01,
	        0x09, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
	        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	        0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	int len = sizeof(data) / sizeof(*data);
	*/

	jambox_req req;
	int actual = 0; //used to find out how many bytes were written

	create_name_request(&req);

	unsigned char * data = (unsigned char *)&req;
	int len = 0x26;

	print_bytes(data, len);

	cout<<"Writing Data"<<endl;
	r = libusb_control_transfer(dev, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 0, 0, 0, data, len, USB_TIMEOUT);
	if(r == len) {
		cout<<"Writing Successful!"<<endl;
	} else {
		cout<<"Write Error "<< r << " " << actual << "/" << len << endl;
	}

	//r = libusb_interrupt_transfer(dev, 0x81, in, sizeof(in), &actual, USB_TIMEOUT);
	r = libusb_interrupt_transfer(dev, 0x81, &req.data[1], sizeof(req) - 1, &actual, USB_TIMEOUT);
	printf("interrupt read returned %d, %d bytes: ", r, actual);
	print_bytes((unsigned char *)&req, actual + 1);
	//printf("\n");

	printf("device name: %s\n", &req.data[20]);

	create_pair_request(&req);
	len = 0x6a;
	print_bytes((unsigned char *)&req, req.header.len + 3);
	

	r = libusb_control_transfer(dev, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 0, 0, 0, data, len, USB_TIMEOUT);
	if(r == len) {
		cout<<"Writing Successful!"<<endl;
	} else {
		cout<<"Write Error "<< r << " " << actual << "/" << len << endl;
	}

	//r = libusb_interrupt_transfer(dev, 0x81, in, sizeof(in), &actual, USB_TIMEOUT);
	r = libusb_interrupt_transfer(dev, 0x81, &req.data[1], sizeof(req) - 1, &actual, USB_TIMEOUT);
	printf("interrupt read returned %d, %d bytes: ", r, actual);
	print_bytes((unsigned char *)&req, actual + 1);
	//printf("\n");

	printf("device name: %s\n", &req.data[20]);


	return 0;
}

