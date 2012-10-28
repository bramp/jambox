#include <cstddef>

struct libusb_context;
struct libusb_device_handle;

/**
 * Simple wrapper around the libusb_context
 * To ensure the context always gets freed
 */
class UsbContext {
	struct libusb_context *ctx;

public:
	UsbContext();
	~UsbContext();

	operator struct libusb_context *() {
		return ctx;
	}

	operator bool () {
		return ctx != NULL;
	}

private:
	UsbContext(const UsbContext &); // No copy constructor
};

/**
 * Simple wrapper around the libusb_device_handle
 * Ensures the following
 * 1) The kernel is not using interface 0
 * 2) Interface 0 is claimed
 * 3) Interface 0 is always released
 * 4) The handle is always released
 */
class UsbDevice {
	struct libusb_device_handle *dev;

public:
	UsbDevice(struct libusb_device_handle *dev);
	~UsbDevice();

	operator struct libusb_device_handle *() {
		return dev;
	}

	operator bool () {
		return dev != NULL;
	}

private:
	UsbDevice(const UsbDevice &); // No copy constructor
};