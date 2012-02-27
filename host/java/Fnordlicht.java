import ch.ntb.usb.*;

/**********************************************************************
* $Id: $
*
* written by Timo Boettcher
* fnordlicht (near) spida (.) net
*
* (C) 2008,2009 Timo Boettcher
* This is free software. You may redistribute copies of it under the
* terms of the GNU General Public License (Version 2)
* <http://www.gnu.org/licenses/gpl.html>.
* There is NO WARRANTY, to the extent permitted by law.
*
**********************************************************************/

public class Fnordlicht {
	public static final short USB_ID_VENDOR			= 0x16c0;
	public static final short USB_ID_PRODUCT		= 0x05dc;
	public static final String USB_MANUFACTURER		= "Spida.net";
	public static final String USB_PRODUCT			= "Fnordlicht";

	public static final short FUNC_VERSION                = 0;
	public static final short FUNC_RESET                  = 1;
	public static final short FUNC_ECHO                   = 2;
	public static final short FUNC_ENUMERATE              = 3;
	public static final short FUNC_GET_STATUS             = 4;
	public static final short FUNC_PREPARE_COLOR_PARAM    = 5;
	public static final short FUNC_PREPARE_FADE_PARAM     = 6;
	public static final short FUNC_EXEC_SET_COLOR         = 7;
	public static final short FUNC_EXEC_FADETO_COLOR      = 8;

	public static final int requesttype = ch.ntb.usb.USB.REQ_TYPE_TYPE_VENDOR | ch.ntb.usb.USB.REQ_TYPE_RECIP_INTERFACE | ch.ntb.usb.USB.REQ_TYPE_DIR_DEVICE_TO_HOST;


	public static void main(String[] args) {
		LibusbJava.usb_init();
		LibusbJava.usb_find_busses();
		LibusbJava.usb_find_devices();


		short usbVendorId = USB_ID_VENDOR;
		short usbProductId = USB_ID_PRODUCT;
		Device device = null;
		Usb_Device usbdevice = null;
		long devicehandle = 0;
		String devicefilename = null;

		for (Usb_Bus bus = LibusbJava.usb_get_busses(); bus != null; bus = bus.getNext()) {
			for (Usb_Device dev = bus.getDevices(); dev != null; dev = dev.getNext()) {
				Usb_Device_Descriptor devDesc = dev.getDescriptor();
				
				devicehandle = LibusbJava.usb_open(dev);
				if ((devDesc.getIdVendor() == usbVendorId) && (devDesc.getIdProduct() == usbProductId)) {
					System.out.println(Integer.toHexString(devDesc.getIdVendor()) + " " + Integer.toHexString(devDesc.getIdProduct()));

					System.out.println(LibusbJava.usb_get_string_simple(devicehandle, (int)devDesc.getIManufacturer()) + " " + LibusbJava.usb_get_string_simple(devicehandle, (int)devDesc.getIProduct()) + " " + LibusbJava.usb_get_string_simple(devicehandle, (int)devDesc.getISerialNumber()));
					if ((devDesc.getIdVendor() == usbVendorId) && (devDesc.getIdProduct() == usbProductId) && (LibusbJava.usb_get_string_simple(devicehandle, (int)devDesc.getIManufacturer()).equals(USB_MANUFACTURER)) && (LibusbJava.usb_get_string_simple(devicehandle, (int)devDesc.getIProduct()).equals(USB_PRODUCT))) {
						if (LibusbJava.usb_get_string_simple(devicehandle, (int)devDesc.getIProduct()).equals(USB_PRODUCT)) {
							System.out.println("FOUND");

							//devicefilename = dev.getFilename();
							//device = USB.getDevice(usbVendorId, usbProductId, devicefilename);

							System.out.println("Resetting");
							byte[] data = {(byte) 0, (byte) 0};
							int length = LibusbJava.usb_control_msg(devicehandle, requesttype, FUNC_RESET, 0, 0, data, data.length, 1000);
						}
					}
				}
				if (devicehandle != 0) {
					LibusbJava.usb_close(devicehandle);
				}
			}
		}
	}
}
