

/**********************************************************************
* $Id: $
*
* written by Timo Boettcher
* fnordlicht (near) spida (.) net
*
* based on powerSwitch.c by Christian Starkjohann
*
* (C) 2008,2009 Timo Boettcher
* This is free software. You may redistribute copies of it under the
* terms of the GNU General Public License (Version 2)
* <http://www.gnu.org/licenses/gpl.html>.
* There is NO WARRANTY, to the extent permitted by law.
*
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <usb.h>

#define USBDEV_SHARED_VENDOR	0x16C0  /* VOTI */
#define USBDEV_SHARED_PRODUCT	0x05DC  /* Obdev's free shared PID */

#define USBRQ_VERSION			0
#define USBRQ_RESET			1
#define USBRQ_ECHO			2
#define USBRQ_ENUMERATE			3
#define USBRQ_GET_STATUS		4
#define USBRQ_PREPARE_COLOR_PARAM	5
#define USBRQ_PREPARE_FADE_PARAM	6
#define USBRQ_EXEC_SET_COLOR		7
#define USBRQ_EXEC_FADETO_COLOR		8


static int usbGetStringAscii(usb_dev_handle *dev, int index, int langid, char *buf, int buflen) {
	char	buffer[256];
	int	 rval, i;

	if((rval = usb_control_msg(dev, USB_ENDPOINT_IN, USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) + index, langid, buffer, sizeof(buffer), 1000)) < 0)
		return rval;
	if(buffer[1] != USB_DT_STRING)
		return 0;
	if((unsigned char)buffer[0] < rval)
		rval = (unsigned char)buffer[0];
	rval /= 2;
	/* lossy conversion to ISO Latin1 */
	for(i=1;i<rval;i++){
		if(i > buflen) /* destination buffer overflow */
			break;
		buf[i-1] = buffer[2 * i];
		if(buffer[2 * i + 1] != 0) /* outside of ISO Latin1 range */
			buf[i-1] = '?';
	}
	buf[i-1] = 0;
	return i-1;
}



#define USB_ERROR_NOTFOUND	1
#define USB_ERROR_ACCESS	2
#define USB_ERROR_IO		3

static int usbOpenDevice(usb_dev_handle **device, int vendor, char *vendorName, int product, char *productName) {
	struct usb_bus *bus;
	struct usb_device *dev;
	usb_dev_handle *handle = NULL;
	int errorCode = USB_ERROR_NOTFOUND;
	static int didUsbInit = 0;

	if(!didUsbInit){
		didUsbInit = 1;
		usb_init();
	}
	usb_find_busses();
	usb_find_devices();
	for(bus=usb_get_busses(); bus; bus=bus->next){
		for(dev=bus->devices; dev; dev=dev->next){
			if(dev->descriptor.idVendor == vendor && dev->descriptor.idProduct == product){
				char	string[256];
				int	 len;
				handle = usb_open(dev); /* we need to open the device in order to query strings */
				if(!handle){
					errorCode = USB_ERROR_ACCESS;
					fprintf(stderr, "Warning: cannot open USB device: %s\n", usb_strerror());
					continue;
				}
				if(vendorName == NULL && productName == NULL){  /* name does not matter */
					break;
				}
				/* now check whether the names match: */
				len = usbGetStringAscii(handle, dev->descriptor.iManufacturer, 0x0409, string, sizeof(string));
				if(len < 0){
					errorCode = USB_ERROR_IO;
					fprintf(stderr, "Warning: cannot query manufacturer for device: %s\n", usb_strerror());
				}else{
					errorCode = USB_ERROR_NOTFOUND;
					/* fprintf(stderr, "seen device from vendor ->%s<-\n", string); */
					if(strcmp(string, vendorName) == 0){
						len = usbGetStringAscii(handle, dev->descriptor.iProduct, 0x0409, string, sizeof(string));
						if(len < 0){
							errorCode = USB_ERROR_IO;
							fprintf(stderr, "Warning: cannot query product for device: %s\n", usb_strerror());
						}else{
							errorCode = USB_ERROR_NOTFOUND;
							/* fprintf(stderr, "seen product ->%s<-\n", string); */
							if(strcmp(string, productName) == 0)
								break;
						}
					}
				}
				usb_close(handle);
				handle = NULL;
			}
		}
		if(handle)
			break;
	}
	if(handle != NULL){
		errorCode = 0;
		*device = handle;
	}
	return errorCode;
}

static void usage(char *name) {
	printf("-l --list        list compatible USB devices\n");
	printf("-d --device      USB device\n");
	printf("-b --bus         busid (local=0, rs485=1, ir=2)\n");
	printf("-r --reset       reset device\n");
	printf("-e --enumerate   enumerate bus connected to device\n");
	printf("-v --version     device version\n");
	printf("-t --test        USB device echotest\n");
	printf("-s --status      display status\n");
	printf("-i --id          device to use (0 is local (default)\n");
	printf("-c --color       color to set (R,G,B,W)\n");
	printf("-f --fade-color  color to fade to (R,G,B,W,dSecs)\n");
}


