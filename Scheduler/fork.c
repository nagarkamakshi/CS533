#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sched.h>

#define NR_SECTORS      128
#define SECTOR_SIZE     512


#define DEVICE_NAME "/dev/CS533_myblock"
#define MY_BLOCK_MAJOR  "240"
#define MY_BLOCK_MINOR  "0"

int  main(int argc, char *argv[])
{
    int number_forks;
    int i;
    char data[10000];
    char fname[15];
    int split_size,ipos,epos;

    pid_t cPid;
    cpu_set_t mask;
    printf("mknod " DEVICE_NAME " b " MY_BLOCK_MAJOR " " MY_BLOCK_MINOR "\n");
    system("mknod " DEVICE_NAME " b " MY_BLOCK_MAJOR " " MY_BLOCK_MINOR "\n");
    sleep(1);

    FILE *fp = fopen("CS533_fork.txt", "r");
    // checking if the file exist or not 
    if (fp == NULL) { 
        printf("File Not Found!\n"); 
        return -1; 
    } 
    fseek(fp, 0L, SEEK_END); 
  
    // calculating the size of the file 
    long int file_size = ftell(fp); 
    printf("Size of the file:%ld\n",file_size);
    
    printf("Enter no.of forks needed:");
    scanf("%d",&number_forks);

	//Split file into pieces
    split_size=(int)(file_size/number_forks);
    printf("split size %d\n",split_size);
    pid_t  pid;
    
	//Open the block device
	int fd;
    fd = open(DEVICE_NAME, O_RDWR);
	
    for(i=0;i<number_forks;i++)
    {
		//Limit sector number for the write to the device to NR of sectors defined in macro at file header
        int sector = i % NR_SECTORS;

        if(fork()==0)
        {
			//Child Only, Get their part of the split data and write to device
			//Get the split data
	        printf("From child, the i value: %d\n",i); 
	        printf("child pid %d from parent pid %d\n",getpid(),getppid());
            ipos=(i*split_size)+i;
	        printf("ipos:%d\n",ipos);
	        epos=(i+1)*split_size;
	        printf("epos:%d\n",epos);
	        fseek(fp, epos, SEEK_SET);
	        fseek(fp, -split_size, SEEK_CUR);
            fgets ( data, split_size, fp );
            printf("File data: %s\n", data);
	        
			//Write data to device
			lseek(fd, sector * SECTOR_SIZE, SEEK_SET);
			write(fd, data, split_size);
			fsync(fd);

            exit(0);
        }
        else 
		{
			//Parent Only, do the CPU set affinity
            CPU_ZERO(&mask);
            CPU_SET(1+i, &mask);
            printf("Mask valu set 0x%x", mask);
            sched_setaffinity(cPid, sizeof(cpu_set_t), &mask);
        }
    }
	//Parent wait for child proc to finish...
    for(i=0;i<number_forks;i++)  
	    wait(NULL);
	
	return 0;
}

