/*
 * Copyright 2010-2013 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, version 2.1.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * Disclaimer: The codes contained in these modules may be specific
 * to the Intel Software Development Platform codenamed Knights Ferry,
 * and the Intel product codenamed Knights Corner, and are not backward
 * compatible with other Intel products. Additionally, Intel will NOT
 * support the codes or instruction set in future products.
 *
 * Intel offers no warranty of any kind regarding the code. This code is
 * licensed on an "AS IS" basis and Intel is not obligated to provide
 * any support, assistance, installation, training, or other services
 * of any kind. Intel is also not obligated to provide any updates,
 * enhancements or extensions. Intel specifically disclaims any warranty
 * of merchantability, non-infringement, fitness for any particular
 * purpose, and any other warranty.
 *
 * Further, Intel disclaims all liability of any kind, including but
 * not limited to liability for infringement of any proprietary rights,
 * relating to the use of the code, even if Intel is notified of the
 * possibility of such liability. Except as expressly stated in an Intel
 * license agreement provided with this code and agreed upon with Intel,
 * no license, express or implied, by estoppel or otherwise, to any
 * intellectual property rights is granted herein.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <dirent.h>
#include <regex.h>
#include <ctype.h>
#include "helper.h"

#define ASSERT    assert

char *progname = NULL;
char *smc_bl;
char *flash_dir = "/usr/share/mpss/flash";

/*
 * Checks the next arg for a valid command. Returns NULL if there is
 * no next arg, or valid command, pointer to the actions structure
 * otherwise.
 */
struct cmd_actions *parse_actions(struct cmd_actions *cactions, int *index,
                                  int argc,
                                  char *argv[])
{
    int option_list = *index;
    struct cmd_actions *p;

    if (option_list >= argc)
        return NULL;

    for (p = cactions; p->cmd_name; p++) {
        if (!strcmp(argv[option_list], p->cmd_name)) {
            *index = option_list + 1;
            return p;
        }
    }

    return NULL;
}

/*
 * A version of strotll(3) that guesses the base. Assumes no leading white
 * spaces.
 */
long get_numl(char *s, char **more)
{
    int base = 10;

    ASSERT(s);
    ASSERT(more);

    if ((s[0] == '0') && s[1] != '\0') {
        base = 8;
        s++;

        if (s[0] == 'x') {
            base = 16;
            s++;
        }
    }

    errno = 0;
    return strtol(s, more, base);
}

/* The following should be read from flash repo */
#pragma pack(push, 1)

struct failsafe_info {
    uint32_t fsi_magic1;
    uint32_t fsi_offset;
    uint32_t fsi_offset_copy;
    uint32_t fsi_magic2;
};

struct flash_desc {
    uint32_t fd_type;
    uint32_t fd_size;
    uint32_t fd_data[1];
};

/* Index into fd_data */
#define CSS_HEADER_SIZE_AT    (4)

/* Valid fdh_type values */
#define DESC_DEVID_SSID       (0)
#define DESC_ADDR_MAP         (3)
#define DESC_CSS              (6)

struct flash_header {
    uint16_t          fh_version : 3;
    uint16_t          fh_size    : 13;
    uint16_t          fh_odm_rev;
    uint32_t          fh_image_type;
    uint32_t          fh_checksum1;
    uint32_t          fh_checksum2;
    struct flash_desc fh_desc[1];
};

/* Valid fh_image_type */
#define FHI_INTERNAL    (0)
#define FHI_UPDATE      (1)
#define FHI_RELEASE     (2)
#define FHI_CUSTOM      (3)

struct addrmap_desc {
    uint32_t ad_id    : 24;
    uint32_t ad_flags : 8;
    uint32_t ad_size;
    uint32_t ad_offs;
    uint32_t ad_alt;
};

/* An ad_id that's used */
#define ID_RPR           (0x525052)

/* Used ad_flags values */
#define FL_BLOCKED       (0x04)
#define FL_SEL_TYPE      (0x60)
#define FL_SEL_REPAIR    (0x20)

struct desc_id {
    uint32_t di_dev_id;
    uint32_t di_ssid;
    uint16_t di_rev_id_low;
    uint16_t di_rev_id_high;
};

static struct flash_desc *find_desc(struct flash_header *, uint32_t);

#pragma pack(pop)

#define FLASH_HEADER_OFFSET     (sizeof(struct failsafe_info))
#define FILE_CSS_HEADER_OFFS    (0x38000)
#define CSS_HEADER_SIZE         (0x284)

#define FS_MAGIC1               (0x00ffaa55)
#define FS_MAGIC2               (0xff0055aa)

#define OFFS_IMAGE_A            (0x10000)
#define OFFS_IMAGE_B            (0x90000)

#define CSS_HEADER_OFFS         (0x28000)

#define BUF_OFFS(buf, offs)    ((void *)((char *)(buf) + (offs)))

static struct flash_desc *find_desc(struct flash_header *flash_hdr,
                                    uint32_t type)
{
    uint32_t hdr_size, desc_offs;
    struct flash_desc *desc = NULL;

    hdr_size = flash_hdr->fh_size;
    desc_offs = (uint32_t)OFFSET_OF(fh_desc[0], flash_hdr);

    while (desc_offs < hdr_size) {
        desc = (struct flash_desc *)((uint8_t *)flash_hdr + desc_offs);

        if (desc->fd_type == type)
            break;

        desc_offs += (sizeof(struct flash_desc) +
                      (desc->fd_size - 1) * sizeof(desc->fd_data[0]));
    }

    if (desc != NULL && desc->fd_type != type)
        return NULL;

    return desc;
}

