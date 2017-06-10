#!/usr/bin/python

################################################################################
#
# A script to program a LPC800 devices via ISP serial interface
#
# Author: Axel Heider, axelheider(_at_)gmx.de
# Created:     2014-Mar-20
# Last Change: 2014-Jan-01
#
# License: Creative Commons, CC-BY-NC-SA 3.0/de
#          German:  http://creativecommons.org/licenses/by-nc-sa/3.0/de/
#          General: http://creativecommons.org/licenses/by-nc-sa/3.0/
#
#
# This script is based on the work of Lars Ole Belhage (belhage@midibel.com),
# who published it together with a minimal development enviroment for the
# LPC800.
#
################################################################################

import sys
import argparse
import serial
import time

#
# http://www.nxp.com/documents/user_manual/UM10601.pdf
#
# Memory map:
#   0x00000000   
#       Flash (4 Kb)
#   0x00001000
#       Flash (on devices with 8 Kb)
#   0x00002000
#       Flash (on devices with 16 Kb)
#   0x00004000
#       ...
#   0x10000000
#       SRAM (1 Kb)
#   0x10000400
#       SRAM (on devices with 2 Kb)
#   0x10000800
#       SRAM (on devices with 4 Kb)
#   0x10001000
#       ...
#   0x14000000
#       Micro Trace Buffer (1 Kb)
#   0x14000400
#       ...
#   0x1FFF0000
#       Boot ROM (8 Kb)
#   0x1FFF2000
#       ....
#
# Flash:
#   page size:    64 Byte
#   sector size:  1 kB (16 pages)
#
# Sector   Page      Address                  4 KB   8 KB  16 Kb
#    0     0 -  15   0x00000000 - 0x000003FF    x      x     x
#    1    16 -  31   0x00000400 - 0x000007FF    x      x     x
#    2    32 -  47   0x00000800 - 0x00000BFF    x      x     x
#    3    48 -  63   0x00000C00 - 0x00000FFF    x      x     x
#    4    64 -  79   0x00001000 - 0x000013FF    -      x     x
#    5    80 -  95   0x00001400 - 0x000017FF    -      x     x
#    6    96 - 111   0x00001800 - 0x00001BFF    -      x     x
#    7   112 - 127   0x00001C00 - 0x00001FFF    -      x     x
#    8   128 - 143   0x00002000 - 0x000023FF    -      -     x
#    9   144 - 159   0x00002400 - 0x000027FF    -      -     x
#   10   160 - 175   0x00002800 - 0x00002BFF    -      -     x
#   11   176 - 191   0x00002C00 - 0x00002FFF    -      -     x
#   12   192 - 207   0x00003000 - 0x000033FF    -      -     x
#   13   208 - 223   0x00003400 - 0x000037FF    -      -     x
#   14   224 - 239   0x00003800 - 0x00003BFF    -      -     x
#   15   240 - 255   0x00003C00 - 0x00003FFF    -      -     x
#
# 80 byte of SRAM from 0x10000000 to 0x10000050 are not used by the bootloader,
# thus content in this area is retained during reset. SRAM memory is not 
# retained for deep power-down mode for full power.
#

ADDR_CHECKSUM = 0x001C

ADDR_CRP      = 0x02fc
NO_ISP = 0x4E697370
CRP1   = 0x12345678
CRP2   = 0x87654321
CRP3   = 0x43218765



#-------------------------------------------------------------------------------
def printVerbose(level,string):
    if (args.verbose >= level):
        print(string)

#-------------------------------------------------------------------------------
def hexdump(data, indentStr="", printOffs=0, length=16, sep='.'):
	FILTER = ''.join([(len(repr(chr(x))) == 3) and chr(x) or sep for x in range(256)])
	lines = []
	for c in xrange(0, len(data), length):
		chars = data[c:c+length]
		hexStr = ' '.join(["%02x"%x for x in chars])
		if len(hexStr) > 24:
			hexStr = "%s %s"%(hexStr[:24], hexStr[24:])
		printable = ''.join(["%s"%((x<=127 and FILTER[x]) or sep) for x in chars])
		lines.append("%s%08x:  %-*s  | %s"%(indentStr,printOffs+c, length*3, hexStr, printable))
	print "\n".join(lines)

