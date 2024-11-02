#!/usr/bin/env python3.6
# -*- coding: utf-8 -*-
#
# Copyright 2008,2009 Free Software Foundation, Inc.
#
# This application is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# This application is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

import socket
import struct
import time

def bind_to_port_range(low, high):
    m_counter = int(low)
    m_high = int(high)
    m_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    while m_counter <= m_high:
        try:
            m_socket.bind(("", m_counter))
            return m_socket
        except:
            m_counter += 1
    return None

# ip address of IC-705 connected to WiFi
udp_ip = '192.168.2.19'
# standard udp port for this transsmision
udp_port_civ = 50002
udp_port_ctl = 50001

#get_freq_cmd = "FEFEACE003FD"
cmd_ack = b'\x01\x01\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00' # command acknowledge
rcv_ack = b'\x01\x02\x00\x00\x00\x02\x00\x00\x04\x00' # receive acknowledge   

# seried  to conenct to control port
connect_ctl = b'\x10\x00\x00\x00\x03\x00\x00\x00\x60\xf2\x00\x00\x00\x00\x00\x00'  # connect before logging in
connect_ctl_r_u_ready = b'\x10\x00\x00\x00\x06\x00\x01\x00\x60\xF2\x00\x00\x2A\x86\x1F\x2F'
ctl_login = b'\x80\x00\x00\x00\x00\x00\x01\x00\x60\xF2\x00\x00\x2A\x86\x1F\x2F\x00\x00\x00\x70\x01\x00\x00\x00\x00\x00\x5A\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
ctl_token = b'40 00 00 00 00 00 02 00 60 F2 00 00 2A 86 1F 2F 00 00 00 30 01 02 00 01 00 00 5A 01 51 EC C8 D2 00 00 00 00 00 00 00'

# series to conenct serial port
connect_udp_serial = b'\x10\x00\x00\x00\x03\x00\x00\x00\x87\xCA\x00\x00\x00\x00\x00\x00'  #Connect to serial from port 51847 to 50002 
udp_serial_ready = b'\x10\x00\x00\x00\x06\x00\x01\x00\x87\xCA\x00\x00\x28\xBC\x6D\xF5'
set_freq_cmd = b'\xFE\xFE\xAC\xE0\x05\x00\x00\x20\x44\x01\xFD' # command to seet radio dial frequency  00,144,478,000
#set_freq_cmd = 'FEFEACE0050000204401FD'   # 0144,200,000Hz  # 5 bytes for frequency < 6GHz.
set_10GHz_freq_cmd = 'FEFEACE005000010600301FD'  # 010,360,100,000 Hz  6 frequency bytes for 10Ghz and higher.
get_freq_cmd = b'\xFE\xFE\xAC\xE0\x03\xFD' # command for getting the dial frequency

sock_ctl = bind_to_port_range(62048,62048) 
if sock_ctl:
    sock_ctl.sendto(connect_ctl, (udp_ip, udp_port_ctl))
    buf, sender = sock_ctl.recvfrom(4096)
    print(buf)
    sock_ctl.sendto(connect_ctl_r_u_ready, (udp_ip, udp_port_ctl))
    buf, sender = sock_ctl.recvfrom(4096)
    print(buf)
    sock_ctl.sendto(ctl_login, (udp_ip, udp_port_ctl))
    buf, sender = sock_ctl.recvfrom(4096)
    print(buf)
    sock_ctl.sendto(ctl_login, (udp_ip, udp_port_ctl))
    buf, sender = sock_ctl.recvfrom(4096)
    print(buf)
        
sock_serial = bind_to_port_range(51847,51847)  
if sock_serial:
    sock_serial.sendto(connect_udp_serial, (udp_ip, udp_port_civ))
    buf, sender = sock_serial.recvfrom(4096)
    print(buf)
    sock_serial.sendto(udp_serial_ready, (udp_ip, udp_port_civ))
    buf, sender = sock_serial.recvfrom(4096)
    print(buf)
    sock_serial.sendto(set_freq_cmd, (udp_ip, udp_port_civ))
    buf, sender = sock_serial.recvfrom(4096)
    print(buf)
    
    
#t = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  #UDP
#t.bind(("", udp_port_ctl))
##t.settimeout(1.0)
#t.setblocking(0)
#print(socket.getnameinfo()[1])

#m2 = b'\xfe\xfe\xac\xe0\x00\x90\x79\x02\x28\x00\xfd'
#t.sendto(connect_udp_serial, (udp_ip, udp_port_ctl))
    
while True:
    time.sleep(1.0)
 
    try:
        # send the command
        #m = ("{}{}" .format(set_freq_cmd).encode().format(set2).encode())
        #m = set_freq_cmd 
        #m1 = bytes([254, 254, 172, 224, 3, 253]) 
        #m1 = bytes([254, 254, 172, 224, 5, 0, 128, 71, 68, 1, 0, 253]) 
        #t.sendto(m1, (udp_ip, udp_port_civ))
        #print("TX to CPU Msg = {}" .format(m1).encode)
        pass
    except:
        #print("TX to CPU FAILED Error = {}" .format(t).encode()) 
        pass
    # close the socket
    #t.close() 
        
  #  try:                
  #      buf, sender = sock_serial.recvfrom(4096)
        #if sender > 1000: print(sender)    # just gets rid of unused var error, does nothing for us here.
        #print("Received message before decode: {}" .format(buf))
  #      print(buf)
  #      s_data = buf    #.decode()
  #      try:
  #          s_data = buf.decode(encoding="utf-8", errors='ignore')
  #      except:
  #          s_data = buf.decode(encoding="ascii", errors='ignore')
        #if s_data[0] == ">":
        #    print("CMD received message echo: {}" .format(s_data))
        #else:
  #      print("received message:", s_data)
        #pd.get_power_data(pd, str(s_data))
  #  except UnicodeDecodeError: # catch error and ignore it
  #      print("Unicode decode error caught")  # will get this on CPU resets
  #  except socket.timeout:
        #pass
  #      print("Timeout on data received from UDP")
  #  except socket.error:
        #print("No data received from UDP")    
  #      pass  



#data = t.recv(22)
#answer = cmd_ack + rcv_ack

#if(data == cmd_ack):
#	data = t.recv(22)
#	if(data != rcv_ack):
#		print("No 'receive ack' received from radio")
#		exit()
#elif(data != answer):
#	if(data != cmd_ack):
#		print("No 'command ack' received from radio")
#		print(data)
#		exit()

#while True:
#	piece = f.read(1024)  
#	if not piece:
#		break
#	piece_size = len(piece).to_bytes(2, byteorder='big')
	#if(len(piece)==1024):
	#	to_send = send_next + piece_size + piece
	#	s.send(to_send)
    #	data = s.recv(10)
	#	while (len(data) == 0):
    #   	data = s.recv(10)
	#	if(data != rcv_ack):
	#		print("Error, 'receive ack' not received.")
	#		exit()
	#else:
	#	to_send = send_last + piece_size + piece
	#	s.send(to_send)
#f.close()
#s.close()