int flash_to_file_image(struct mic_device *mdh, void *buf, size_t buf_size,
                        void **outbuf, size_t *outbuf_size)
{
    size_t flash_size;
    struct failsafe_info *fs, fsi;
    off_t active, other;
    struct flash_desc *hd, *desc;
    struct flash_header *flash_hdr, *flash_hdr2;
    void *p;
    uint32_t n_entries, block_size;
    struct addrmap_desc *ad;

/* Check the names of all these #define identifiers. */
#define PAGE_SIZE                (0x1000)
#define FILE_ACTIVE_OFFS         (0x10000)
#define FILE_INACTIVE_OFFS       (0x90000)
#define IMAGE_SIZE               (0x70000)
#define FILL_FF_OFFS             (0x38000)
#define EXTENDED_FW_OFFS         (0xf0000)
#define FILE_EXTENDED_FW_OFFS    (0x100000)
#define COMPRESSED_FW_OFFS       (0x90000)
#define EXTENDED_FW_SIZE         (0x80000)
#define FIXED_DATA_OFFS          (0x1000)
#define FIXED_DATA_SIZE          (0xf000)
#define R_SIZE                   (0x1000)
#define S_SIZE                   (0x1000)
#define BOOT_LOADER_OFFS         (0x80000)
#define BOOT_LOADER_SIZE         (0x10000)

    if (mic_flash_size(mdh, &flash_size) != E_MIC_SUCCESS) {
        error_msg_start("%s: Failed to read flash header information: "
                        "%s: %s\n", mic_get_device_name(mdh),
                        mic_get_error_string(), strerror(errno));
        return OTHER_ERR;
    }

    UT_INSTRUMENT_EVENT("FLASH1_INSTRUMENT_FLASH_TO_IMAGE_0",
                        buf_size = flash_size - 1);
    if (buf_size < flash_size) {
        error_msg_start
            ("%s: Internal error: Flash size (0x%lx bytes) is "
            "smaller than the header (0x%lx bytes)\n",
            mic_get_device_name(mdh), buf_size, flash_size);
        return OTHER_ERR;
    }

    fs = (struct failsafe_info *)buf;

    UT_INSTRUMENT_EVENT("FLASH1_INSTRUMENT_FLASH_TO_IMAGE_1",
                        fs->fsi_magic1 = 0);
    if ((fs->fsi_magic1 != FS_MAGIC1) || (fs->fsi_magic2 != FS_MAGIC2)) {
        error_msg_start("%s: Corrupted flash image: magic 0x%x, 0x%x\n",
                        mic_get_device_name(mdh),
                        fs->fsi_magic1, fs->fsi_magic2);
        return OTHER_ERR;
    }

    if (mic_flash_active_offs(mdh, &active) != E_MIC_SUCCESS) {
        error_msg_start("%s: Failed to read active offs: "
                        "%s: %s\n", mic_get_device_name(mdh),
                        mic_get_error_string(), strerror(errno));
        return OTHER_ERR;
    }

    UT_INSTRUMENT_EVENT("FLASH1_INSTRUMENT_FLASH_TO_IMAGE_2",
                        active = 0);
    if ((active != OFFS_IMAGE_A) && (active != OFFS_IMAGE_B)) {
        error_msg_start("%s: Corrupted flash image: Active offset: "
                        "0x%lx\n", mic_get_device_name(mdh), active);
        return OTHER_ERR;
    }

    other = active == OFFS_IMAGE_A ? OFFS_IMAGE_B : OFFS_IMAGE_A;

    hd = (struct flash_desc *)((char *)buf + active + CSS_HEADER_OFFS);
    UT_INSTRUMENT_EVENT("FLASH1_INSTRUMENT_FLASH_TO_IMAGE_3",
                        hd->fd_type = DESC_CSS + 1);
    if (hd->fd_type != DESC_CSS) {
        error_msg_start("%s: Malformed image: Bad descriptor: 0x%x\n",
                        mic_get_device_name(mdh), hd->fd_type);
        return OTHER_ERR;
    }

    flash_hdr = (struct flash_header *)BUF_OFFS(buf, active +
                                                CSS_HEADER_OFFS +
                                                CSS_HEADER_SIZE +
                                                sizeof(struct
                                                       failsafe_info));

    /*
     * Failsafe info to be written into file.
     */
    fsi.fsi_magic1 = FS_MAGIC1;
    fsi.fsi_magic2 = FS_MAGIC2;
    fsi.fsi_offset = OFFS_IMAGE_A;
    fsi.fsi_offset_copy = OFFS_IMAGE_A;

    switch (flash_hdr->fh_image_type) {
    case FHI_UPDATE:
        /* Likely not supported. */
        flash_hdr2 = (struct flash_header *)((char *)buf +
                                             sizeof(struct
                                                    failsafe_info));
        if (flash_hdr2->fh_image_type == FHI_RELEASE)
            flash_hdr = flash_hdr2;
    /* FALL THRU */

    case FHI_RELEASE:
        *outbuf_size = hd->fd_data[CSS_HEADER_SIZE_AT] *
                       sizeof(uint32_t);
        p = *outbuf = calloc(*outbuf_size, 1);
        if (p == NULL) {
            error_msg_start("%s: malloc: %s\n",
                            mic_get_device_name(mdh),
                            strerror(errno));
            return MEM_ERR;
        }

        (void)memcpy(BUF_OFFS(p, 0),
                     BUF_OFFS(buf, active + CSS_HEADER_OFFS),
                     CSS_HEADER_SIZE);

        (void)memcpy(BUF_OFFS(p,
                              CSS_HEADER_SIZE +
                              sizeof(struct failsafe_info)),
                     BUF_OFFS(buf,
                              active + CSS_HEADER_OFFS +
                              CSS_HEADER_SIZE +
                              sizeof(struct failsafe_info)),
                     PAGE_SIZE);

        (void)memcpy(BUF_OFFS(p, FILE_ACTIVE_OFFS + CSS_HEADER_SIZE),
                     BUF_OFFS(buf, active), IMAGE_SIZE);

        /* Fill with 0xff's */
        (void)memset(BUF_OFFS(p, FILL_FF_OFFS + CSS_HEADER_SIZE),
                     0xff, 2 * PAGE_SIZE);

        (void)
        memcpy(BUF_OFFS(p, COMPRESSED_FW_OFFS + CSS_HEADER_SIZE),
               BUF_OFFS(buf, active + EXTENDED_FW_OFFS),
               EXTENDED_FW_SIZE);

        (void)memcpy(BUF_OFFS(p, CSS_HEADER_SIZE + FIXED_DATA_OFFS),
                     BUF_OFFS(buf, FIXED_DATA_OFFS), FIXED_DATA_SIZE);

        desc = find_desc(flash_hdr, DESC_ADDR_MAP);
        if (desc == NULL)
            break;

        n_entries = desc->fd_size / (sizeof(struct addrmap_desc) /
                                     sizeof(uint32_t));
        ad = (struct addrmap_desc *)desc->fd_data;

        while (n_entries--) {
            block_size = ad->ad_size;
            if ((ad->ad_id) == ID_RPR) {
                if (block_size < (R_SIZE + S_SIZE))
                    block_size = (R_SIZE + S_SIZE);
            }

            if ((ad->ad_flags & FL_BLOCKED) ||
                ((ad->ad_flags & FL_SEL_TYPE) == FL_SEL_REPAIR)) {
                /* Fill with ff's */
                memset(BUF_OFFS
                           (p, CSS_HEADER_SIZE +
                           ad->ad_offs), 0xff,
                       block_size);
            }
            ad++;
        }

        (void)memcpy(BUF_OFFS(p, CSS_HEADER_SIZE),
                     (void *)&fsi, sizeof(struct failsafe_info));
        break;

    case FHI_INTERNAL:
    case FHI_CUSTOM:
        *outbuf_size = hd->fd_data[CSS_HEADER_SIZE_AT] *
                       sizeof(uint32_t) - CSS_HEADER_SIZE;
        p = *outbuf = calloc(*outbuf_size, 1);
        if (p == NULL) {
            error_msg_start("%s: malloc: %s\n",
                            mic_get_device_name(mdh),
                            strerror(errno));
            return MEM_ERR;
        }

        (void)memcpy(BUF_OFFS(p, sizeof(struct failsafe_info)),
                     BUF_OFFS(buf,
                              active + CSS_HEADER_OFFS +
                              CSS_HEADER_SIZE +
                              sizeof(struct failsafe_info)),
                     PAGE_SIZE);

        (void)memcpy(BUF_OFFS(p, FILE_ACTIVE_OFFS),
                     BUF_OFFS(buf, active), IMAGE_SIZE);

        (void)memcpy(BUF_OFFS(p, BOOT_LOADER_OFFS),
                     BUF_OFFS(buf, BOOT_LOADER_OFFS),
                     BOOT_LOADER_SIZE);

        (void)memcpy(BUF_OFFS(p, FILE_INACTIVE_OFFS),
                     BUF_OFFS(buf, other), IMAGE_SIZE);

        (void)memcpy(BUF_OFFS(p, FILE_EXTENDED_FW_OFFS),
                     BUF_OFFS(buf, active + EXTENDED_FW_OFFS),
                     EXTENDED_FW_SIZE);

        (void)memcpy(BUF_OFFS(p, FIXED_DATA_OFFS),
                     BUF_OFFS(buf, FIXED_DATA_OFFS), FIXED_DATA_SIZE);

        (void)memcpy(BUF_OFFS(p, 0), (void *)&fsi,
                     sizeof(struct failsafe_info));
        break;
    }

    return 0;
}

