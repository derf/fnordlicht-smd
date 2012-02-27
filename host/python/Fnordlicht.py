#!/usr/bin/env python
# coding=utf-8
#*********************************************************************
# $Id: $
#
# written by Timo Boettcher
# fnordlicht (near) spida (.) net
#
# (C) 2008,2009 Timo Boettcher
# This is free software. You may redistribute copies of it under the
# terms of the GNU General Public License (Version 2)
# <http://www.gnu.org/licenses/gpl.html>.
# There is NO WARRANTY, to the extent permitted by law.
#
#*********************************************************************

import sys
try:
	import usb
	from optparse import OptionParser
except ImportError, err:
	print "couldn't load module. %s"%(err)
	sys.exit(2)


class Fnordlicht:
	USB_ID_VENDOR	= 0x16c0
	USB_ID_PRODUCT	= 0x05dc
	USB_MANUFACTURER= "Spida.net"
	USB_PRODUCT	= "Fnordlicht"

	USBRQ_VERSION			= 0
	USBRQ_RESET			= 1
	USBRQ_ECHO			= 2
	USBRQ_ENUMERATE			= 3
	USBRQ_GET_STATUS		= 4
	USBRQ_PREPARE_COLOR_PARAM	= 5
	USBRQ_PREPARE_FADE_PARAM	= 6
	USBRQ_EXEC_SET_COLOR		= 7
	USBRQ_EXEC_FADETO_COLOR		= 8

	requesttype = usb.TYPE_VENDOR | usb.RECIP_INTERFACE | usb.ENDPOINT_IN

	@staticmethod
	def UsbList():
		found = []
		for bus in usb.busses():
			for dev in bus.devices:
				if dev.idVendor == Fnordlicht.USB_ID_VENDOR and dev.idProduct == Fnordlicht.USB_ID_PRODUCT:
					if ((dev.open().getString(dev.iManufacturer,64) == Fnordlicht.USB_MANUFACTURER) and (dev.open().getString(dev.iProduct,64) == Fnordlicht.USB_PRODUCT)):
						serial = dev.open().getString(dev.iSerialNumber,64)
						found.append((bus,dev,serial))
		if len(found) == 1:
			for (bus, dev, serial) in found:
				print "Found a Device (VendorId %s, ProductId %s, Serial %s) at bus %s"%(hex(dev.idVendor), hex(dev.idProduct), serial, bus.dirname)
		elif len(found) > 1:
			for number, (bus, dev, serial) in enumerate(found):
				print "Found Device %s (VendorId %s, ProductId %s, Serial %s) at bus %s"%(number, hex(dev.idVendor), hex(dev.idProduct), serial, bus.dirname)
		else:
			print "Couldn't find a Device (VendorId %s, ProductId %s)"%(hex(Fnordlicht.USB_ID_VENDOR), hex(Fnordlicht.USB_ID_PRODUCT))
			sys.exit(1)

	def __init__(self, usbVendorId=USB_ID_VENDOR, usbProductId=USB_ID_PRODUCT, usbManufacturer=USB_MANUFACTURER, usbProduct=USB_PRODUCT, deviceid=0):
		self.device = self.FindDevice(usbVendorId, usbProductId, usbManufacturer, usbProduct, deviceid)
		self.handle = self.OpenDevice(self.device)

	def FindDevice(self, usbVendorId, usbProductId, usbManufacturer, usbProduct, deviceid=0):
		found = []
		for bus in usb.busses():
			for dev in bus.devices:
				if dev.idVendor == usbVendorId and dev.idProduct == usbProductId:
					if ((dev.open().getString(dev.iManufacturer,64) == usbManufacturer) and (dev.open().getString(dev.iProduct,64) == usbProduct)):
						serial = dev.open().getString(dev.iSerialNumber,64)
						found.append((dev,serial))

		#FIXME make sure deviceid is in the allowed range
		return found[deviceid][0]

	def OpenDevice(self, device):
		configuration = device.configurations[0]
		interface = configuration.interfaces[0][0]

		handle = device.open()
		if not handle:
			print "Couldn't open the device"
			sys.exit(2)
		try:
			handle.detachKernelDriver(interface)
		except usb.USBError:
			# already detached
			pass
		try:
			handle.setConfiguration(configuration)
			handle.claimInterface(interface)
			handle.setAltInterface(interface)
		except usb.USBError, err:
			print err
			sys.exit(3)
		return handle


	def Version(self, busid=0, deviceid=0):
		buf = 0xff
		result = self.handle.controlMsg(self.requesttype, self.USBRQ_VERSION, buf, busid, deviceid)
		return result

	def Reset(self, busid=0, deviceid=0):
		buf = 0xff
		result = self.handle.controlMsg(self.requesttype, self.USBRQ_RESET, buf, busid, deviceid)

	def Enumerate(self, busid=0):
		buf = 0xff

		#result = self.handle.controlMsg(self.requesttype, self.USBRQ_ENUMERATE, buf, busid, 0)

	def EchoDevice(self, long = False):
		buf = 0xff
		for senda in range(0,256):
			if long:
				for sendb in range(0,256):
					returned = self.handle.controlMsg(self.requesttype, self.USBRQ_ECHO, buf, value=senda, index=sendb, timeout=1000)
					#print "Sent: %s %s, Received %s"%(senda, sendb, returned)
					if returned[0] != senda or returned[1] != sendb or returned[2] != senda or returned[3] != sendb or returned[4] != senda or returned[5] != sendb or returned[6] != senda or returned[7] != sendb:
						return False
			else:
				returned = self.handle.controlMsg(self.requesttype, self.USBRQ_ECHO, buf, value=senda, index=senda, timeout=1000)
				#print "Sent: %s, Received %s"%(senda, returned)
				if returned[0] != senda or returned[1] != senda or returned[2] != senda or returned[3] != senda or returned[4] != senda or returned[5] != senda or returned[6] != senda or returned[7] != senda:
					return False
		return True

	def Status(self, busid=0, deviceid=0):
		buf = 0xff
		returned = self.handle.controlMsg(self.requesttype, self.USBRQ_GET_STATUS, buf, busid, deviceid)
		return (returned[0], returned[1], returned[2], returned[3])

	def SetColor(self, R, G, B, W, busid=0, deviceid=0):
		if 0 <= R and R <256 and 0 <= B and B <256 and 0 <= G and G <256 and 0 <= W and W <256:
			buf = 0xff
			RG = (R<<8)+G
			BW = (B<<8)+W
			result = self.handle.controlMsg(self.requesttype, self.USBRQ_PREPARE_COLOR_PARAM, buf, RG, BW)
			result = self.handle.controlMsg(self.requesttype, self.USBRQ_EXEC_SET_COLOR, buf, busid, deviceid)
		else:
			print "Invalid Color"

	def FadeColor(self, R, G, B, W, time, busid=0, deviceid=0):
		if 0 <= R and R <256 and 0 <= B and B <256 and 0 <= G and G <256 and 0 <= W and W <256:
			if 0 <= time and time < 65536:
				buf = 0xff
				RG = (R<<8)+G
				BW = (B<<8)+W
				result = self.handle.controlMsg(self.requesttype, self.USBRQ_PREPARE_COLOR_PARAM, buf, RG, BW)
				result = self.handle.controlMsg(self.requesttype, self.USBRQ_PREPARE_FADE_PARAM, buf, time, 0)
				result = self.handle.controlMsg(self.requesttype, self.USBRQ_EXEC_FADETO_COLOR, buf, busid, deviceid)
			else:
				print "Invalid Fadetime"
		else:
			print "Invalid Color"



