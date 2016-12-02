#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define RT_DEVICE_FILE          "/dev/rdm0"
#define RT_RDM_CMD_WRITE        0x6B02
#define RT_RDM_CMD_READ         0x6B03

static int ra_reg_open()
{
    return open("/dev/rdm0", O_RDONLY);
}

static int ra_reg_write(int fd, int offset, int value)
{
    int method = RT_RDM_CMD_WRITE | (offset << 16);

    if (ioctl(fd, method, &value) < 0)
        return -1;

    return 0;
}

static int ra_reg_read(int fd, int offset)
{
    if (ioctl(fd, RT_RDM_CMD_READ, &offset) < 0)
        return -1;

    return offset;
}

static int ra_reg_mod_bits(int fd, int offset, int data, int  start_bit, int len)
{
    int Mask = 0;
    int Value;
    int i;

    for (i = 0; i < len; i++) {
        Mask |= 1 << (start_bit + i);
    }

    if ((Value = ra_reg_read(fd, offset)) < 0)
        return -1;

    Value &= ~Mask;
    Value |= (data << start_bit) & Mask;;

    return ra_reg_write(fd, offset, Value);
}

int reset_ble_module(int sleep_sec)
{
    int fd;

    if ((fd = ra_reg_open()) < 0)
        return -1;

    /* BLE_RESET PIN (GPIO 37) - GPIO_DATA_1, bit5 */

    if (ra_reg_mod_bits(fd, 0x624, 0, 5, 1) < 0)
    {
        close(fd);
        return -2;
    }

    sleep(sleep_sec);

    if (ra_reg_mod_bits(fd, 0x624, 1, 5, 1) < 0)
    {
        close(fd);
        return -3;
    }

    close(fd);
    return 0;
}


/*
 * This API disable the Serial bootloader backdoor 
 * using GPIO 43 signal pull low 
 */
int ti_ble_disable_sbl_backdoor(int sleep_sec)
{
        int fd;

        if ((fd = ra_reg_open()) < 0)
        	return -1;
        
        /* BLE_SERIAL_BOOTLOADER_DISABLE PIN (GPIO 43) - GPIO_DATA_1, bit11 */

        if (ra_reg_mod_bits(fd, 0x624, 0, 11, 1) < 0)
        {
                close(fd);
                return -2;
        }

        sleep(sleep_sec);

        close(fd);
        return 0;
}
/*
 * This API  enable the Serial bootloader backdoor 
 * using GPIO 43 signal pull high 
 */
int ti_ble_enable_sbl_backdoor(int sleep_sec)
{
        int fd;

        if ((fd = ra_reg_open()) < 0)
                return -1;

        /* BLE_SERIAL_BOOTLOADER_ENABLE PIN (GPIO 43) - GPIO_DATA_1, bit11 */

        if (ra_reg_mod_bits(fd, 0x624, 1, 11, 1) < 0)
        {
                close(fd);
                return -3;
        }

        sleep(sleep_sec);

        close(fd);
        return 0;
}