#-------------------------------------------------------------------------------
def get_uint32_le(data, offset):
    return data[offset] \
           | (data[offset+1] <<  8) \
           | (data[offset+2] << 16) \
           | (data[offset+3] << 24)


#-------------------------------------------------------------------------------
def set_uint32_le(data,offset,val):
    data[offset]   = (val >>  0) & 0xff
    data[offset+1] = (val >>  8) & 0xff
    data[offset+2] = (val >> 16) & 0xff
    data[offset+3] = (val >> 24) & 0xff

#-------------------------------------------------------------------------------
def get_uint16_le(data, offset):
    return data[offset] \
           | (data[offset+1] <<  8)

#-------------------------------------------------------------------------------
def set_uint16_le(data,offset,val):
    data[offset]   = (val >>  0) & 0xff
    data[offset+1] = (val >>  8) & 0xff

#-------------------------------------------------------------------------------
def append_uint16_le(data,val):
    data.append( (val >>  0) & 0xff );
    data.append( (val >>  8) & 0xff );

#-------------------------------------------------------------------------------
def append_uint32_le(data,val):
    data.append( (val >>  0) & 0xff );
    data.append( (val >>  8) & 0xff );
    data.append( (val >> 16) & 0xff );
    data.append( (val >> 24) & 0xff );


#-------------------------------------------------------------------------------
def sendSer(data):
    printVerbose(2,"sendRaw: <"+data+">")
    ser.write(data)
    ser.flush()

#-------------------------------------------------------------------------------
def sendSerCrLf(data):
    printVerbose(2,"sendCrLf: <"+data+"> CR LF")
    ser.write(data+"\r\n")
    ser.flush()

#-------------------------------------------------------------------------------
def readSer():
    ret = ser.readline().rstrip()
    printVerbose(2,"read: <"+ret+">")
    return ret

#-------------------------------------------------------------------------------
CMD_SUCCESS              =  0 
ERR_INVALID_CMD          =  1
ERR_SRC_ADDR             =  2 # not on word boundary.
ERR_DST_ADDR             =  3 # not on a correct boundary.
ERR_SRC_ADDR_NOT_MAPPED  =  4 # not mapped in memory map. Count value is taken in to consideration where applicable.
ERR_DST_ADDR_NOT_MAPPED  =  5 # not mapped in memory map. Count value is taken in to consideration where applicable.
ERR_COUNT                =  6 # Byte count is not multiple of 4 or is not a permitted value.
ERR_INVALID_SECTOR       =  7 # Sector number is invalid or end sector number is greater than start sector number.
ERR_SECTOR_NOT_BLANK     =  8
ERR_SECTOR_NOT_PREPARED  =  9 # Command to prepare sector for write operation was not executed.
ERR_COMPARE              = 10
ERR_BUSY                 = 11 # Flash programming hardware interface is busy.
ERR_PARAM                = 12
ERR_ADDR                 = 13 # not on word boundary
ERR_ADDR_NOT_MAPPED      = 14 # Address is not mapped in the memory map. Count value is taken in to consideration where applicable.
ERR_CMD_LOCKED           = 15
ERR_INVALID_CODE         = 16
ERR_INVALID_BAUD_RATE    = 17
ERR_INVALID_STOP_BIT     = 18
ERR_CRP_ENABLED          = 19