def main(argv):
	parser = OptionParser()
	parser.set_defaults(mode='list')

	parser.add_option('-l', '--list',       dest='mode',             action='store_const', const='list',      help='list compatible USB devices')
	parser.add_option('-d', '--device',     dest='device',           action='store',       type="int",        help='USB device', default=0)
	parser.add_option('-b', '--bus',        dest='busid',            action='store',       type="int",        help='busid (local=0, rs485=1, ir=2)', default=0)
	parser.add_option('-r', '--reset',      dest='mode',             action='store_const', const='reset',     help='reset device')
	parser.add_option('-e', '--enumerate',  dest='mode',             action='store_const', const='enumerate', help='enumerate bus connected to device')
	parser.add_option('-v', '--version',    dest='mode',             action='store_const', const='version'  , help='device version')
	parser.add_option('-t', '--test',       dest='mode',             action='store_const', const='test',      help='USB device echotest')
	parser.add_option('-s', '--status',     dest='mode',             action='store_const', const='status',    help='display status')
	parser.add_option('-i', '--id',         dest='deviceid',         action='store',       type="int",        help='device to use (0 is local (default))', default=0)
	parser.add_option('-c', '--color',      dest='setcolorstring',   action='store',       type="string",     help='color to set', default=None, metavar="(R,G,B,W)")
	parser.add_option('-f', '--fade-color', dest='fadecolorstring',  action='store',       type="string",     help='color to fade to, and time to use (in 1/500 s)', default=None, metavar="(R,G,B,W,ticks)")
	(options, args) = parser.parse_args()


	if options.mode=='list' and options.setcolorstring==None and options.fadecolorstring==None:
		Fnordlicht.UsbList()
	else:
		myFnordlicht = Fnordlicht(deviceid=options.device)
		if options.mode=='reset':
			print "Resetting device at %s"%options.busid
			myFnordlicht.Reset(options.busid)
		elif options.mode=='enumerate':
			print "not implemented"
		elif options.mode=='version':
			version =  myFnordlicht.Version(options.busid)
			print "Fnordlicht at %s has version %s"%(options.busid,version)
		elif options.mode=='test':
			print "Running EchoTest for device %s"%options.busid
			if myFnordlicht.EchoDevice():
				print "Successfull"
			else:
				print "Failed"

		elif options.mode=='status':
			print myFnordlicht.Status(options.busid)
		elif options.setcolorstring!=None:
			s = options.setcolorstring
			if s[0] == "(" and s[-1]== ")":
				params = s[1:-1].split(",")
			else:
				params = s.split(",")
			if len(params) == 4:
				for index,value in enumerate(params):
					try:
						params[index] = int(value)
					except:
						print "Colorvalues are supposed to be integer"
						sys.exit(3)
					if params[index] < 0 or params[index] > 255:
						print "Colorvalues are supposed to be 0 <= x <= 255"
						sys.exit(3)
			else:
				print "Expected a set of 4 colors to set, but got %s values"%len(params)
				sys.exit(3)
			(R, G, B, W) = params

			myFnordlicht.SetColor(R, G, B, W, options.busid)
		elif options.fadecolorstring!=None:
			s = options.fadecolorstring
			if s[0] == "(" and s[-1]== ")":
				params = s[1:-1].split(",")
			else:
				params = s.split(",")
			if len(params) == 5:
				for index,value in enumerate(params):
					try:
						params[index] = int(value)
					except:
						print "Colorvalues are supposed to be integer"
						sys.exit(3)
					if index <= 3:
						if params[index] < 0 or params[index] > 255:
							print "Colorvalues are supposed to be 0 <= x <= 255"
							sys.exit(3)
					else:
						if params[index] < 0 or params[index] > 65535:
							print "Time is supposed to be 0 <= x <= 65535"
							sys.exit(3)
			else:
				print "Expected a set of 4 colors and a time to set, but got %s values"%len(params)
				sys.exit(3)
			(R, G, B, W, time) = params

			myFnordlicht.FadeColor(R, G, B, W, time, options.busid)


if __name__ == "__main__":
	main(sys.argv[1:])
