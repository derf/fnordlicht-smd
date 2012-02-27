#!/usr/bin/lua

--*********************************************************************
-- $Id: $
--
-- written by Timo Boettcher
-- fnordlicht (near) spida (.) net
--
-- (C) 2008,2009 Timo Boettcher
-- This is free software. You may redistribute copies of it under the
-- terms of the GNU General Public License (Version 2)
-- <http://www.gnu.org/licenses/gpl.html>.
-- There is NO WARRANTY, to the extent permitted by law.
--
--*********************************************************************

assert(loadlib("/usr/local/lib/lua/usb/libluausb.so","luaopen_libusb"))()

USB_ID_VENDOR	= "0x16c0"
USB_ID_PRODUCT	= "0x05dc"
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

requesttype = 193 --usb.TYPE_VENDOR | usb.RECIP_INTERFACE | usb.ENDPOINT_IN

function getopt(arg, options)
	local tab = {}
	for k, v in ipairs(arg) do
		if string.sub(v, 1, 2) == "--" then
			local x = string.find(v, "=", 1, true)
			if x then
				tab[string.sub(v, 3, x-1)] = string.sub(v, x+1)
			else 
				tab[string.sub(v, 3 )] = true
			end
		elseif string.sub(v, 1, 1) == "-" then
			local y = 2
			local l = string.len(v)
			local jopt
			while (y <= l) do
				jopt = string.sub(v, y, y)
				if string.find(options, jopt, 1, true) then
					if y < l then
						tab[jopt] = string.sub(v, y+1)
						y = l
					else
						tab[jopt] = arg[k + 1]
					end
				else
					tab[jopt] = true
				end
				y = y + 1
			end
		end
	end
	return tab
end

function UsbList()
	local buses=libusb.get_busses()

	local handle
	local manuf
	local prod
	local serial
		
	for dirname, bus in pairs(buses) do
		local devices=libusb.get_devices(bus)
		for filename, device in pairs(devices) do

			local descriptor=libusb.device_descriptor(device)
			-- lua 5.1 knows hexadecimal numbers
--			if descriptor.idVendor == USB_ID_VENDOR and descriptor.idProduct == USB_ID_PRODUCT then
			-- so for lua 5.0 use tonumber instead
			if descriptor.idVendor == tonumber(USB_ID_VENDOR) and descriptor.idProduct == tonumber(USB_ID_PRODUCT) then 
				handle = libusb.open(device)
				manuf = libusb.get_string_simple(handle, 1, 64)
				prod = libusb.get_string_simple(handle, 2, 64)
				serial = libusb.get_string_simple(handle, 3, 64)
				libusb.close(handle)
				if manuf == USB_MANUFACTURER and prod == USB_PRODUCT then 
					print (string.format("Found a Device (VendorId %04x, ProductId %04x, Serial %s) at bus %s",descriptor.idVendor, descriptor.idProduct, serial, dirname))
				end
			end
		end
	end
end

function FindDevice()
	local buses=libusb.get_busses()
	local found_device
		
	for dirname, bus in pairs(buses) do
		local devices=libusb.get_devices(bus)
		for filename, device in pairs(devices) do

			local descriptor=libusb.device_descriptor(device)
			local serial=0
			-- lua 5.1 knows hexadecimal numbers
--			if descriptor.idVendor == USB_ID_VENDOR and descriptor.idProduct == USB_ID_PRODUCT then
			-- so for lua 5.0 use tonumber instead
			if descriptor.idVendor == tonumber(USB_ID_VENDOR) and descriptor.idProduct == tonumber(USB_ID_PRODUCT) then 
				found_device = device
			end
		end
	end
	return found_device
end

function OpenDevice(device)
	local handle
	handle = libusb.open(device)
	return handle
end

function Version(handle, busid, deviceid)
	local buf = "123456"
	result = libusb.control_msg(handle, requesttype, USBRQ_VERSION, busid, deviceid, buf, 6)
	print(string.format("Fnordlicht at 0 has version %d,%d,%d,%d,%d,%d", string.byte(result,1), string.byte(result,2), string.byte(result,3), string.byte(result,4), string.byte(result,5), string.byte(result,6)))
end

function Reset(handle, busid, deviceid)
 	libusb.control_msg(handle, requesttype, USBRQ_RESET, busid, deviceid)
end

-- function Enumerate(busid=0)
-- end

-- function EchoDevice(handle)
-- end

function Status(handle, busid, deviceid)
	local buf = "1234"
	result = libusb.control_msg(handle, requesttype, USBRQ_GET_STATUS, busid, deviceid, buf, 4)
	print(string.format("(%d, %d, %d, %d)", string.byte(result,1), string.byte(result,2), string.byte(result,3), string.byte(result,4)))
end

function SetColor(handle, R, G, B, W, busid, deviceid)
	if 0 <= R and R <256 and 0 <= B and B <256 and 0 <= G and G <256 and 0 <= W and W <256 then
		RG = R * 256 + G
		BW = B * 256 + W
		libusb.control_msg(handle, requesttype, USBRQ_PREPARE_COLOR_PARAM, RG, BW)
		libusb.control_msg(handle, requesttype, USBRQ_EXEC_SET_COLOR, busid, deviceid)
	else
		print "Invalid Color"
	end
end

function FadeColor(handle, R, G, B, W, time, busid, deviceid)
	if 0 <= R and R <256 and 0 <= B and B <256 and 0 <= G and G <256 and 0 <= W and W <256 then
		if 0 <= time and time < 65536 then
			RG = R * 256 + G
			BW = B * 256 + W
			libusb.control_msg(handle, requesttype, USBRQ_PREPARE_COLOR_PARAM, RG, BW)
			libusb.control_msg(handle, requesttype, USBRQ_PREPARE_FADE_PARAM, time, 0)
			libusb.control_msg(handle, requesttype, USBRQ_EXEC_FADETO_COLOR, busid, deviceid)
		else
			print "Invalid Fadetime"
		end
	else
		print "Invalid Color"
	end
end

opts = getopt(arg, "lr")
for k, v in pairs(opts) do
	print(k, v)
end

mode = "s"

if mode == "l" then
	UsbList()
else
	local device
	local handle
	device = FindDevice()
	handle = OpenDevice(device)
	local busid = 0
	local deviceid = 0

	if mode == "r" then
		Reset(handle, busid, deviceid)
	elseif mode == "e" then
	elseif mode == "v" then
		Version(handle, busid, deviceid)
	elseif mode == "t" then
	elseif mode == "s" then
		Status(handle, busid, deviceid)
	elseif mode == "c" then
		R = 1
		B = 10
		G = 80
		W = 255
		SetColor(handle, R, G, B, W, busid, deviceid)
	elseif mode == "f" then
		R = 255
		B = 80
		G = 10
		W = 1
		time = 720
		FadeColor(handle, R, G, B, W, time, busid, deviceid)
	end

end

