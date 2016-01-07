# Copyright 2010-2013 Intel Corporation.
#
# This library is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, version 2.1.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# Disclaimer: The codes contained in these modules may be specific
# to the Intel Software Development Platform codenamed Knights Ferry,
# and the Intel product codenamed Knights Corner, and are not backward
# compatible with other Intel products. Additionally, Intel will NOT
# support the codes or instruction set in future products.
#
# Intel offers no warranty of any kind regarding the code. This code is
# licensed on an "AS IS" basis and Intel is not obligated to provide
# any support, assistance, installation, training, or other services
# of any kind. Intel is also not obligated to provide any updates,
# enhancements or extensions. Intel specifically disclaims any warranty
# of merchantability, non-infringement, fitness for any particular
# purpose, and any other warranty.
#
# Further, Intel disclaims all liability of any kind, including but
# not limited to liability for infringement of any proprietary rights,
# relating to the use of the code, even if Intel is notified of the
# possibility of such liability. Except as expressly stated in an Intel
# license agreement provided with this code and agreed upon with Intel,
# no license, express or implied, by estoppel or otherwise, to any
# intellectual property rights is granted herein.

import ctypes
from . _miccommon import MicFlashException

FLASH_OP_STATUS	= (0x1 << 4)
SMC_OP_STATUS = (0x2 << 4)

FLASH_OP_IDLE = 0
FLASH_OP_INVALID = FLASH_OP_IDLE + 1
FLASH_OP_IN_PROGRESS = (FLASH_OP_STATUS | 1)
FLASH_OP_COMPLETED = FLASH_OP_IN_PROGRESS + 1
FLASH_OP_FAILED = FLASH_OP_COMPLETED + 1
FLASH_OP_AUTH_FAILED = FLASH_OP_FAILED + 1
SMC_OP_IN_PROGRESS = (SMC_OP_STATUS | 1)
SMC_OP_COMPLETED = SMC_OP_IN_PROGRESS + 1
SMC_OP_FAILED = SMC_OP_COMPLETED + 1
SMC_OP_AUTH_FAILED = SMC_OP_FAILED + 1

PAGE_SIZE		= (0x1000)
FILE_ACTIVE_OFFS	= (0x10000)
FILE_INACTIVE_OFFS	= (0x90000)
IMAGE_SIZE		= (0x70000)
FILL_FF_OFFS		= (0x38000)
EXTENDED_FW_OFFS	= (0xf0000)
FILE_EXTENDED_FW_OFFS	= (0x100000)
COMPRESSED_FW_OFFS	= (0x90000)
EXTENDED_FW_SIZE	= (0x80000)
FIXED_DATA_OFFS		= (0x1000)
FIXED_DATA_SIZE		= (0xf000)
R_SIZE			= (0x1000)
S_SIZE			= (0x1000)
BOOT_LOADER_OFFS	= (0x80000)
BOOT_LOADER_SIZE	= (0x10000)
DESC_DEVID_SSID	        = (0)
DESC_ADDR_MAP	        = (3)
DESC_CSS	        = (6)
FHI_INTERNAL	        = (0)
FHI_UPDATE	        = (1)
FHI_RELEASE	        = (2)
FHI_CUSTOM	        = (3)

ID_RPR		= (0x525052)

# Used ad_flags values 
FL_BLOCKED	= (0x04)
FL_SEL_TYPE	= (0x60)
FL_SEL_REPAIR	= (0x20)

FILE_CSS_HEADER_OFFS	= (0x38000)
CSS_HEADER_SIZE		= (0x284)
FS_MAGIC1		= (0x00ffaa55)
FS_MAGIC2		= (0xff0055aa)
OFFS_IMAGE_A		= (0x10000)
OFFS_IMAGE_B		= (0x90000)
CSS_HEADER_OFFS		= (0x28000)
CSS_HEADER_SIZE_AT	= (4)

FLASH_ERRORS = dict()
FLASH_ERRORS[FLASH_OP_IDLE] = MicFlashException("FLASH_OP_IDLE")
FLASH_ERRORS[FLASH_OP_INVALID] = MicFlashException("FLASH_OP_INVALID")
FLASH_ERRORS[FLASH_OP_IN_PROGRESS] = MicFlashException("FLASH_OP_IN_PROGRESS")
FLASH_ERRORS[FLASH_OP_FAILED] = MicFlashException("FLASH_OP_FAILED")
FLASH_ERRORS[FLASH_OP_AUTH_FAILED] = MicFlashException("FLASH_OP_AUTH_FAILED")
FLASH_ERRORS[SMC_OP_IN_PROGRESS] = MicFlashException("SMC_OP_IN_PROGRESS")
FLASH_ERRORS[SMC_OP_FAILED] = MicFlashException("SMC_OP_FAILED")
FLASH_ERRORS[SMC_OP_AUTH_FAILED] = MicFlashException("SMC_OP_AUTH_FAILED")