/*
 * Get flash image start offs and size. Returns -1 if no image exists, 0
 * if flash image exists, and 1 if SMC only.
 */
static int get_flash_block_start_size(struct mic_device *mdh, const char *file,
                                      void *buf,
                                      size_t buf_size, off_t *offs,
                                      size_t *size)
{
    struct failsafe_info *fs;
    struct flash_desc *hd;
    size_t image_size;

    fs = (struct failsafe_info *)buf;

    if ((fs->fsi_magic1 == FS_MAGIC1) && (fs->fsi_magic2 == FS_MAGIC2)) {
        /*
         * Magic numbers are right at the beginning of the file -
         * It's an internal or a development image.
         */
        struct flash_header *fh;

        /*
         * Make sure that we'll be able to read at least the CSS
         * header.
         */
        if (buf_size < (FILE_CSS_HEADER_OFFS + CSS_HEADER_SIZE)) {
            error_msg_start("%s: %s: Malformed file: "
                            "No CSS header\n",
                            mic_get_device_name(mdh), file);
            return -1;
        }

        hd = (struct flash_desc *)
             (((unsigned char *)buf) + FILE_CSS_HEADER_OFFS);

        if (hd->fd_type != DESC_CSS) {
            error_msg_start("%s: %s: Malformed file: Bad header\n",
                            mic_get_device_name(mdh), file);
            return -1;
        }

        fh = (struct flash_header *)
             (((unsigned char *)buf) + FLASH_HEADER_OFFSET);

        image_size = hd->fd_data[CSS_HEADER_SIZE_AT] *
                     sizeof(uint32_t);

        switch (fh->fh_image_type) {
        case FHI_RELEASE:
            *size = buf_size;
            break;

        case FHI_INTERNAL:
            if (image_size < buf_size)
                *size = image_size - CSS_HEADER_SIZE;
            else
                *size = buf_size;
            break;

        default:
            error_msg_start("%s: %s: Malformed file: "
                            "Unexpected image type: 0x%x\n",
                            mic_get_device_name(mdh), file,
                            fh->fh_image_type);
            return -1;
        }
        *offs = 0;
    } else {
        if (buf_size < CSS_HEADER_SIZE) {
            error_msg_start("%s: %s: Malformed file: "
                            "No CSS header\n",
                            mic_get_device_name(mdh), file);
            return -1;
        }

        fs = (struct failsafe_info *)BUF_OFFS(buf, CSS_HEADER_SIZE);
        if ((fs->fsi_magic1 != FS_MAGIC1) ||
            (fs->fsi_magic2 != FS_MAGIC2)) {
            /* Likely, SMC with CSS header */
            *offs = 0;
            *size = 0;
            return 1;
        }

        hd = (struct flash_desc *)buf;

        if (hd->fd_type != DESC_CSS) {
            error_msg_start("%s: %s: Malformed file: Bad header\n",
                            mic_get_device_name(mdh), file);
            return -1;
        }

        image_size = hd->fd_data[CSS_HEADER_SIZE_AT] * sizeof(uint32_t);

        if (image_size <= buf_size) {
            /* Flash + SMC image or Flash only */
            *offs = CSS_HEADER_SIZE;
            *size = image_size - CSS_HEADER_SIZE;
        } else {
            /* SMC only */
            *offs = 0;
            *size = 0;
            return 1;
        }
    }

    return 0;
}

/* Versions string is of the form: v1.v2.v3.v4 */
#define N_VERSION    (4)

