/*
 * read_battery.c - Read battery from Angry Miao INFINITY 8K Mouse
 *
 * Reads battery percentage via hidraw interface using direct ioctl calls.
 * Requires udev rule to grant access to /dev/hidraw device.
 *
 * Compile: gcc -O2 -Wall -o read_battery read_battery.c
 * Usage: read_battery [-v] [-m|-s|-b]
 *   -v: verbose output
 *   -m: mouse battery only (default)
 *   -s: spare battery only
 *   -b: both batteries (format: "mouse spare")
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/hidraw.h>

/* Angry Miao INFINITY 8K Mouse */
#define VENDOR_ID_PATTERN  "00003151:00005007"
#define INTERFACE_PATTERN  "input2"

/* HID Feature Report ioctls */
#ifndef HIDIOCSFEATURE
#define HIDIOCSFEATURE(len) _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x06, len)
#endif
#ifndef HIDIOCGFEATURE
#define HIDIOCGFEATURE(len) _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x07, len)
#endif

#define REPORT_SIZE 65
#define REPORT_ID   0xF7
#define MOUSE_BATTERY_INDEX  2
#define SPARE_BATTERY_INDEX  10

typedef enum {
    SHOW_MOUSE = 1,
    SHOW_SPARE = 2,
    SHOW_BOTH = 3
} show_mode_t;

static int verbose = 0;

/*
 * Find the hidraw device for Angry Miao mouse interface 2
 * Returns: device path (e.g., "/dev/hidraw2") or NULL if not found
 */
static char* find_hidraw_device(void) {
    DIR *dir;
    struct dirent *entry;
    static char device_path[512];
    char uevent_path[512];
    char line[256];
    FILE *fp;
    int found_vid = 0, found_pid = 0, found_if = 0;

    dir = opendir("/sys/class/hidraw");
    if (!dir) {
        if (verbose)
            perror("opendir /sys/class/hidraw");
        return NULL;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;

        snprintf(uevent_path, sizeof(uevent_path),
                 "/sys/class/hidraw/%s/device/uevent", entry->d_name);

        fp = fopen(uevent_path, "r");
        if (!fp)
            continue;

        found_vid = found_pid = found_if = 0;

        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, VENDOR_ID_PATTERN))
                found_vid = found_pid = 1;
            if (strstr(line, INTERFACE_PATTERN))
                found_if = 1;
        }
        fclose(fp);

        if (found_vid && found_pid && found_if) {
            snprintf(device_path, sizeof(device_path), "/dev/%s", entry->d_name);
            closedir(dir);
            if (verbose)
                fprintf(stderr, "Found device: %s\n", device_path);
            return device_path;
        }
    }

    closedir(dir);
    return NULL;
}

/*
 * Read battery percentages from the device
 * Returns: 0 on success, -1 on error
 * Output: mouse_bat and spare_bat are set to battery percentages (0-100)
 */
static int read_batteries(const char *device_path, int *mouse_bat, int *spare_bat) {
    int fd, res;
    unsigned char buf[REPORT_SIZE];

    /* Open hidraw device */
    fd = open(device_path, O_RDWR);
    if (fd < 0) {
        if (verbose)
            fprintf(stderr, "Error opening %s: %s\n", device_path, strerror(errno));
        return -1;
    }

    /* Prepare initialization feature report */
    memset(buf, 0, sizeof(buf));
    buf[0] = 0x00;  /* Report ID (0 for this device) */
    buf[1] = 0xF7;  /* Init command */

    /* Send initialization */
    res = ioctl(fd, HIDIOCSFEATURE(REPORT_SIZE), buf);
    if (res < 0) {
        if (verbose)
            fprintf(stderr, "HIDIOCSFEATURE failed: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    if (verbose)
        fprintf(stderr, "Sent init report\n");

    /* Small delay for device to respond */
    usleep(50000);  /* 50ms */

    /* Prepare to read feature report */
    memset(buf, 0, sizeof(buf));
    buf[0] = REPORT_ID;

    /* Get feature report */
    res = ioctl(fd, HIDIOCGFEATURE(REPORT_SIZE), buf);
    if (res < 0) {
        if (verbose)
            fprintf(stderr, "HIDIOCGFEATURE failed: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);

    if (verbose) {
        fprintf(stderr, "Got %d bytes:\n", res);
        for (int i = 0; i < res; i++) {
            fprintf(stderr, "%02x ", buf[i]);
            if ((i + 1) % 16 == 0)
                fprintf(stderr, "\n");
        }
        if (res % 16 != 0)
            fprintf(stderr, "\n");

        fprintf(stderr, "\nBattery candidates (values 1-100):\n");
        for (int i = 0; i < res; i++) {
            if (buf[i] >= 1 && buf[i] <= 100) {
                fprintf(stderr, "  byte[%2d] = %3d (0x%02x)\n", i, buf[i], buf[i]);
            }
        }
    }

    /* Extract battery percentages */
    *mouse_bat = buf[MOUSE_BATTERY_INDEX];
    *spare_bat = buf[SPARE_BATTERY_INDEX];

    return 0;
}

int main(int argc, char *argv[]) {
    char *device;
    int mouse_battery, spare_battery;
    show_mode_t show_mode = SHOW_MOUSE;  /* default */

    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-m") == 0) {
            show_mode = SHOW_MOUSE;
        } else if (strcmp(argv[i], "-s") == 0) {
            show_mode = SHOW_SPARE;
        } else if (strcmp(argv[i], "-b") == 0) {
            show_mode = SHOW_BOTH;
        } else {
            fprintf(stderr, "Usage: %s [-v] [-m|-s|-b]\n", argv[0]);
            fprintf(stderr, "  -v  verbose output\n");
            fprintf(stderr, "  -m  mouse battery only (default)\n");
            fprintf(stderr, "  -s  spare battery only\n");
            fprintf(stderr, "  -b  both batteries\n");
            return 1;
        }
    }

    /* Find device */
    device = find_hidraw_device();
    if (!device) {
        if (verbose)
            fprintf(stderr, "Mouse not found\n");
        return 1;
    }

    /* Read batteries */
    if (read_batteries(device, &mouse_battery, &spare_battery) < 0) {
        if (verbose)
            fprintf(stderr, "Failed to read battery\n");
        return 1;
    }

    /* Output battery percentage(s) based on mode */
    switch (show_mode) {
        case SHOW_MOUSE:
            printf("%d\n", mouse_battery);
            break;
        case SHOW_SPARE:
            printf("%d\n", spare_battery);
            break;
        case SHOW_BOTH:
            printf("%d %d\n", mouse_battery, spare_battery);
            break;
    }

    return 0;
}