class failsafe_info(ctypes.Structure):
    c_uint = ctypes.c_uint32
    _fields_ = [("fsi_magic1", c_uint, 32),
               ("fsi_offset", c_uint, 32),
               ("fsi_offset_copy", c_uint, 32),
               ("fsi_magic2", c_uint, 32)]

FLASH_HEADER_OFFSET	= (ctypes.sizeof(failsafe_info))

class flash_desc(ctypes.Structure):
    c_uint = ctypes.c_uint32
    _fields_ = [("fd_type", c_uint),
                ("fd_size", c_uint),
                ("fd_data", c_uint)]

class flash_desc_data(ctypes.Structure):
    c_uint = ctypes.c_uint32
    _fields_ = [("fd_type", c_uint),
                ("fd_size", c_uint),
                ("fd_data", c_uint * 5)]

class flash_header(ctypes.Structure):
    c_uint16 = ctypes.c_uint16
    c_uint32 = ctypes.c_uint32
    _pack_ = 1
    _fields_ = [("fh_version", c_uint16, 3),
                ("fh_size", c_uint16, 13),
                ("fh_odm_rev", c_uint16),
                ("fh_image_type", c_uint32),
                ("fh_checksum1", c_uint32),
                ("fh_checksum2", c_uint32),
                ("fh_desc", flash_desc)]

class addrmap_desc(ctypes.Structure):
    c_uint32 = ctypes.c_uint32
    c_uint16 = ctypes.c_uint16
    c_uint8 = ctypes.c_uint8
    c_int = ctypes.c_int
    _pack_ = 1
    _fields_ = [("ad_id", c_uint32, 24),
                ("ad_flags", c_uint32, 8),
                ("ad_size", c_uint32),
                ("ad_offs", c_uint32),
                ("ad_alt", c_uint32)]
    
def raise_flash_error(status):
    try:
        raise FLASH_ERRORS[status]
    except KeyError:
        return
        
def load_from_buffer(dest_struct, buf, size, offset=0):
    """"Casts" the ctypes buffer buf to ctypes struct dest_struct,
    offsetting it offset bytes, and copying size bytes.
    """
    memmove = ctypes.memmove
    addressof = ctypes.addressof
    sizeof = ctypes.sizeof
    byref = ctypes.byref
    memmove(addressof(dest_struct), byref(buf, offset), size)
        
def move_buffer_chunk(dest_buf, src_buf, size, offset=0):
    """Moves to dest_buf the contents of src_buf, from byte offset
    to byte size.
    """
    memmove = ctypes.memmove
    addressof = ctypes.addressof
    sizeof = ctypes.sizeof
    byref = ctypes.byref
    memmove(dest_buf, byref(src_buf, offset), size)
        
def find_desc(flash_buf, utype, offset_for_flash_hdr):
    """Find suitable descriptor
    
    Expects flash buffer, and offset for the flash header.
    Returns tuple (descriptor, offset_for_descriptor). This
    way, descriptor can be re-referenced using its offset
    (relative to flash buffer).
    """
    addressof = ctypes.addressof
    sizeof = ctypes.sizeof
    memmove = ctypes.memmove
    byref = ctypes.byref
    local_fhdr = flash_header()
    load_from_buffer(local_fhdr,
                      flash_buf,
                      sizeof(flash_header),
                      offset_for_flash_hdr)
    hdr_size = local_fhdr.fh_size
    desc_offs = addressof(local_fhdr.fh_desc) - addressof(local_fhdr)
    fdesc = flash_desc()

    while desc_offs < hdr_size:
        # add offset_for_flash_hdr
        # to compensate for flash header offset
        load_from_buffer(fdesc,
                          flash_buf,
                          sizeof(flash_desc),
                          desc_offs +
                          offset_for_flash_hdr) 
        if fdesc.fd_type == utype:
            #set desc_offs relative to flash_buf start
            desc_offs += offset_for_flash_hdr
            break
        
        ##sizeof(fdesc.fd_data[0]) = sizeof(cuint)
        desc_offs += sizeof(flash_desc) + \
                     (fdesc.fd_size - 1) * \
                     sizeof(ctypes.c_uint) 
        
    if fdesc.fd_type != utype:
        return None
    
    return fdesc, desc_offs