static int extract_version(const char *str, uint32_t va[N_VERSION])
{
    char *ep;
    const char *s;
    int i;
    uint32_t t;

    errno = 0;
    s = str;
    for (i = 0; i < N_VERSION - 1; i++) {
        va[i] = strtol(s, &ep, 10);
        if ((*ep != '.') || (errno != 0))
            return -1;
        s = ep + 1;
    }
    va[i] = strtol(s, &ep, 10);

    if ((*ep != '\0') || (errno != 0))
        return -1;

    /* Minor version is returned just before the major version:
     * For example: 2.1.383-1 is higher than 2.1.381.3 but
     * these are returned as (2,1,1,383) and (2,1,3,381)
     * respectively. Swap the two for later comparison.
     */
    t = va[N_VERSION - 2];
    va[N_VERSION - 2] = va[N_VERSION - 1];
    va[N_VERSION - 1] = t;

    return 0;
}


int compare_version(const uint32_t *v1, const uint32_t *v2)
{
    int i;

    for (i = 0; i < N_VERSION; i++) {
        if (v1[i] != v2[i])
            break;
    }

    if (i == N_VERSION)
        return 0;

    return v1[i] - v2[i];
}

int read_flash_device_maint_mode(struct mic_device *device, void **rbuf,
                                 size_t *size)
{
    struct mic_flash_op *desc = NULL;
    void *buf = NULL;
    uint32_t interval;

    if (mic_flash_size(device, size) != E_MIC_SUCCESS) {
        error_msg_start(
            "%s: Failed to read flash header information: "
            "%s: %s\n", mic_get_device_name(device),
            mic_get_error_string(), strerror(errno));
        return OTHER_ERR;
    }

    buf = valloc(*size);
    if (buf == NULL) {
        error_msg_start("%s: malloc: %s\n",
                        mic_get_device_name(device), strerror(errno));
        return MEM_ERR;
    }

    if (mic_flash_read_start(device, buf, *size, &desc) != E_MIC_SUCCESS) {
        error_msg_start("%s: Failed to start flash read: %s: %s\n",
                        mic_get_device_name(device),
                        mic_get_error_string(), strerror(errno));
        free(buf);
        return OTHER_ERR;
    }

    interval = cmdopts.line_mode_flag ? 0 : FLASH_READ_POLL_INTERVAL;
    if (poll_flash_op(device, desc, interval,
                      FLASH_READ_TIMEOUT, 0, 0) != 0) {
        (void)mic_flash_read_done(desc);
        free(buf);
        return OTHER_ERR;
    }

    if (mic_flash_read_done(desc) != E_MIC_SUCCESS) {
        error_msg_start("%s: Failed to read flash header: %s: %s\n",
                        mic_get_device_name(device),
                        mic_get_error_string(), strerror(errno));
        free(buf);
        return OTHER_ERR;
    }

    *rbuf = buf;
    return 0;
}

/*
 * Verify if the given file image is compatible with the given device. Returns
 * 0 if it is (*flash_image is set zero if possibly an SMC image, it's set to
 * non-zero value if it's a flash image). Negative values on error and a non-
 * zero positive value if image is not compatible.
 */
int verify_image(const char *file, struct mic_device *mdh, void *fbuf,
                 size_t fbufsize, int *flash_image, int oldimage_ok)
{
    uint32_t i, csum;
    off_t first_block, start, last;
    size_t size;
    uint32_t *ubuf;
    uint16_t vendor_id, dev_id, subsys_id;
    uint8_t rev_id;
    struct flash_header *flash_hdr;
    struct flash_desc *desc;
    struct desc_id *di;
    uint32_t dev_id_vendor, ssid_vendor;
    struct mic_pci_config *pci_config;
    char fver[NAME_MAX], dver[NAME_MAX];
    int ret;
    uint32_t fv[N_VERSION], dv[N_VERSION];
    void *dbuf;
    size_t dsize;

    /* Checksum the image */
    if ((ret = get_flash_block_start_size(mdh, file, fbuf, fbufsize,
                                          &first_block, &size)) < 0)
        return -OTHER_ERR;

    if (ret == 1) {
        /* SMC only */
        *flash_image = 0;
        return 0;
    }

    *flash_image = 1;
    if (size < (size_t)first_block) {
        error_msg_start("%s: %s: Image too small\n",
                        mic_get_device_name(mdh), file);
        return -OTHER_ERR;
    }

    if (size & 3) {
        error_msg_start("%s: %s: Image size is not a multiple of 4\n",
                        mic_get_device_name(mdh), file);
        return -OTHER_ERR;
    }

    flash_hdr = (struct flash_header *)((uint8_t *)fbuf + first_block
                                        + FLASH_HEADER_OFFSET);

    start = first_block >>= 2;
    last = start + (size >> 2);

    ubuf = (uint32_t *)fbuf;
    csum = 0;
    for (i = start; i < last; i++)
        csum += ubuf[i];

    if (csum != 0) {
        error_msg_start("%s: %s: Checksum verification failed\n",
                        mic_get_device_name(mdh), file);
        return -OTHER_ERR;
    }

    if (mic_get_pci_config(mdh, &pci_config) != E_MIC_SUCCESS) {
        error_msg_start("%s: System error: failed to read PCI "
                        "configuration: %s: %s\n",
                        mic_get_device_name(mdh),
                        mic_get_error_string(), strerror(errno));
        return -OTHER_ERR;
    }

    if ((mic_get_vendor_id(pci_config, &vendor_id) != E_MIC_SUCCESS) ||
        (mic_get_device_id(pci_config, &dev_id) != E_MIC_SUCCESS) ||
        (mic_get_revision_id(pci_config, &rev_id) != E_MIC_SUCCESS) ||
        (mic_get_subsystem_id(pci_config, &subsys_id) != E_MIC_SUCCESS)) {
        error_msg_start("%s: System error: failed to read PCI "
                        "information: %s: %s\n",
                        mic_get_device_name(mdh),
                        mic_get_error_string(), strerror(errno));
        mic_free_pci_config(pci_config);
        return -OTHER_ERR;
    }

    mic_free_pci_config(pci_config);
    dev_id_vendor = (((uint32_t)dev_id) << 16) | vendor_id;
    ssid_vendor = (((uint32_t)subsys_id) << 16) | vendor_id;

    desc = find_desc(flash_hdr, DESC_DEVID_SSID);
    if (desc == NULL) {
        error_msg_start("%s: %s: Corrupted image: "
                        "No dev-id/ssid section",
                        mic_get_device_name(mdh), file);
        return -OTHER_ERR;
    }

    ASSERT((desc->fd_size % 3) == 0);
    di = (struct desc_id *)desc->fd_data;

    for (i = desc->fd_size; i; di++, i -=
             (sizeof(*di) / sizeof(uint32_t))) {
        if ((di->di_dev_id == dev_id_vendor) &&
            (di->di_ssid == ssid_vendor) &&
            (rev_id >= di->di_rev_id_low) &&
            (rev_id <= di->di_rev_id_high))
            break;
    }

    if (i == 0)
        return IMAGE_MISMATCH;

    if (!oldimage_ok) {
        if ((ret = read_flash_device_maint_mode(mdh, &dbuf,
                                                &dsize)) != 0)
            return -ret;

        if (mic_flash_version(mdh, dbuf, dver, sizeof(dver))
            != E_MIC_SUCCESS) {
            error_msg_start("%s: Failed to retrieve version: "
                            "%s: %s\n", mic_get_device_name(mdh),
                            mic_get_error_string(), strerror(errno));
            free(dbuf);
            return -OTHER_ERR;
        }

        if ((ret = flash_file_version(file, fbuf, fver, sizeof(fver)))
            != 0) {
            free(dbuf);
            return -ret;
        }

        if (extract_version(fver, fv) < 0) {
            error_msg_start("%s: %s: Invalid version string: %s\n",
                            mic_get_device_name(mdh), file, fver);
            free(dbuf);
            return -OTHER_ERR;
        }

        ASSERT(extract_version(dver, dv) == 0);

        if (compare_version(fv, dv) < 0) {
            error_msg_start("%s: %s: File image version older "
                            "than flash\n",
                            mic_get_device_name(mdh), file);
            free(dbuf);
            return -OTHER_ERR;
        }

        free(dbuf);
    }

    return 0;
}

