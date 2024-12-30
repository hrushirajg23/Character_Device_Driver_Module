#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <wait.h>

#define DRIVER_NAME "/dev/eagle"

int main() {
    int fd = open(DRIVER_NAME, O_RDWR);
    if (fd == -1) {
        perror("Couldn't open device");
        return -1;
    }

    char str[] = "linux_device_drivers";
    ssize_t wret = write(fd, str, strlen(str));
    if (wret == -1) {
        switch (errno) {
            case EFAULT:
                printf("Address space issue\n");
                break;
            case EINVAL:
                printf("Invalid object\n");
                break;
            case EIO:
                printf("Low-level I/O error occurred\n");
                break;
            case EBADF:
                printf("Bad file descriptor\n");
                break;
            default:
                printf("Unknown error: %s\n", strerror(errno));
        }
        close(fd);
        return -1;
    } else {
        printf("Successfully wrote %ld bytes\n", wret);
    }

    ssize_t rret = read(fd, str, strlen(str) - 3);
    if (rret == -1) {
        perror("Error during read");
        close(fd);
        return -1;
    } else if (rret == 0) {
        printf("End of file reached\n");
    } else {
        str[rret] = '\0';  // Null-terminate the string
        printf("After reading: %s\n", str);
    }

    close(fd);
    return 0;
}
