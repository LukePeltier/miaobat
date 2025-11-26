# miaobat

Fast, lightweight battery monitor for Angry Miao INFINITY 8K Mouse with support for both mouse and spare battery (in charging dock).

## Features

- **17KB binary** - Minimal resource usage
- **<100ms execution time** - Fast polling
- **Dual battery support** - Monitor both mouse and spare battery
- **Direct hidraw access** - Uses Linux kernel HID interface via ioctl
- **Waybar integration** - Ready-to-use status bar module

## Installation

### 1. Compile the Program

```bash
make
```

This creates the `read_battery` executable (~17KB).

### 2. Install udev Rule (Required)

Grant permission to access the HID device without root:

```bash
sudo cp udev/99-angry-miao-mouse.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger
```

**Unplug and replug your mouse** (or reboot) for the rule to take effect.

### 3. Test

```bash
./read_battery -b
```

Expected output: `100 50` (mouse battery, then spare battery)

## Usage

### Command Line

```bash
# Mouse battery only (default)
./read_battery
# Output: 100

# Spare battery only
./read_battery -s
# Output: 50

# Both batteries
./read_battery -b
# Output: 100 50

# Verbose mode (for debugging)
./read_battery -v
```

## Battery Indices

The HID report contains battery data at these byte positions:
- **Byte 2**: Mouse battery percentage (0-100)
- **Byte 10**: Spare battery percentage (0-100)

## Technical Details

### Performance

| Metric | Value |
|--------|-------|
| Binary size | 17 KB |
| Execution time | ~100ms |
| Memory usage | <1 MB |
| Dependencies | None (libc only) |

### How It Works

1. Scans `/sys/class/hidraw/*/device/uevent` for Angry Miao mouse (VID:PID = 3151:5007, interface 2)
2. Opens `/dev/hidraw*` device with `O_RDWR`
3. Sends HID feature report initialization (report ID 0x00, command 0xF7)
4. Reads HID feature report 0xF7 (65 bytes)
5. Extracts battery percentages from bytes 2 and 10

## Files

- `read_battery.c` - Main C source code
- `Makefile` - Build configuration
- `read_battery` - Compiled binary (after `make`)
- `waybar-mouse-battery.sh` - Waybar wrapper script
- `99-angry-miao-mouse.rules` - udev rule for permissions
- `waybar-config-example.json` - Example waybar configuration

## Troubleshooting

### Permission denied

Make sure the udev rule is installed and the mouse has been replugged:

```bash
ls -l /dev/hidraw* | grep 3151
```

You should see `crw-rw-rw-` permissions on the relevant hidraw device.

### Mouse not found

Check if the mouse is connected and detected:

```bash
lsusb | grep 3151
./read_battery -v
```

### Compilation errors

Make sure you have gcc and kernel headers:

```bash
sudo pacman -S gcc linux-headers  # Arch
sudo apt install gcc linux-headers-$(uname -r)  # Debian/Ubuntu
```

## Optional: System-wide Installation

```bash
sudo make install
```

This installs to `/usr/local/bin/read_battery`.

Then update `waybar-mouse-battery.sh` to use `read_battery` instead of the full path.

## Uninstall

```bash
sudo make uninstall
sudo rm /etc/udev/rules.d/99-angry-miao-mouse.rules
```