#-------------------------------------------------------------------------------
def cmdErr2Str(cmdErr):
    return { CMD_SUCCESS: "SUCCESS",
             ERR_INVALID_CMD: "ERR_INVALID_CMD",
             ERR_SRC_ADDR: "ERR_SRC_ADDR",
             ERR_DST_ADDR: "ERR_DST_ADDR",
             ERR_SRC_ADDR_NOT_MAPPED: "ERR_SRC_ADDR_NOT_MAPPED",
             ERR_DST_ADDR_NOT_MAPPED: "ERR_DST_ADDR_NOT_MAPPED",
             ERR_COUNT: "ERR_COUNT",
             ERR_INVALID_SECTOR: "ERR_INVALID_SECTOR",
             ERR_SECTOR_NOT_BLANK: "ERR_SECTOR_NOT_BLANK",
             ERR_SECTOR_NOT_PREPARED: "ERR_SECTOR_NOT_PREPARED",
             ERR_COMPARE: "ERR_COMPARE",
             ERR_BUSY: "ERR_BUSY",
             ERR_PARAM: "ERR_PARAM",
             ERR_ADDR: "ERR_ADDR",
             ERR_ADDR_NOT_MAPPED: "ERR_ADDR_NOT_MAPPED",
             ERR_CMD_LOCKED: "ERR_CMD_LOCKED",
             ERR_INVALID_CODE: "ERR_INVALID_CODE",
             ERR_INVALID_BAUD_RATE: "ERR_INVALID_BAUD_RATE",
             ERR_INVALID_STOP_BIT: "ERR_INVALID_STOP_BIT",
             ERR_CRP_ENABLED: "ERR_CRP_ENABLED",
            }.get(cmdErr, "ERR_%d_ ???"%(cmdErr))

#-------------------------------------------------------------------------------
def sendCmd(cmd, dataResp=0):

    sendSerCrLf(cmd)
    if (echo != 0):
        cmdEcho = readSer()
        printVerbose(2,"ECHO: <"+cmdEcho+">")

    respCode = 0
    if cmd.startswith("G "):
         while (1):
             c = ser.read()
             if (c >= '0') and (c <= '9'):
                respCode = 10*respCode + (ord(c)-ord('0'))
             else:
                print("ERROR: response for G command broken")
                quit()
    else: 
        respStr = readSer()
        respCode = int(respStr)

    if (respCode != 0):
        print("ERROR: command <%s> failed, ret=%s (%s)"%(cmd,respCode,cmdErr2Str(respCode)))

    return respCode

#-------------------------------------------------------------------------------
def Cmd_A(val):
    printVerbose(1,"Set Echo to %d"%(val))
    cmdStr = "A %d"%(val)
    cmdRet = sendCmd(cmdStr)
    if (cmdRet != CMD_SUCCESS):
        quit()

    global echo
    echo = val

#-------------------------------------------------------------------------------
def Cmd_C(addrFlash, addrRam, numBytes):
    printVerbose(1,"Copy %d byte from 0x%08x in RAM to 0x%08x in flash"%(numBytes,addrRam,addrFlash))
    cmdStr = "C %d %d %d"%(addrFlash,addrRam,numBytes)
    cmdRet = sendCmd(cmdStr)
    if (cmdRet != CMD_SUCCESS):
        quit()

#-------------------------------------------------------------------------------
def Cmd_E(startSector,endSector):
    printVerbose(1,"Erasing sector %d - %d"%(startSector,endSector))
    cmdStr = "E %d %d"%(startSector,endSector)
    cmdRet = sendCmd(cmdStr)
    if (cmdRet != CMD_SUCCESS):
        quit()

#-------------------------------------------------------------------------------
def Cmd_G(addr):
    # execute a program residing in RAM or flash memory. Sddress must 
    # 0x00000200 or  greater. "T" is thumb mode
    printVerbose(1,"Executing code at 0x%08x"%(addr))
    cmdStr = "G %d T"%(addr)
    cmdRet = sendCmd(cmdStr)

#-------------------------------------------------------------------------------
def Cmd_J():
    cmdRet = sendCmd("J")
    if (cmdRet != CMD_SUCCESS):
        quit()
    partID = int(readSer())
    return partID 


#-------------------------------------------------------------------------------
def Cmd_K():
    cmdRet = sendCmd("K")
    if (cmdRet != CMD_SUCCESS):
        quit()
    bootRomMinor = int(readSer())
    bootRomMajor = int(readSer())

    return (bootRomMajor, bootRomMinor)

#-------------------------------------------------------------------------------
def Cmd_M(addr1,addr2,numBytes):
    printVerbose(1,"Compare %d bytes at 0x%08x and 0x%08x"%(numBytes,addr1,addr2))
    cmdStr = "M %d %d %d"%(addr1,addr2,numBytes)
    cmdRet = sendCmd(cmdStr)
    return cmdRet
 