static char *flash_status_str(int status)
{
    static char *err_str[] = {
        "Invalid signature (0x0)",
        "Unsupported maintenance mode handler (0x1)",
        "VMM authentication failed (0x2)",
        "Invalid download address (0x3)",
        "Unsigned image (0x4)",
        "Invalid upload address (0x5)"
    };

    if (status >= (int)(sizeof(err_str) / sizeof(char *)))
        return "Unknown error";

    return err_str[status];
}

static char *smc_status_str(int status)
{
    static char *err_str[] = {
        "Unknown error (0x0)",
        "SMC buffer size exceeded (0x1)",
        "SMC Flash size error (0x2)",
        "Failed to erase SMC Flash (0x3)",
        "Failed to write SMC Flash (0x4)",
        "Invalid SMC image (0x5)",
        "SMC protocol error (0x6)",
        "Invalid error mask (0x7)",
        "Invalid Firmware update state (0x8)",
        "SMC self-test error (0x9)",
        "Flash image too big (0xa)",
        "SMC failed to switch to update state (0xb)",
        "SMC failed to switch to firmware download state (0xc)",
        "SMC not in idle state (0xd)",
        "CRC mismatch (0xe)",
        "Invalid request (0xf)"
    };

    if (status >= (int)(sizeof(err_str) / sizeof(char *)))
        return "Unknown error";

    return err_str[status];
}

static int check_flash_op(struct mic_flash_op *desc, uint32_t *percent,
                          int *status, int *ext_status)
{
    struct mic_flash_status_info *stat;

    UT_INSTRUMENT_EVENT("FLASH1_INSTRUMENT_POLL_FAILURE_0",
                        errno = EINVAL);
    UT_INSTRUMENT_EVENT("FLASH1_INSTRUMENT_POLL_FAILURE_0", return -1);

    UT_INSTRUMENT_EVENT("FLASH1_INSTRUMENT_POLL_FAILURE_6",
                        errno = EINVAL);
    UT_INSTRUMENT_EVENT("FLASH1_INSTRUMENT_POLL_FAILURE_6", return -1);

    if (mic_get_flash_status_info(desc, &stat) != E_MIC_SUCCESS)
        return -1;

    (void)mic_get_progress(stat, percent);
    (void)mic_get_status(stat, status);
    (void)mic_get_ext_status(stat, ext_status);

    (void)mic_free_flash_status_info(stat);
    return 0;
}

/*
 * Poll for given flash operation. update_op is true if polling for an
 * update operation. Note that update operation may be SMC only in which
 * case flash_update is set to false.
 */
