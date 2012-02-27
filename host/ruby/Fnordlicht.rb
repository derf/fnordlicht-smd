#!/usr/bin/ruby -w

#*********************************************************************
# $Id: $
#
# written by Timo Boettcher
# usbnotify (near) spida (.) net
#
# (C) 2008,2009 Timo Boettcher
# This is free software. You may redistribute copies of it under the
# terms of the GNU General Public License (Version 2)
# <http://www.gnu.org/licenses/gpl.html>.
# There is NO WARRANTY, to the extent permitted by law.
#
#*********************************************************************

require 'rubygems'
gem 'ruby-usb'
require 'usb'
require 'optparse'

class Fnordlicht
	USB_ID_VENDOR			= 0x16c0
	USB_ID_PRODUCT			= 0x05dc
	USB_MANUFACTURER		= "Spida.net"
	USB_PRODUCT			= "Fnordlicht

	USBRQ_VERSION			= 0
	USBRQ_RESET			= 1
	USBRQ_ECHO			= 2
	USBRQ_ENUMERATE			= 3
	USBRQ_GET_STATUS		= 4
	USBRQ_PREPARE_COLOR_PARAM	= 5
	USBRQ_PREPARE_FADE_PARAM	= 6
	USBRQ_EXEC_SET_COLOR		= 7
	USBRQ_EXEC_FADETO_COLOR		= 8
	
	REQUESTTYPE = USB::USB_TYPE_VENDOR | USB::USB_RECIP_INTERFACE | USB::USB_ENDPOINT_IN
	INTERFACE = 0
	TIMEOUT = 1000

	def Fnordlicht.list
		::USB.devices.each { |d|
			if d.idVendor == USB_ID_VENDOR and d.idProduct == USB_ID_PRODUCT and d.product == USB_PRODUCT and d.manufacturer == USB_MANUFACTURER
				puts "Found a Device (VendorId 0x%04x, ProductId 0x%04x, Serial %s)" % [d.idVendor, d.idProduct, d.serial_number]
			end
		} || raise("Couldn't find a Device (VendorId %s, ProductId %s)", USB_ID_VENDOR, USB_ID_PRODUCT)
	end

	def initialize
		device = finddevice()
		@handle = opendevice(device)
	end

	def finddevice
		device = ::USB.devices.select { |d|
			d.idVendor == USB_ID_VENDOR and d.idProduct == USB_ID_PRODUCT and d.product == USB_PRODUCT and d.manufacturer == USB_MANUFACTURER
		} || raise("Couldn't find a Device (VendorId %s, ProductId %s)", USB_ID_VENDOR, USB_ID_PRODUCT)

		return device.at(0)
	end

	def opendevice(device)
		handle = device.open
		raise "Unable to obtain a handle to the device." if handle.nil?

		retries = 0
		begin
			error_code = handle.usb_claim_interface(INTERFACE)
			raise unless error_code.nil?
		rescue
			handle.usb_detach_kernel_driver_np(INTERFACE);
			if retries.zero? 
				retries += 1
				retry
			else
				raise "Unable to claim the device interface."
			end
		end

		raise "Unable to set alternative interface." unless handle.set_altinterface(0).nil?
		return handle
	end

	def version(busid, deviceid)
		buffer = "\0" * 6
		ret = @handle.usb_control_msg(REQUESTTYPE, USBRQ_VERSION, busid, deviceid, buffer, TIMEOUT)
		return "Fnordlicht at 0 has version (%d, %d, %d, %d, %d, %d)" %buffer.unpack('C*')
	end

	def enumerate()
	end

	def reset()
		buffer = ""
		print "Resetting device at 0"
		ret = @handle.usb_control_msg(REQUESTTYPE, USBRQ_RESET, 0, 0, buffer, TIMEOUT)
	end

	def echodevice()
		buffer = "\0" * 4
		0.upto(255).each { |senda| 
			ret = @handle.usb_control_msg(REQUESTTYPE, USBRQ_ECHO, senda, senda, buffer, TIMEOUT)
			resultbuffer = buffer.unpack("C*")
			if resultbuffer[0] != senda or resultbuffer[1] != senda or resultbuffer[2] != senda or resultbuffer[3] != senda
				puts "Sent: %d, Received %d %d %d %d\n"% [senda, resultbuffer[0], resultbuffer[1], resultbuffer[2], resultbuffer[3]]
			end
		}
	end

	def status(busid, deviceid)
		buffer = "\0" * 4
		ret = @handle.usb_control_msg(REQUESTTYPE, USBRQ_GET_STATUS, busid, deviceid, buffer, TIMEOUT)
		return "(%d, %d, %d, %d)" %buffer.unpack('C*')
	end

	def setcolor(r, g, b, w, busid, deviceid)
		if 0 <= r and r <256 and 0 <= b and b <256 and 0 <= g and g <256 and 0 <= w and w <256
			buffer = ""
			rg = (r<<8)+g
			bw = (b<<8)+w
			ret = @handle.usb_control_msg(REQUESTTYPE, USBRQ_PREPARE_COLOR_PARAM, rg, bw, buffer, TIMEOUT)
			ret = @handle.usb_control_msg(REQUESTTYPE, USBRQ_EXEC_SET_COLOR, busid, deviceid, buffer, TIMEOUT)
		else
			raise "Invalid Color"
		end
	end

	def fadecolor(r, g, b, w, time, busid, deviceid)
		if 0 <= r and r <256 and 0 <= b and b <256 and 0 <= g and g <256 and 0 <= w and w <256
			if 0 <= time and time < 65536
				buffer = ""
				rg = (r<<8)+g
				bw = (b<<8)+w
				ret = @handle.usb_control_msg(REQUESTTYPE, USBRQ_PREPARE_COLOR_PARAM, rg, bw, buffer, TIMEOUT)
				ret = @handle.usb_control_msg(REQUESTTYPE, USBRQ_PREPARE_FADE_PARAM, time, 0, buffer, TIMEOUT)
				ret = @handle.usb_control_msg(REQUESTTYPE, USBRQ_EXEC_FADETO_COLOR, busid, deviceid, buffer, TIMEOUT)
			else
				raise "Invalid Fadetime"
			end
		else
			raise "Invalid Color"
		end
	end