int main(int argc, char **argv) {
	int command = 0;
	int c;
	opterr = 0;

	usb_dev_handle *handle = NULL;
	unsigned char buffer[8];
	int nBytes;

	unsigned int busid = 0;
	unsigned int deviceid = 0;

	uint32_t r;
	uint32_t g;
	uint32_t b;
	uint32_t w;
	uint32_t time;

	uint8_t R = 0;
	uint8_t G = 0;
	uint8_t B = 0;
	uint8_t W = 0;
	uint16_t Time = 360;

	uint16_t RG;
	uint16_t BW;

	static struct option long_options[] = {
		{"list",	no_argument, 0, 'l'},
		{"device",	required_argument, 0, 'd'},
		{"bus",		required_argument, 0, 'b'},
		{"reset",	no_argument, 0, 'r'},
		{"enumerate",	no_argument, 0, 'e'},
		{"version",	no_argument, 0, 'v'},
		{"test",	no_argument, 0, 't'},
		{"status",	no_argument, 0, 's'},
		{"id",		no_argument, 0, 'i'},
		{"color",	required_argument, 0, 'c'},
		{"fade-color",	required_argument, 0, 'f'},
		{"help",	no_argument, 0, 'h'},
		{0, 0, 0, 0}
		};

	while (1) {
		int option_index = 0;

		c = getopt_long(argc, argv, "ld:b:revtsic:f:h", long_options, &option_index);

		switch (c) {
		case 'l': // list
			command = 1;
			break;
		case 'd': // device
			break;
		case 'b': // bus
			break;
		case 'r': // reset
			command = 4;
			break;
		case 'e': // enumerate
			command = 5;
			break;
		case 'v': // version
			command = 6;
			break;
		case 't': // test
			command = 7;
			break;
		case 's': // status
			command = 8;
			break;
		case 'i': // id
			command = 9;
			break;
		case 'c': // color
			command = 10;
			sscanf (optarg,"(%u,%u,%u,%u)", &r, &g, &b, &w);
			if ((0 <= r) && (r < 256) && (0 <= g) && (g < 256) && (0 <= b) && (b < 256) && (0 <= w) && (w < 256)) {
				R = r;
				G = g;
				B = b;
				W = w;
			}

			break;
		case 'f': // fade-color
			command = 11;
			sscanf (optarg,"(%u,%u,%u,%u,%u)", &r, &g, &b, &w, &time);
			if ((0 <= r) && (r < 256) && (0 <= g) && (g < 256) && (0 <= b) && (b < 256) && (0 <= w) && (w < 256)) {
				R = r;
				G = g;
				B = b;
				W = w;
			}
			if ((0 <= time) &&( time < 65536)) {
				Time = time;
			}
			break;
		case 'h': // help
			usage(argv[0]);
			break;
		}
     
		if (c == -1)
			break;

	}

	usb_init();
	if (usbOpenDevice(&handle, USBDEV_SHARED_VENDOR, "Spida.net", USBDEV_SHARED_PRODUCT, "Fnordlicht") != 0) {
		fprintf(stderr, "Could not find USB device \"Fnordlicht\" with vid=0x%x pid=0x%x\n", USBDEV_SHARED_VENDOR, USBDEV_SHARED_PRODUCT);
		exit(1);
	}


	if (command == 1) { // list
	}
	if (command == 4) { //reset
		nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, USBRQ_RESET, 0x0, 0x0, (char *)buffer, sizeof(buffer), 1000);
		if (nBytes < 0)
			fprintf(stderr, "USB error: %s\n", usb_strerror());
	}
	if (command == 5) { // enumerate
		printf("not implemented\n");
	}
	if (command == 6) { // version
		nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, USBRQ_VERSION, 0x0, 0x0, (char *)buffer, sizeof(buffer), 1000);
		if (nBytes != 6)
			printf("%u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
	}
	if (command == 7) { //test
		int i, v, r;
		for (i=0;i<1000;i++) {
			v = rand() & 0xffff;
			nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, USBRQ_ECHO, v, 0, (char *)buffer, sizeof(buffer), 1000);
			if (nBytes < 2) {
				if(nBytes < 0)
					fprintf(stderr, "USB error: %s\n", usb_strerror());
				fprintf(stderr, "only %d bytes received in iteration %d\n", nBytes, i);
				fprintf(stderr, "value sent = 0x%x\n", v);
				exit(1);
			}
			r = buffer[0] | (buffer[1] << 8);
			if (r != v) {
				fprintf(stderr, "data error: received 0x%x instead of 0x%x in iteration %d\n", r, v, i);
				exit(1);
			}
		}
		printf("test succeeded\n");
	} 
	if (command == 8) { // status
		nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, USBRQ_GET_STATUS, 0x0, 0x0, (char *)buffer, sizeof(buffer), 1000);
		if (nBytes != 4)
			printf("%u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3]);
	}
	if (command == 9) { // id
	}
	if (command == 10) { // color
		RG = (R<<8)+G;
		BW = (B<<8)+W;
		nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, USBRQ_PREPARE_COLOR_PARAM, RG, BW, (char *)buffer, sizeof(buffer), 1000);
		nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, USBRQ_EXEC_SET_COLOR, busid, deviceid, (char *)buffer, sizeof(buffer), 1000);
	}
	if (command == 11) { // fade-color
		RG = (R<<8)+G;
		BW = (B<<8)+W;
		nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, USBRQ_PREPARE_COLOR_PARAM, RG, BW, (char *)buffer, sizeof(buffer), 1000);
		nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, USBRQ_PREPARE_FADE_PARAM, time, 0x0, (char *)buffer, sizeof(buffer), 1000);
		nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, USBRQ_EXEC_FADETO_COLOR, busid, deviceid, (char *)buffer, sizeof(buffer), 1000);
	}

	usb_close(handle);
	return 0;
}