#-------------------------------------------------------------------------------
def Cmd_N():
    cmdRet = sendCmd("N")
    if (cmdRet != CMD_SUCCESS):
        quit()
    uid1 = int(readSer())
    uid2 = int(readSer())
    uid3 = int(readSer())
    uid4 = int(readSer())
    return (uid1,uid2,uid3,uid4)

#-------------------------------------------------------------------------------
def Cmd_P(startSector,endSector):
    printVerbose(1,"Prepare sector %d - %d for write operation"%(startSector,endSector))
    cmdStr = "P %d %d"%(startSector,endSector)
    cmdRet = sendCmd(cmdStr)
    if (cmdRet != CMD_SUCCESS):
        quit()

#-------------------------------------------------------------------------------
def Cmd_R(addr,numBytes):
    printVerbose(1,"Read %d bytes from 0x%08x"%(numBytes,addr))
    cmdStr = "R %d %d"%(addr,numBytes)
    cmdRet = sendCmd(cmdStr,numBytes)
    if (cmdRet != CMD_SUCCESS):
        quit()

    data = []
    while (numBytes != 0):
        c = ord(ser.read())
        data.append(c)
        numBytes -= 1
    
    if (args.verbose >= 2):
        hexdump(data)

    return data

#-------------------------------------------------------------------------------
def Cmd_S(ramAddr,numBytes):
    printVerbose(1,"Read CRC checksum of block at 0x%08x len &d"%(ramAddr,numBytes))
    cmdStr = "S %d %d"%(ramAddr,numBytes)
    cmdRet = sendCmd(cmdStr)
    if (cmdRet != CMD_SUCCESS):
        quit()
    crc = int(readSer())
    return crc

#-------------------------------------------------------------------------------
def Cmd_U(val):
    printVerbose(1,"unlocking with code %d"%(val))
    cmdStr = "U %d"%(val)
    cmdRet = sendCmd(cmdStr)
    if (cmdRet != CMD_SUCCESS):
        quit()

#-------------------------------------------------------------------------------
def Cmd_W(addr,data):
    numBytes = len(data)
    printVerbose(1,"Write %d byte to RAM at 0x%08x"%(numBytes,addr))

    if (args.verbose >= 2):
        hexdump(data)

    cmdStr = "W %d %d"%(addr,numBytes)
    cmdRet = sendCmd(cmdStr)
    if (cmdRet != CMD_SUCCESS):
        quit()

    ser.write(data)

#-------------------------------------------------------------------------------
def getPartName(partID):
    return { 0x00008100: "LPC810M021FN8",
    		 0x00008241: "LPC824M201JHI33", 
			 0x00008221: "LPC822M101JHI33",
			 0x00008242: "LPC824M201JDH20",
			 0x00008222: "LPC822M101JDH20",
    #          0x00008110: "LPC811M001JDH16",
    #          0x00008120: "LPC812M101JDH16",
    #          0x00008121: "LPC812M101JD20",
    #          0x00008122: "LPC812M101JDH20"
            }.get(partID, "")

#-------------------------------------------------------------------------------
def unlock():
    UNLOCK_MAGIC = 23130 # magic value
    Cmd_U(UNLOCK_MAGIC) 


#-------------------------------------------------------------------------------
def printInfo():

    partID = Cmd_J()
    (uid1,uid2,uid3,uid4) = Cmd_N()
    (bootRomMajor, bootRomMinor) = Cmd_K()

    partName = getPartName(partID);
    partNameStr = partName 
    if (partNameStr == ""):
        partNameStr = "unknown"

    print("Device:")
    print("  PartID: 0x%08x (%s)"%(partID,partNameStr))
    print("  UID: %08x-%08x-%08x-%08x"%(uid1,uid2,uid3,uid4))
    print("  Boot Loder: V%d.%d"%(bootRomMajor,bootRomMinor))


#-------------------------------------------------------------------------------
def checkIfSupportedCotroller():
    partID = Cmd_J()
    partName = getPartName(partID);
    if (partName == ""):
        printInfo()
        quit()