end

options = {}
options[:deviceid]=0
options[:busid]=0

parser = OptionParser.new
parser.on('--list',                                      '-l',          'list compatible USB devices')         {options[:mode] = "list"}
parser.on('--device [NUM]',                              '-d', Integer, 'USB device')                          {|options[:deviceid]|}
parser.on('--bus [NUM]',                                 '-b', Integer, 'busid (local=0, rs485=1, ir=2)')      {|options[:busid]|}
parser.on('--reset',                                     '-r',          'reset device')                        {options[:mode] = "reset"}
parser.on('--enumerate',                                 '-e',          'enumerate bus connected to device')   {options[:mode] = "enumerate"}
parser.on('--version',                                   '-v',          'device version')                      {options[:mode] = "version"}
parser.on('--test',                                      '-t',          'USB device echotest')                 {options[:mode] = "test"}
parser.on('--status',                                    '-s',          'display status')                      {options[:mode] = "status"}
parser.on('--id [NUM]',                                  '-i', Integer, 'device to use (0 is local (default)') {|options[:id]|}
parser.on('--color [NUM],[NUM]},[NUM],[NUM]',            '-c', Array,   'color to set (R,G,B,W)')              {|options[:r], options[:g], options[:b], options[:w]| options[:mode]="color"}
parser.on('--fade-color [NUM],[NUM]},[NUM],[NUM],[NUM]', '-f', Array,   'color to fade to (R,G,B,W,dSecs)')    {|options[:r], options[:g], options[:b], options[:w], options[:time]| options[:mode]="fade"}
parser.on('--help',                                      '-h',          'Show this help message.')             { puts parser; exit }

parser.parse ARGV

if options[:mode] == "list"
	Fnordlicht.list()
else
	myFnordlicht = Fnordlicht.new()
	case options[:mode]
	when "reset"
		myFnordlicht.reset()
	when "enumerate"
		puts "not implemented"
	when "version"
		puts myFnordlicht.version(options[:deviceid], options[:busid])
	when "test"
		myFnordlicht.echodevice(options[:deviceid], options[:busid])
	when "status"
		puts myFnordlicht.status(0,0)
	when "color"
		r = options[:r].to_i()
		g = options[:g].to_i()
		b = options[:b].to_i()
		w = options[:w].to_i()
		myFnordlicht.setcolor(r, g, b, w, options[:deviceid], options[:busid])
	when "fade"
		r = options[:r].to_i()
		g = options[:g].to_i()
		b = options[:b].to_i()
		w = options[:w].to_i()
		time = options[:time].to_i()
		myFnordlicht.fadecolor(r, g, b, w, time, options[:deviceid], options[:busid])
	end
end

