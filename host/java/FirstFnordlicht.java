import ch.ntb.usb.*;
import gnu.getopt.*;

public class FirstFnordlicht {
	public static final short USB_ID_VENDOR			= 0x16c0;
	public static final short USB_ID_PRODUCT		= 0x05dc;
	public static final String USB_MANUFACTURER		= "Spida.net";
	public static final String USB_PRODUCT			= "Fnordlicht";

	public static final short FUNC_VERSION			= 0;
	public static final short FUNC_RESET			= 1;
	public static final short FUNC_ECHO			= 2;
	public static final short FUNC_ENUMERATE		= 3;
	public static final short FUNC_GET_STATUS		= 4;
	public static final short FUNC_PREPARE_COLOR_PARAM	= 5;
	public static final short FUNC_PREPARE_FADE_PARAM	= 6;
	public static final short FUNC_EXEC_SET_COLOR		= 7;
	public static final short FUNC_EXEC_FADETO_COLOR	= 8;

	public static final int requesttype = ch.ntb.usb.USB.REQ_TYPE_TYPE_VENDOR | ch.ntb.usb.USB.REQ_TYPE_RECIP_INTERFACE | ch.ntb.usb.USB.REQ_TYPE_DIR_DEVICE_TO_HOST;


	public static void main(String[] argv) {
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
					if ((LibusbJava.usb_get_string_simple(devicehandle, (int)devDesc.getIManufacturer()).equals(USB_MANUFACTURER)) && (LibusbJava.usb_get_string_simple(devicehandle, (int)devDesc.getIProduct()).equals(USB_PRODUCT))) {
						if (LibusbJava.usb_get_string_simple(devicehandle, (int)devDesc.getIProduct()).equals(USB_PRODUCT)) {
							System.out.println("FOUND");


							//devicefilename = dev.getFilename();
							//device = USB.getDevice(usbVendorId, usbProductId, devicefilename);


							if (devicehandle != 0) {

								int c;
								String arg;
								LongOpt[] longopts = new LongOpt[3];

								longopts[0] = new LongOpt("help", LongOpt.NO_ARGUMENT, null, 'h');
								longopts[0] = new LongOpt("list", LongOpt.NO_ARGUMENT, null, 'l');
								longopts[1] = new LongOpt("device", LongOpt.REQUIRED_ARGUMENT, null, 'd'); 
								longopts[1] = new LongOpt("bus", LongOpt.REQUIRED_ARGUMENT, null, 'b'); 
								longopts[0] = new LongOpt("reset", LongOpt.NO_ARGUMENT, null, 'r');
								longopts[0] = new LongOpt("enumerate", LongOpt.NO_ARGUMENT, null, 'e');
								longopts[0] = new LongOpt("version", LongOpt.NO_ARGUMENT, null, 'v');
								longopts[0] = new LongOpt("test", LongOpt.NO_ARGUMENT, null, 't');
								longopts[0] = new LongOpt("status", LongOpt.NO_ARGUMENT, null, 's');
								longopts[0] = new LongOpt("id", LongOpt.REQUIRED_ARGUMENT, null, 'i');
								longopts[0] = new LongOpt("color", LongOpt.REQUIRED_ARGUMENT, null, 'c');
								longopts[0] = new LongOpt("fade-color", LongOpt.REQUIRED_ARGUMENT, null, 'f');

								Getopt g = new Getopt("FirstFnordlicht", argv, "hld:b:revtsi:c:f:", longopts);
								g.setOpterr(false);

								int R = 0;
								int G = 0;
								int B = 0;
								int W = 0;
								int time = 367;
								int RG = 0;
								int BW = 0;
								byte[] buffer = {(byte) 0, (byte) 0, (byte) 0, (byte) 0 , (byte) 0, (byte) 0};
								int result;
								int senda = 0;
								int sendb = 0;
								boolean longtest = false;
								int gota = 0;
								int gotb = 0;
								while ((c = g.getopt()) != -1)
									switch (c) {
										case 'l':
											break;
										case 'r':
											System.out.println("Resetting device at 0");
											result = LibusbJava.usb_control_msg(devicehandle, requesttype, FUNC_RESET, 0, 0, buffer, buffer.length, 1000);
											break;
										case 'e':
											break;
										case 'v':
											result = LibusbJava.usb_control_msg(devicehandle, requesttype, FUNC_VERSION, 0, 0, buffer, buffer.length, 1000);
											System.out.format("Fnordlicht at 0 has version (%d, %d, %d, %d, %d, %d)\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
											break;
										case 't':
											for (senda = 0; senda < 256; senda++) {
												if (longtest) {
													for (sendb = 0; sendb < 256; sendb++) {
												
														result = LibusbJava.usb_control_msg(devicehandle, requesttype, FUNC_ECHO, senda, sendb, buffer, buffer.length, 1000);
														if ((((int)buffer[0]&0xff) != senda) | (((int)buffer[1]&0xff) != sendb) | (((int)buffer[0]&0xff) != senda) | (((int)buffer[3]&0xff) != sendb)) {
															System.out.format("Sent: %d %d, Received %d %d %d %d\n", senda, sendb, buffer[0]&0xff, buffer[1]&0xff, buffer[2]&0xff, buffer[3]&0xff);
														}
													}
												} else {
													result = LibusbJava.usb_control_msg(devicehandle, requesttype, FUNC_ECHO, senda, senda, buffer, buffer.length, 1000);
													if ((((int)buffer[0]&0xff) != senda) | (((int)buffer[1]&0xff) != senda) | (((int)buffer[0]&0xff) != senda) | (((int)buffer[3]&0xff) != senda)) {
														System.out.format("Sent: %d, Received %d %d %d %d\n", senda, buffer[0]&0xff, buffer[1]&0xff, buffer[2]&0xff, buffer[3]&0xff);
													}
														
												}
											}
											break;
										case 's':
											result = LibusbJava.usb_control_msg(devicehandle, requesttype, FUNC_GET_STATUS, 0, 0, buffer, buffer.length, 1000);
											System.out.format("(%d, %d, %d, %d)\n" , buffer[0]&0xff, buffer[1]&0xff, buffer[2]&0xff, buffer[3]&0xff);
											break;
										case 'c':
											String carg = g.getOptarg();
											String []csplits = carg.split(",");
											
											if (csplits.length == 4) {
													R = Integer.parseInt(csplits[0]);
													G = Integer.parseInt(csplits[1]);
													B = Integer.parseInt(csplits[2]);
													W = Integer.parseInt(csplits[3]);

													if ((0 <= R) && (R < 256) && (0 <= G) && (G < 256) && (0 <= B) && (B < 256) && (0 <= W) && (W < 256)) {
															RG = (R<<8)+G;
															BW = (B<<8)+W;
															
															result = LibusbJava.usb_control_msg(devicehandle, requesttype, FUNC_PREPARE_COLOR_PARAM, RG, BW, buffer, buffer.length, 1000);
															result = LibusbJava.usb_control_msg(devicehandle, requesttype, FUNC_EXEC_SET_COLOR, 0, 0, buffer, buffer.length, 1000);
													} else {
															System.out.println("Colorvalues are supposed to be 0 <= x <= 255");
													}
											} else {
													System.out.format("Expected a set of 4 colors to set, but got %s values", csplits.length);
											}
											break;
										case 'f':
											String farg = g.getOptarg();
											String []fsplits = farg.split(",");
											
											if (fsplits.length == 5) {
													R = Integer.parseInt(fsplits[0]);
													G = Integer.parseInt(fsplits[1]);
													B = Integer.parseInt(fsplits[2]);
													W = Integer.parseInt(fsplits[3]);
													time = Integer.parseInt(fsplits[4]);

													if ((0 <= R) && (R < 256) && (0 <= G) && (G < 256) && (0 <= B) && (B < 256) && (0 <= W) && (W < 256)) {
															if ((0 <= time) && (time < 65536)) {

																	RG = (R<<8)+G;
																	BW = (B<<8)+W;
																
																	result = LibusbJava.usb_control_msg(devicehandle, requesttype, FUNC_PREPARE_COLOR_PARAM, RG, BW, buffer, buffer.length, 1000);
																	result = LibusbJava.usb_control_msg(devicehandle, requesttype, FUNC_PREPARE_FADE_PARAM, time, 0, buffer, buffer.length, 1000);
																	result = LibusbJava.usb_control_msg(devicehandle, requesttype, FUNC_EXEC_FADETO_COLOR, 0, 0, buffer, buffer.length, 1000);
															} else {
																	System.out.println("Time is supposed to be 0 <= x <= 65535");
															}
													} else {
															System.out.println("Colorvalues are supposed to be 0 <= x <= 255");
													}
											} else {
													System.out.format("Expected a set of 4 colors and a time to set, but got %s values", fsplits.length);
											}
											break;
										case 'h':
											System.out.println("");
											break;
										default:
											System.out.println("getopt() returned " + c);
											break;
									}
								for (int i = g.getOptind(); i < argv.length ; i++)
									System.out.println("Non option argv element: " + argv[i] + "\n");


								LibusbJava.usb_close(devicehandle);
							}
						}
					}
				}
			}
		}

	}
}