def process_flash_buffer(mic_obj, flash_buf, _buf_size):
    """Processes flash_buf read via mic_read_flash into
    an active flash image.
    """
    byref = ctypes.byref
    sizeof = ctypes.sizeof
    addressof = ctypes.addressof
    #Get buf_size.
    try:
        # If _buf_size is int
        buf_size = ctypes.c_size_t(int(_buf_size))
    except TypeError:
        #If _buf_size if a ctypes type
        buf_size = ctypes.c_size_t(_buf_size.value)
        
    flash_size = ctypes.c_size_t()
    fs = failsafe_info()
    fsi = failsafe_info()
    active = ctypes.c_ulong()
    other = ctypes.c_ulong()
    hd = flash_desc_data()
    desc = flash_desc()
    flash_hdr = flash_header()
    flash_hdr2 = flash_header()
    p = ctypes.c_voidp()
    n_entries = ctypes.c_uint32()
    block_size = ctypes.c_uint32()
    ad = addrmap_desc()
    #Buffer to return flashable image
    outbuf = None
    outbuf_size = ctypes.c_uint32()
    
    #Get flash image size
    ret_code = mic_obj.mic.mic_flash_size(mic_obj.mdh, byref(flash_size))
    if ret_code:
        raise MicFlashException("Failed to read flash header information")
    
    if buf_size.value < flash_size.value:
        raise MicFlashException("Internal Error: Flash size (0x%s) is smaller \
                           than header (0x%s)" % (hex(buf_size.value),
                                                  hex(flash_size.value)))
    
    # Load fs
    load_from_buffer(fs, flash_buf, sizeof(failsafe_info))
    
    if fs.fsi_magic1 != FS_MAGIC1 or fs.fsi_magic2 != FS_MAGIC2:
        raise MicFlashException("Corrupted flash image: magic 0x%s, 0x%s" %
                           (hex(fs.fsi_magic1), hex(fs.fsi_magic2)))
    
    # Get active offset
    ret_code = mic_obj.mic.mic_flash_active_offs(mic_obj.mdh, byref(active))
    if ret_code:
        raise MicFlashException("Failed to read active offs")
    
    if active.value != OFFS_IMAGE_A and active.value != OFFS_IMAGE_B:
        raise MicFlashException("Corrupted flash image: Active offset: 0x%s"
                           % hex(active.value))

    if(active.value == OFFS_IMAGE_A):
        other.value = OFFS_IMAGE_B
    else:
        other.value = OFFS_IMAGE_A
    
    offset_for_hd = active.value + CSS_HEADER_OFFS
    # Load flash descriptor
    load_from_buffer(hd, flash_buf, sizeof(flash_desc_data), offset_for_hd)
    
    if hd.fd_type != DESC_CSS:
        raise MicFlashException("Malformed image: Bad descriptor: 0x%s" %
                           hex(hd.fd_type))
    
    #Get flash header
    offset_for_flash_hdr = offset_for_hd + \
                           CSS_HEADER_SIZE + \
                           ctypes.sizeof(failsafe_info)
    load_from_buffer(flash_hdr,
                      flash_buf,
                      sizeof(flash_header),
                      offset_for_flash_hdr)
    
    #Failsafe info to be written into file.
    fsi.fsi_magic1 = FS_MAGIC1
    fsi.fsi_magic2 = FS_MAGIC2
    fsi.fsi_offset = OFFS_IMAGE_A
    fsi.fsi_offset_copy = OFFS_IMAGE_A
    
    image_type = flash_hdr.fh_image_type
    
    if image_type == FHI_UPDATE:
        load_from_buffer(flash_hdr2,
                          flash_buf,
                          sizeof(flash_header),
                          sizeof(failsafe_info))
        
        if flash_hdr2.fh_image_type == FHI_RELEASE:
            flash_hdr = flash_hdr2
        
        outbuf_size.value = hd.fd_data[CSS_HEADER_SIZE_AT] * \
                            sizeof(ctypes.c_uint)
        
        outbuf = ctypes.create_string_buffer(outbuf_size.value)
        move_buffer_chunk(byref(outbuf, 0),
                               flash_buf,
                               CSS_HEADER_SIZE,
                               active.value + CSS_HEADER_OFFS)
        move_buffer_chunk(byref(outbuf,
                                 CSS_HEADER_SIZE + \
                                 sizeof(failsafe_info)),
                               flash_buf,
                               PAGE_SIZE,
                               active.value + \
                               CSS_HEADER_OFFS + \
                               CSS_HEADER_SIZE + \
                               sizeof(failsafe_info))
        
        move_buffer_chunk(byref(outbuf, FILE_ACTIVE_OFFS + CSS_HEADER_SIZE),
                                flash_buf,
                                IMAGE_SIZE,
                                active.value)
        
        ctypes.memset(byref(outbuf, FILL_FF_OFFS + CSS_HEADER_SIZE),
                      0xff,
                      2 * PAGE_SIZE)
        
        move_buffer_chunk(byref(outbuf, COMPRESSED_FW_OFFS + CSS_HEADER_SIZE),
                                flash_buf,
                                EXTENDED_FW_SIZE,
                                active.value + EXTENDED_FW_OFFS)
        
        move_buffer_chunk(byref(outbuf, CSS_HEADER_SIZE + FIXED_DATA_OFFS),
                                flash_buf,
                                FIXED_DATA_SIZE,
                                FIXED_DATA_OFFS)
        
        desc, desc_offset = find_desc(flash_buf,
                                       DESC_ADDR_MAP,
                                       offset_for_flash_hdr)
        if desc is None:
            raise MicFlashException("No suitable descriptor found")
        
        n_entries = desc.fd_size / \
                    (sizeof(addrmap_desc) / \
                     sizeof(ctypes.c_uint32))
        
        ad = addrmap_desc()
        desc_offset = desc_offset + desc.fd_data.offset
        load_from_buffer(ad, flash_buf, sizeof(addrmap_desc), desc_offset)
        iteration = 0
        while n_entries:
            n_entries-=1
            iteration += 1
            block_size = ad.ad_size
            if ad.ad_id == ID_RPR:
                if block_size < (R_SIZE + S_SIZE):
                    block_size = R_SIZE + S_SIZE
                    
            if (ad.ad_flags & FL_BLOCKED) or \
               ((ad.ad_flags & FL_SEL_TYPE) == FL_SEL_REPAIR):
                ctypes.memset(byref(outbuf, CSS_HEADER_SIZE + ad.ad_offs),
                              0xff, block_size)
            
            load_from_buffer(ad, flash_buf,
                              sizeof(addrmap_desc),
                              desc_offset +
                              (sizeof(addrmap_desc) * iteration))

        move_buffer_chunk(byref(outbuf, CSS_HEADER_SIZE),
                           fsi,
                           sizeof(failsafe_info))
        
        return outbuf, outbuf_size.value
    
    elif image_type == FHI_CUSTOM:
        outbuf_size.value = \
                    (hd.fd_data[CSS_HEADER_SIZE_AT] * sizeof(ctypes.c_uint)) - \
                    CSS_HEADER_SIZE
        
        outbuf = ctypes.create_string_buffer(outbuf_size.value)
        
        move_buffer_chunk(byref(outbuf, sizeof(failsafe_info)),
                                flash_buf,
                                PAGE_SIZE,
                                active.value + CSS_HEADER_OFFS + CSS_HEADER_SIZE
                                + sizeof(failsafe_info))
        
        move_buffer_chunk(byref(outbuf, FILE_ACTIVE_OFFS),
                                flash_buf,
                                IMAGE_SIZE,
                                active.value)
        
        move_buffer_chunk(byref(outbuf, BOOT_LOADER_OFFS),
                                flash_buf,
                                BOOT_LOADER_SIZE,
                                BOOT_LOADER_OFFS)
        
        move_buffer_chunk(byref(outbuf, FILE_INACTIVE_OFFS),
                                flash_buf,
                                IMAGE_SIZE,
                                other.value)
        
        move_buffer_chunk(byref(outbuf, FILE_EXTENDED_FW_OFFS),
                                flash_buf,
                                EXTENDED_FW_SIZE,
                                active.value + EXTENDED_FW_OFFS)
        
        move_buffer_chunk(byref(outbuf, FIXED_DATA_OFFS),
                                flash_buf,
                                FIXED_DATA_SIZE,
                                FIXED_DATA_OFFS)
        
        move_buffer_chunk(outbuf, fsi, sizeof(failsafe_info))
        
        return outbuf, outbuf_size
