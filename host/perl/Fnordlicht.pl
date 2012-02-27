#!/usr/bin/perl

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

use Getopt::Long;
use Device::USB;
use strict;
use warnings;

my $usb = Device::USB->new();

	use constant {
		USB_ID_VENDOR			=> 0x16c0,
		USB_ID_PRODUCT			=> 0x05dc,
		USB_MANUFACTURER		=> "Spida.net",
		USB_PRODUCT			=> "Fnordlicht",

		USBRQ_VERSION			=> 0,
		USBRQ_RESET			=> 1,
		USBRQ_ECHO			=> 2,
		USBRQ_ENUMERATE			=> 3,
		USBRQ_GET_STATUS		=> 4,
		USBRQ_PREPARE_COLOR_PARAM	=> 5,
		USBRQ_PREPARE_FADE_PARAM	=> 6,
		USBRQ_EXEC_SET_COLOR		=> 7,
		USBRQ_EXEC_FADETO_COLOR		=> 8,

	};



	my $list;
	my $deviceid = 0;
	my $busid = 0;
	my $reset;
	my $enumerate;
	my $version;
	my $test;
	my $status;
	my $color;
	my $fadecolor;
	GetOptions (
		"list"			=> \$list,
		"d|device=i"		=> \$deviceid,
		"b|bus=i"		=> \$busid,
		"r|reset"		=> \$reset,
		"e|enumrate"		=> \$enumerate,
		"v|version"		=> \$version,
		"t|test"		=> \$test,
		"s|status"		=> \$status,
		"c|color=s"		=> \$color,
		"f|fade-color=s"	=> \$fadecolor
		) or exit (1);

	my @devicelist = $usb->list_devices(USB_ID_VENDOR, USB_ID_PRODUCT);
	my @filtereddevicelist;
	my $dev;
	my $serial;
	my $manufacturer;
	my $product;
	foreach $dev (@devicelist) {
		if($dev->open()) {
			$serial = $dev->serial_number();
			$manufacturer = $dev->manufacturer();
			$product = $dev->product();
		} else {
			$serial = "";
			$manufacturer = "";
			$product = "";
		}
		if (($manufacturer eq USB_MANUFACTURER) && ($product eq USB_PRODUCT)) {
			push @filtereddevicelist, $dev
		}
	}
	$serial = "";
	my $filtereddevicelistlength = scalar @filtereddevicelist;
	if ($filtereddevicelistlength == 1) {
		$dev = $filtereddevicelist[0];
		printf "Found a Device (VendorId 0x%04x, ProductId 0x%04x, Serial %s)\n", $dev->idVendor(), $dev->idProduct(), $serial;
	} elsif ($filtereddevicelistlength > 1) {
		foreach $dev (@filtereddevicelist) {
			printf "Found a Device (VendorId 0x%04x, ProductId 0x%04x, Serial %s)\n", $dev->idVendor(), $dev->idProduct(), $serial;
		}
	} else {
		print "Device not found.\n";
		die();
	}

	$dev->open();
	$dev->set_configuration(0);
	$dev->claim_interface(0);



	my $requesttype = 193; #usb.TYPE_VENDOR | usb.RECIP_INTERFACE | usb.ENDPOINT_IN
	my $buffer;
	my $result;
		my $R = 17;
		my $G = 255;
		my $B = 17;
		my $W = 255;
		my $time = 367;

	if ($version) {
		$buffer="\0" x 6;
		$result = $dev->control_msg($requesttype, USBRQ_VERSION, $busid, $deviceid, $buffer, 6, 1000);
		printf ("Fnordlicht at 0 has version (%d, %d, %d, %d, %d, %d)\n", unpack('C*',$buffer));
	} elsif ($reset) {
		print "Resetting device at 0\n";
		$dev->control_msg($requesttype, USBRQ_RESET, 0, 0, $buffer, 8, 1000);
	} elsif ($test) {
		$buffer="\0" x 4;
		my @resultbuffer;
		for (my $senda=0; $senda < 256 ;$senda++) {
			$result = $dev->control_msg($requesttype, USBRQ_ECHO, $senda, $senda, $buffer, 4, 1000);
			@resultbuffer = unpack('C*',$buffer);
			if (($resultbuffer[0] != $senda) | ($resultbuffer[1] != $senda) | ($resultbuffer[2] != $senda) | ($resultbuffer[3] != $senda)) {
				printf("Sent: %d, Received %d %d %d %d\n", $senda, $resultbuffer[0], $resultbuffer[1], $resultbuffer[2], $resultbuffer[3]);
				die();
			}
		}
	} elsif ($status) {
		$buffer="\0" x 4;
		$result = $dev->control_msg($requesttype, USBRQ_GET_STATUS, $busid, $deviceid, $buffer, 4, 1000);
		printf ("(%d, %d, %d, %d)\n", unpack('C*',$buffer));
	} elsif ($color) {
		my @params = split(/,/, $color);
		my $params = scalar @params;
		if ($params == 4) {
			my $param;
			foreach $param (@params) {
				if (($param < 0) || ($param > 255)) {
					print "Invalid Color\n";
					die();
				}
			}	
			($R, $G, $B, $W) = @params;
			
			if ((0 <= $R) && ($R <256) && (0 <= $B) && ($B <256) && (0 <= $G) && ($G <256) && (0 <= $W) && ($W <256)) {
				my $RG = ($R<<8)+$G;
				my $BW = ($B<<8)+$W;
				$result = $dev->control_msg($requesttype, USBRQ_PREPARE_COLOR_PARAM, $RG, $BW, $buffer, 0, 1000);
				$result = $dev->control_msg($requesttype, USBRQ_EXEC_SET_COLOR, $busid, $deviceid, $buffer, 0, 1000);
			} else {
				print "Invalid Color\n";
				die();
			}
		} else {
			printf("Expected a set of 4 colors to set, but got %s values\n", $params);
			die();
		}
	} elsif ($fadecolor) {
		my @params = split(/,/, $fadecolor);
		my $params = scalar @params;
		if ($params == 5) {
			($R, $G, $B, $W, $time) = @params;

			if ((0 <= $R) && ($R <256) && (0 <= $B) && ($B <256) && (0 <= $G) && ($G <256) && (0 <= $W) && ($W <256)) {
				if ((0 <= $time) && ($time < 65536)) {
					my $RG = ($R<<8)+$G;
					my $BW = ($B<<8)+$W;
					$result = $dev->control_msg($requesttype, USBRQ_PREPARE_COLOR_PARAM, $RG, $BW, $buffer, 0, 1000);
					$result = $dev->control_msg($requesttype, USBRQ_PREPARE_FADE_PARAM, $time, 0, $buffer, 0, 1000);
					$result = $dev->control_msg($requesttype, USBRQ_EXEC_FADETO_COLOR, $busid, $deviceid, $buffer, 0, 1000);
				} else {
					print "Invalid Fadetime";
					die();
				}
			} else {
				print "Invalid Color";
				die();
			}
		} else {
			printf("Expected a set of 4 colors and a time to set, but got %s values\n", $params);
			die();
		}
	}