int poll_flash_op(struct mic_device *device, struct mic_flash_op *desc,
                  uint32_t delay, int timeout, int update_op, int flash_update)
{
    uint32_t percent_complete, prev_percent_complete;
    int status, ext_status, done;
    time_t start_time;
    char *str;
    int ret = -1;
    char *op;

    prev_percent_complete = UINT_MAX;
    if (check_flash_op(desc, &percent_complete, &status, &ext_status)
        != 0) {
        error_msg_start("%s: Failed to read flash status: %s: %s\n",
                        mic_get_device_name(device),
                        mic_get_error_string(), strerror(errno));
        return ret;
    }

#define WAIT_FOR_OP()    {                                      \
        int s, e;                                               \
        while (check_flash_op(desc, &percent_complete, &s, &e), \
               s != FLASH_OP_COMPLETED) ;                       \
}
    if (update_op) {
        if (flash_update) {
            if (cmdopts.line_mode_flag) {
                printf("Flash update:\n");
            } else {
                printf("%s: Flash update in progress\n",
                       mic_get_device_name(device));
            }
            op = "Flash update";
            UT_INSTRUMENT_EVENT(
                "FLASH1_INSTRUMENT_UPDATE_FAILURE_0",
                error_msg_start("Instrumented "
                                "Flash update error\n"));
            UT_INSTRUMENT_EVENT(
                "FLASH1_INSTRUMENT_UPDATE_FAILURE_0",
                WAIT_FOR_OP());
            UT_INSTRUMENT_EVENT(
                "FLASH1_INSTRUMENT_UPDATE_FAILURE_0",
                return -1);
        } else {
            if (cmdopts.smc_bootloader) {
                str = "boot-loader ";
                op = "SMC boot-loader update";
            } else {
                str = "";
                op = "SMC update";
            }
            if (cmdopts.line_mode_flag) {
                printf("SMC %supdate:\n", str);
            } else {
                printf("%s: SMC %supdate in progress\n",
                       mic_get_device_name(device), str);
            }
        }
    } else {
        if (cmdopts.line_mode_flag) {
            printf("Flash read:\n");
        } else {
            printf("%s: Flash read in progress\n",
                   mic_get_device_name(device));
        }
        op = "Flash read";
        UT_INSTRUMENT_EVENT("FLASH1_INSTRUMENT_READ_FAILURE_0",
                            error_msg_start(
                                "Instrumented Flash read error\n"));
        UT_INSTRUMENT_EVENT(
            "FLASH1_INSTRUMENT_READ_FAILURE_0", WAIT_FOR_OP());
        UT_INSTRUMENT_EVENT("FLASH1_INSTRUMENT_READ_FAILURE_0",
                            return -1);
    }

    done = 0;
    start_time = time(NULL);
    for (;; ) {
        /* If we started with flash update and now SMC update is in
         * progress then flash update is complete. Therefore switch
         * to SMC update status.
         */
        if (flash_update && SMC_OP(status)) {
            ret = FLASH_UPDATED;
            if (prev_percent_complete != 100) {
                if (cmdopts.line_mode_flag) {
                    printf("100\n");
                } else {
                    printf("%s: Flash update successful\n",
                           mic_get_device_name(device));
                }
            }
            flash_update = 0;
            if (cmdopts.line_mode_flag) {
                printf("SMC update:\n");
            } else {
                printf("%s: SMC update in progress\n",
                       mic_get_device_name(device));
            }
            op = "SMC update";
            start_time = time(NULL);
        }


        {
#define SET_STAT_ON_COMPLETION(stat)    {                       \
        int s, e;                                               \
        while (check_flash_op(desc, &percent_complete, &s, &e), \
               (s != FLASH_OP_COMPLETED) &&                     \
               (s != SMC_OP_COMPLETED)) ;                       \
        status = stat;                                          \
}


            UT_INSTRUMENT_EVENT("FLASH1_INSTRUMENT_POLL_FAILURE_1",
                                SET_STAT_ON_COMPLETION(
                                    FLASH_OP_IDLE));

            UT_INSTRUMENT_EVENT("FLASH1_INSTRUMENT_POLL_FAILURE_2",
                                SET_STAT_ON_COMPLETION(
                                    FLASH_OP_INVALID));

            UT_INSTRUMENT_EVENT("FLASH1_INSTRUMENT_POLL_FAILURE_3",
                                SET_STAT_ON_COMPLETION(
                                    FLASH_OP_FAILED));
            UT_INSTRUMENT_EVENT("FLASH1_INSTRUMENT_POLL_FAILURE_3",
                                ext_status = 0);

            UT_INSTRUMENT_EVENT("FLASH1_INSTRUMENT_POLL_FAILURE_4",
                                SET_STAT_ON_COMPLETION(
                                    FLASH_OP_AUTH_FAILED));
            UT_INSTRUMENT_EVENT("FLASH1_INSTRUMENT_POLL_FAILURE_4",
                                ext_status = 2);

            UT_INSTRUMENT_EVENT("FLASH1_INSTRUMENT_POLL_FAILURE_5",
                                SET_STAT_ON_COMPLETION(
                                    SMC_OP_AUTH_FAILED + 128));
            /*
             * TODO(eamaro): re-enable. uncrustify cannot parse this.
             * UT_INSTRUMENT_EVENT ("FLASH1_INSTRUMENT_POLL_FAILURE_7",
             *                   if (status ==
             *                       SMC_OP_IN_PROGRESS &&
             *                       percent_complete > 50)
             *                           SET_STAT_ON_COMPLETION(
             *                           SMC_OP_AUTH_FAILED));
             */
        }

        switch (status) {
        case FLASH_OP_IDLE:
            printf("\n");
            error_msg_start(
                "%s: Internal Error: No flash "
                "operation in progress\n",
                mic_get_device_name(
                    device));
            return ret;

        case FLASH_OP_INVALID:
            printf("\n");
            error_msg_start(
                "%s: Internal Error: Invalid "
                "flash operation requested\n",
                mic_get_device_name(
                    device));
            return ret;

        case FLASH_OP_IN_PROGRESS:
            break;

        case FLASH_OP_COMPLETED:
            percent_complete = 100;
            done++;
            break;

        case FLASH_OP_FAILED:
            printf("\n");
            error_msg_start(
                "%s: Flash operation failed: %s\n",
                mic_get_device_name(
                    device),
                flash_status_str(
                    ext_status));
            return ret;

        case FLASH_OP_AUTH_FAILED:
            printf("\n");
            error_msg_start(
                "%s: Flash operation not permitted: "
                "%s\n",
                mic_get_device_name(
                    device),
                flash_status_str(
                    ext_status));
            return ret;

        case SMC_OP_IN_PROGRESS:
            break;

        case SMC_OP_COMPLETED:
            percent_complete = 100;
            done++;
            break;

        case SMC_OP_FAILED:
            printf("\n");
            error_msg_start(
                "%s: SMC update failed: %s\n",
                mic_get_device_name(
                    device),
                smc_status_str(
                    ext_status));
            return ret;

        case SMC_OP_AUTH_FAILED:
            printf("\n");
            error_msg_start(
                "%s: SMC update not permitted: %s\n",
                mic_get_device_name(
                    device),
                smc_status_str(ext_status));
            return ret;

        default:
            error_msg_start(
                "%s: Unknown flash op status: %d\n",
                mic_get_device_name(
                    device),
                status);
            return ret;
        }
        if (percent_complete !=
            (prev_percent_complete)) {
            if (cmdopts.line_mode_flag) {
                printf("%u\n",
                       percent_complete);
            } else if (cmdopts.
                       verbose_flag) {
                printf(
                    "%s: %s: %u%%\n",
                    mic_get_device_name(
                        device),
                    op,
                    percent_complete);
            }
            fflush(stdout);
            prev_percent_complete =
                percent_complete;
        }
        if (done)
            break;

        sleep(delay);
        if (check_flash_op(desc,
                           &
                           percent_complete,
                           &status,
                           &ext_status) !=
            0) {
            error_msg_start(
                "%s: Failed to read flash "
                "status: %s: %s\n",
                mic_get_device_name(
                    device),
                mic_get_error_string(),
                strerror(
                    errno));
            return ret;
        }
        if (timeout != -1) {
            if ((start_time +
                 timeout) <= time(NULL))
                break;
        }
    }

    if (percent_complete !=
        (prev_percent_complete)) {
        if (cmdopts.line_mode_flag)
            printf("%u\n",
                   percent_complete);
        prev_percent_complete =
            percent_complete;
    }

    if (percent_complete < 100) {
        error_msg_start(
            "%s: Flash operation timed out due to a corrupted image file or an internal error.\n",
            mic_get_device_name(
                device));
        return ret;
    } else {
        if (!cmdopts.
            line_mode_flag) {
            if (update_op) {
                if (
                    flash_update) {
                    printf(
                        "%s: Flash update successful\n",
                        mic_get_device_name(
                            device));
                } else {
                    printf(
                        "%s: SMC %supdate successful\n",
                        mic_get_device_name(
                            device),
                        cmdopts
                        .
                        smc_bootloader
                        ?
                        "boot-loader "
                        :
                        "");
                }
            } else {
                printf(
                    "%s: Flash read successful\n",
                    mic_get_device_name(
                        device));
            }
        }
    }

    return 0;
}

