//Operations (Open, Read, Write, Close) test for block device
//Seek to first sector, write test string, seek back to the first sector, then read and compare
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#define NR_SECTORS	128
#define SECTOR_SIZE	512

#define DEVICE_NAME	"/dev/CS533_myblock"
#define MY_BLOCK_MAJOR	"240"
#define MY_BLOCK_MINOR	"0"

int main(void)
{
	int fd;
	size_t i;
	int back_errno;
	unsigned char test_string[] = "CS533BlockDeviceDriverTest";
	unsigned char test_string_copy[27]";
	
	printf("mknod " DEVICE_NAME " b " MY_BLOCK_MAJOR " " MY_BLOCK_MINOR "\n");
	system("mknod " DEVICE_NAME " b " MY_BLOCK_MAJOR " " MY_BLOCK_MINOR "\n");
	sleep(1);

	fd = open(DEVICE_NAME, O_RDWR);
	if (fd < 0) {
		printf("Error: open failed.\n");
		return 1;
	}
	printf("Device should be opened now.\n");

	lseek(fd, 0, SEEK_SET);
	write(fd, test_string, strlen(test_string);
	printf("Test string should be written into device.\n")

	fsync(fd);

	lseek(fd, 0, SEEK_SET);
	read(fd, test_string_copy, strlen(test_string));
	printf("Test string should be read from device into the copy.\n");

	printf("Comparing test string and the data in the first sector.\n");
	if (memcmp(test_string, test_string_copy, strlen(test_string)) == 0)
		printf("passed, they match\n");
	else
		printf("failed, they don't match\n");
	
	close(fd);
	printf("Device should be closed now.\n");
	
	sleep(1);
	printf("rmmod " MODULE_NAME "\n");
	system("rmmod " MODULE_NAME "\n");

	return 0;
}
