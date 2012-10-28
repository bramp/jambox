#include "usb.h"

#include <libusb.h>
#include <log4cplus/logger.h>

#define DEFAULT_INTERFACE 0

using namespace log4cplus;
static Logger logger = Logger::getInstance("usb");

UsbContext::UsbContext() {
	int r = libusb_init(&ctx); //initialize the library for the session we just declared
	if(r < 0) {
		throw "libusb_init error";
	}
	libusb_set_debug(ctx, 3); //set verbosity level to 3, as suggested in the documentation
}

UsbContext::~UsbContext() {
	 libusb_exit(ctx);
}

UsbDevice::UsbDevice(libusb_device_handle *dev) : dev(dev) {
	if (!dev)
		return;

	int ret;

	// Ensure we are the only users of the device
	ret = libusb_kernel_driver_active(dev, DEFAULT_INTERFACE);
	if(ret == 1) {
		LOG4CPLUS_DEBUG(logger, "Kernel driver is active, detaching...");

		ret = libusb_detach_kernel_driver(dev, DEFAULT_INTERFACE);
		if(ret != 0) {
			//LOG4CPLUS_ERROR(logger, "Kernel driver failed to be detached");
			throw "Kernel driver failed to be detached";
		}

		LOG4CPLUS_DEBUG(logger, "Kernel driver is detached");
	}

	ret = libusb_claim_interface(dev, DEFAULT_INTERFACE);
	if(ret != 0) {
		throw "Cannot Claim Interface";
	}

	LOG4CPLUS_DEBUG(logger, "Interface claimed");
}

UsbDevice::~UsbDevice() {
	if (!dev)
		return;

	libusb_release_interface(dev, DEFAULT_INTERFACE);
	libusb_close(dev);
}