int poll_maint_mode(struct
                    mic_device *
                    device,
                    uint32_t delay,
                    int timeout)
{
    int done = 0;
    time_t start_time;

    if (mic_in_maint_mode(
            device,
            &done) < 0) {
        error_msg_start(
            "%s: Failed to switch to maintenance "
            "mode: %s: %s\n",
            mic_get_device_name(
                device),
            mic_get_error_string(),
            strerror(
                errno));
        return -1;
    }

    start_time = time(NULL);
    while (!done) {
        sleep(delay);
        if (
            mic_in_maint_mode(
                device,
                &
                done) < 0) {
            error_msg_start(
                "%s: Failed to switch to maintenance "
                "mode: %s: %s\n",
                mic_get_device_name(
                    device),
                mic_get_error_string(),
                strerror(
                    errno));
            return -1;
        }
        if (timeout !=
            -1) {
            if ((
                    start_time
                    +
                    timeout)
                <=
                time(
                    NULL))
                break;
        }
    }

    if (!done) {
        error_msg_start(
            "%s: Switch to maintenance mode timed out\n",
            mic_get_device_name(
                device));
        return -1;
    }

    return 0;
}

int poll_ready_state(struct
                     mic_device *
                     device,
                     uint32_t
                     delay, int timeout)
{
    char post_code[NAME_MAX],
         prev_post_code[NAME_MAX];
    int done = 0;
    size_t size;
    time_t start_time;

    if (cmdopts.test_mode_flag
        || cmdopts.no_reset)
        return 0;

    if (mic_in_ready_state(
            device,
            &done) < 0) {
        error_msg_start(
            "%s: Failed to read card state: %s: %s\n",
            mic_get_device_name(
                device),
            mic_get_error_string(),
            strerror(
                errno));
        return -1;
    }

    if (cmdopts.line_mode_flag) {
        printf(
            "Resetting: POST:\n");
    } else {
        printf(
            "%s: Resetting\n",
            mic_get_device_name(
                device));
    }

    (void)strncpy(
        prev_post_code,
        "?",
        sizeof(
            prev_post_code));
    prev_post_code[sizeof(
                       prev_post_code)
                   - 1] = '\0';
    start_time = time(NULL);
    while (!done) {
        size =
            sizeof(
                post_code);
        if (
            mic_get_post_code(
                device,
                post_code,
                &
                size)
            !=
            E_MIC_SUCCESS) {
            error_msg_start(
                "%s: Failed to read post code: "
                "%s: %s\n",
                mic_get_device_name(
                    device),
                mic_get_error_string(),
                strerror(
                    errno));
            return -1;
        }
        if (strncmp(
                post_code,
                prev_post_code,
                sizeof(
                    post_code))
            != 0) {
            post_code[
                size
                -
                1]
                = '\0';
            if (
                cmdopts
                .
                line_mode_flag) {
                printf(
                    "%s \n",
                    post_code);
            } else if (
                cmdopts
                .
                verbose_flag) {
                printf(
                    "%s: POST: %s\n",
                    mic_get_device_name(
                        device),
                    post_code);
            }
            fflush(
                stdout);

            (void)
            strncpy(
                prev_post_code,
                post_code,
                sizeof(
                    prev_post_code));
        }

        sleep(delay);
        if (
            mic_in_ready_state(
                device,
                &
                done) < 0) {
            error_msg_start(
                "%s: Failed to read card state: "
                "%s: %s\n",
                mic_get_device_name(
                    device),
                mic_get_error_string(),
                strerror(
                    errno));
            return -1;
        }
        if (timeout !=
            -1) {
            if ((
                    start_time
                    +
                    timeout)
                <=
                time(
                    NULL))
                break;
        }
    }

    size = sizeof(post_code);
    if (mic_get_post_code(
            device,
            post_code,
            &size) !=
        E_MIC_SUCCESS) {
        error_msg_start(
            "%s: Failed to read post code: %s: %s\n",
            mic_get_device_name(
                device),
            mic_get_error_string(),
            strerror(
                errno));
        return -1;
    }
    if (strncmp(post_code,
                prev_post_code,
                sizeof(
                    post_code))
        != 0) {
        post_code[size -
                  1] =
            '\0';
        if (cmdopts.
            line_mode_flag) {
            printf(
                "%s \n",
                post_code);
        } else if (cmdopts
                   .
                   verbose_flag) {
            printf(
                "%s: POST: %s\n",
                mic_get_device_name(
                    device),
                post_code);
        }
        fflush(stdout);
        (void)strncpy(
            prev_post_code,
            post_code,
            sizeof(
                prev_post_code));
    }
    if (!(cmdopts.
          line_mode_flag ||
          cmdopts.verbose_flag))
        printf("\n");

    if (!done) {
        error_msg_start(
            "%s: Switch to ready state timed out\n",
            mic_get_device_name(
                device));
        return -1;
    } else {
        if (cmdopts.
            line_mode_flag) {
            printf(
                "%s: Reset complete\n",
                mic_get_device_name(
                    device));
        }
    }

    return 0;
}