#-------------------------------------------------------------------------------
def init():
    # serial setting: 8 data bits, 1 stop bit, no parity.
    # Device detects baud rate automatically when host sends "?" (0x3F) as the
    # first character. ASCII string "Synchronized"+CR+LF is send back then with
    # the detected baud rate. Host must send the same "Synchronized"+CR+LF 
    # back, so device can verify the baud rate. If synchronization is verified
    # then device sends "OK"+CR+LF. 
    # Then host must send the crystal frequency (in kHz) at which the part is
    # running. The response is required for backward compatibility of the boot
    # loader code. On the LPC800 it is ignored, boot loader configures 12 MHz 
    # IRC frequency. 
    # Now ISP command handler is invoked. For safety reasons an "Unlock" 
    # command is required before executing the commands resulting in flash
    # erase/write operations and the "Go" command.  The rest of the commands
    # can be executed without the unlock command. The Unlock command is 
    # required to be executed once per ISP session.

    isEcho = 1
    syncStr = "Synchronized"

    ser.flushInput()
    ser.flushOutput()

    ser.timeout = 2
    sendSer("?")
    j = readSer()
    ser.timeout = 1
    if j == syncStr:
        print("LPC8xx ISP setup")
        sendSerCrLf(syncStr)
        readSer() # cmd echo
        readSer() # "OK"
        sendSerCrLf("12") # backwards compatibility dummy 
        readSer() #  cmd echo
        readSer() # "OK"
    else:
        sendSerCrLf("")
        c = readSer()
        if (j != "?"):
            # already in ISP more with echo off?
            isEcho = 0
        else:
            # already in ISP more with echo on?
            if (c != ""):
                print("LPC8xx ISP mode error 1, read: <"+c+">")
                quit()
            c = readSer()

        if (c != "1"):
            print("LPC8xx ISP mode error 2, read: <"+c+">")
            quit()
        
    global echo
    echo = isEcho

    # try echo off command 
    Cmd_A(0) 

    print("LPC8xx ISP ready")


#-------------------------------------------------------------------------------
def calcChecksum(data):

    # The reserved Cortex-M0+ exception vector (offset 0x1C) contains the 
    # 2-complement of the check-sum of vector table entries 0 through 6. This 
    # causes the checksum of the first 8 table entries to be 0. The bootloader
    # code checksums the first 8 locations in sector 0 of the flash. If the
    # result is 0, then execution control is transferred to the user code.

    dlen = len(data)
    if (dlen < ADDR_CHECKSUM+4):
        print("Error: not enough data to insert the checksum")
        quit()

    sig = 0
    for v in range(0, 7):
	    sig += get_uint32_le(data, 4*v)

    sig ^= 0xffffffff
    sig += 1

    print("setting checksum: 0x%08x"%(sig))

    set_uint32_le(data,ADDR_CHECKSUM,sig)


#-------------------------------------------------------------------------------
def checkCrp(data):

    # Code Read Protection (CRP)
    # any CRP change becomes effective in the next power cycle.
    #
    # CRP   User    ISP entry    SWD      Enters     partial flash 
    #       Code    pin at       enabled  ISP mode   update in 
    #       Valid   reset                            ISP mode
    # 
    # None  No      x            Yes      Yes        Yes
    # None  Yes     High         Yes      No         NA
    # None  Yes     Low          Yes      Yes        Yes
    # CRP1  Yes     High         No       No         NA
    # CRP1  Yes     Low          No       Yes        Yes
    # CRP2  Yes     High         No       No         NA
    # CRP2  Yes     Low          No       Yes        No
    # CRP3  Yes     x            No       No         NA
    # CRP1  No      x            No       Yes        Yes
    # CRP2  No      x            No       Yes        No
    # CRP3  No      x            No       Yes        No


    dlen = len(data)
    if (dlen >= ADDR_CRP+4):
        crp = get_uint32_le(data,ADDR_CRP)
        if (crp in [NO_ISP, CRP1, CRP2, CRP3]):
            print("Error: found CRP-Magic word 0x%08x"%(magic))
            quit()


