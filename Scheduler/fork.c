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
#define MODULE_NAME     "ram-disk"
#define MY_BLOCK_MAJOR  "240"
#define MY_BLOCK_MINOR  "0"

#define max_elem_value(elem)    \
        (1 << 8*sizeof(elem))

static unsigned char buffer[SECTOR_SIZE];
static unsigned char buffer_copy[SECTOR_SIZE];


int  main(int argc, char *argv[])
{
    int number_forks;
    int i,c;
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

    split_size=(int)(file_size/number_forks);
    printf("split size %d\n",split_size);
    pid_t  pid;
    FILE *files[number_forks];
    
    for(i=0;i<number_forks;i++)
    {
        int sector = i;
        int fd;
        fd = open(DEVICE_NAME, O_RDWR);
        int b;

        if(fork()==0)
        {
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
	        sprintf(fname,"file%d.txt",i);
	        printf("File created is %s",fname);
	        FILE *f = fopen(fname,"w");
	        if(f==NULL)
	        {
		        printf("Error!");
		        exit(1);
	        }
            fprintf(f,"%s",data);
	        fclose(f);

            for(;;) 
	        {
                for (b = 0; b < sizeof(buffer) / sizeof(buffer[0]); b++)
                   buffer[b] = rand() % max_elem_value(buffer[0]);
                lseek(fd, sector * SECTOR_SIZE, SEEK_SET);
                write(fd, buffer, sizeof(buffer));
                fsync(fd);
                lseek(fd, sector * SECTOR_SIZE, SEEK_SET);
                read(fd, buffer_copy, sizeof(buffer_copy));
                printf("[%d]test sector %3d ... ", getpid(), sector);
                if (memcmp(buffer, buffer_copy, sizeof(buffer_copy)) == 0)
                    printf("passed\n");
                else
                    printf("failed\n");
            }
             exit(0);
        }
        else 
		{
            CPU_ZERO(&mask);
            CPU_SET(1+i, &mask);
            printf("Mask valu set 0x%x", mask);
            sched_setaffinity(cPid, sizeof(cpu_set_t), &mask);
        }
    }
    for(i=0;i<number_forks;i++)  
	    wait(NULL);
}