int flash_file_version(const char
                       *fname,
                       void *buf,
                       char *flash,
                       size_t size)
{
    struct failsafe_info *fs;
    struct flash_desc *hd;
    off_t offs;

    fs =
        (struct
         failsafe_info *)
        buf;
    if ((fs->fsi_magic1 ==
         FS_MAGIC1) &&
        (fs->fsi_magic2 ==
         FS_MAGIC2)) {
        /* Make sure that it's a valid internal image */
        hd =
            (struct
             flash_desc
             *)
            (((
                  unsigned
                  char
                  *)
              buf) +
             FILE_CSS_HEADER_OFFS);

        if (hd->fd_type !=
            DESC_CSS) {
            error_msg_start(
                "%s: Not valid flash image\n",
                fname);
            return
                OTHER_ERR;
        }
        offs = 0;
    } else {
        /* Check the validity of external image */
        hd =
            (struct
             flash_desc
             *)buf;
        if (hd->fd_type !=
            DESC_CSS) {
            error_msg_start(
                "%s: Not valid flash image\n",
                fname);
            return
                OTHER_ERR;
        }
        offs =
            CSS_HEADER_SIZE;
    }

#define VERSION_OFFS    (0xff0)
    offs += VERSION_OFFS;

    strncpy(flash,
            BUF_OFFS(buf,
                     offs),
            size);
    flash[size - 1] = '\0';

    return 0;
}

int check_smc_bootloader_image(char* image_path,
			       int devnum)
{
    char *p;
    struct mic_device *mdh;
    char stepping[NAME_MAX];
    size_t size =
        sizeof(stepping);
    size_t path_len;

    if(!image_path){
        image_path = flash_dir;
    }

    /* Make sure that the place holder is unique in regex and not
     * a regex meta-character. It'll be replaced by lower case major "
     * stepping - a, b or c.
     */
#define STEPPING_PLACE_HOLDER    "X"
    char regex_str[] =
        ".*SMC_Bootloader.*\\.css_.*"
        STEPPING_PLACE_HOLDER
        ".*";
    regex_t regex;
    char major_stepping = '\0';
    int i, n, ret;
    struct dirent **namelist;
    struct mic_processor_info
    *pinfo;

    smc_bl = NULL;

    if (mic_open_device(&mdh,
                        devnum)
        != E_MIC_SUCCESS) {
        error_msg_start(
            "failed to open mic'%d': %s: %s\n",
            devnum,
            mic_get_error_string(),
            strerror(
                errno));
        return -1;
    }

    if (mic_get_processor_info(
            mdh,
            &pinfo) !=
        E_MIC_SUCCESS) {
        error_msg_start(
            "%s: failed to get processor info: "
            "%s: %s\n",
            mic_get_device_name(
                mdh),
            mic_get_error_string(),
            strerror(
                errno));
        (void)
        mic_close_device(
            mdh);
        return -1;
    }

    if (
        mic_get_processor_stepping(
            pinfo,
            stepping,
            &
            size)
        != E_MIC_SUCCESS) {
        error_msg_start(
            "%s: failed to get stepping: %s: %s\n",
            mic_get_device_name(
                mdh),
            mic_get_error_string(),
            strerror(
                errno));
        (void)
        mic_free_processor_info(
            pinfo);
        (void)
        mic_close_device(
            mdh);
        return -1;
    }

    (void)
    mic_free_processor_info(
        pinfo);
    (void)mic_close_device(mdh);

    if (size < 1) {
        error_msg_start(
            "%s: Invalid stepping ID: %s\n",
            mic_get_device_name(
                mdh),
            stepping);
        return -1;
    }

    p =
        strchr(regex_str,
               STEPPING_PLACE_HOLDER
               [0]);
    ASSERT(p != NULL);

    *p = major_stepping =
             tolower(
                 stepping
                 [0]);

    ret =
        regcomp(&regex,
                regex_str,
                REG_EXTENDED);
    ASSERT(ret == 0);

    ret = 0;
    n =
        scandir(image_path,
                &namelist,
                0,
                alphasort);
    if (n < 0) {
        error_msg_start(
            "mic%u: %s: scandir: %s\n",
            devnum,
            image_path,
            strerror(
                errno));
        return -1;
    }

    for (i = n - 1;
         i >= 0;
         i--) {
        if (regexec(&regex,
                    namelist
                    [i]->
                    d_name,
                    0, NULL, 0) == 0)
            break;
    }

    path_len =
        strlen(image_path);
    if (i >= 0) {
        smc_bl =
            malloc(
                path_len
                +
                strlen(
                    namelist
                    [
                        i]->d_name) + 2);
        if (smc_bl ==
            NULL) {
            error_msg_start(
                "mic%u: Out of memory\n",
                devnum);
            ret = -1;
            goto
            free_scanned;
        }
        sprintf(smc_bl,
                "%s/%s",
                image_path,
                namelist[i
                ]->d_name);
    } else {
        error_msg_start(
            "mic%u: No compatible SMC boot-loader image "
            "found\n",
            devnum);
        ret = -1;
        goto free_scanned;
    }

free_scanned:
    for (i = 0; i < n; i++)
        free(namelist[i]);
    free(namelist);

    return ret;
}

inline void error_msg_start(char *
                            format,
                            ...)
{
    va_list args;

    va_start(args, format);

    if (cmdopts.line_mode_flag) {
        fprintf(stdout,
                "ERROR: %s: ",
                progname);
        vfprintf(stdout,
                 format,
                 args);
    } else {
        fprintf(stderr,
                "%s: ",
                progname);
        vfprintf(stderr,
                 format,
                 args);
    }

    va_end(args);
}

inline void error_msg_cont(char *
                           format,
                           ...)
{
    va_list args;

    va_start(args, format);

    if (cmdopts.line_mode_flag) {
        vfprintf(stdout,
                 format,
                 args);
    } else {
        vfprintf(stderr,
                 format,
                 args);
    }

    va_end(args);
}