#-------------------------------------------------------------------------------
def flash(image):

    data = bytearray(image.read())
    image.close()

    dlen = len(data)
    if (0 == dlen):
        print("no data found")
        quit()

    sectEnd = (dlen-1)/1024
    pageCnt = (dlen + 63) / 64
    rlen = pageCnt * 64

    data += bytearray(rlen-dlen)  # add padding 

    checkCrp(data)
    calcChecksum(data) 

    print("flashing %d byte (%d pages)"%(dlen, pageCnt))

    unlock()
    Cmd_P(0,sectEnd)
    Cmd_E(0,sectEnd)

    tmpRamAddr = 0x10000000
    dataOffset = 0
    flashAddr = 0
    blockLen = 64

    while (dataOffset < dlen):
        if (args.verbose == 0):
           sys.stdout.write(".")
           sys.stdout.flush()
        else:
          printVerbose(1,"Write block: 0x%08x"%(flashAddr))

        Cmd_P(0,sectEnd)

        Cmd_W(tmpRamAddr,data[dataOffset:dataOffset+blockLen])
        Cmd_R(tmpRamAddr,blockLen)

        Cmd_C(flashAddr,tmpRamAddr,blockLen)
        Cmd_R(flashAddr,blockLen)

        flashAddr += blockLen
        dataOffset += blockLen

    if (args.verbose == 0):
       print("done")

    print("%d bytes were written"%(dlen))

#-------------------------------------------------------------------------------
def systemReset():

    print("resetting device...")

    # issue system reset as described in in LPC800 user manual section 22.5.1.8
    #   AIRCR_VECTKEY     = (0x05FA << 8)  // magic number
    #   AIRCR_SYSRESETREQ = (1U << 2)      // request reset
    #   LPC8xx_SCS->AIRCR = (AIRCR_VECTKEY << 8) | AIRCR_SYSRESETREQ;

    data = []
    append_uint16_le(data,0x4a01)     # 00: ldr r0, [pc, #4]   ; r0 = [08]
    append_uint16_le(data,0x4b02)     # 02: ldr r1, [pc, #8]   ; r0 = [0c]
    append_uint16_le(data,0x601a)     # 04: str r0, [r1]       ; write
    append_uint16_le(data,0xe7fe)     # 06: b   .              ; dummy failsafe loop
    append_uint32_le(data,0xe000ed0c) # 08: .word	0x05fa0004 ; (VECTKEY << 8) | SYSRESETREQ
    append_uint32_le(data,0x05fa0004) # 0c: .word	0xe000ed0c ; &(LPC8xx_SCS->AIRCR)

    Cmd_W(0x10000000, bytearray(data))
    unlock();
    Cmd_G(0x10000000)

#-------------------------------------------------------------------------------
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("-p", dest="port", default="/dev/ttyUSB0", help="serial/USB port, default is /dev/ttyUSB0")
    parser.add_argument('--verbose', '-v', default=0, action='count', help="verbosity level, -v or -vv or -vvv ...")
    parser.add_argument("--reset", '-r',  action='store_true', help="reset target")
    parser.add_argument("--dump",  action='store_true', help="dump Flash and RAM")
    parser.add_argument("image", nargs="?", type=argparse.FileType('rb'), help="image to flash")

    args = parser.parse_args()
    if (args.verbose > 0):
        print(args)
    
    try:
        ser = serial.Serial(args.port, 
                            baudrate=115200, bytesize=8, parity='N', stopbits=1, 
                            timeout=10, xonxoff=0, rtscts=0)
    except serial.serialutil.SerialException as e:
        print("Error: %s"%e)
        quit()
    except:
        print("Error: %s"%sys.exc_info()[0])
        quit()

    serial.dtr = False
    time.sleep(0.2)
    serial.dtr = True
    init()
    checkIfSupportedCotroller()
    printInfo()

    if (args.dump):
        print("RAM dump (first 0x80 survive reset):")
        data = Cmd_R(0x10000000, 0x400)
        hexdump(data,"  ")
        print("Flash dump:")
        data = Cmd_R(0x00000000, 0x1000)
        print("  Image checksum at 0x%08x: 0x%08x"%(ADDR_CHECKSUM,get_uint32_le(data,ADDR_CHECKSUM)))
        print("  CRP            at 0x%08x: 0x%08x"%(ADDR_CRP,get_uint32_le(data,ADDR_CRP)))
        hexdump(data,"  ")
    #else:
    #    print("RAM dump of contents surviving a reset:")
    #    data = Cmd_R(0x10000000, 80)
    #    hexdump(data,"  ")

    if (args.image):
        flash(args.image)

    if (args.reset):
        systemReset()
