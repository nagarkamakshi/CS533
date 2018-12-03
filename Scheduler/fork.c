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

#define NR_SECTORS      128		//Max number of sectors, used to limit the sectors the child processes can access.
#define SECTOR_SIZE     512
#define NCPUS			8		//Number of CPUS of the system

#define DEVICE_NAME "/dev/CS533_myblock"
#define MY_BLOCK_MAJOR  "240"
#define MY_BLOCK_MINOR  "0"

int  main(int argc, char *argv[])
{
	bool
    int number_forks;
	int cpu_input;
	int ncpu = NCPUS;
	bool cpu_aff[NCPUS];
    int i;
    char data[10000];
    char fname[15];
    int split_size,ipos,epos;

    pid_t cPid;			//PID of process whose cpu affinity is to be set to the cpu set
    cpu_set_t mask;		//the CPU set
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
  
	//Init value for NCPUS all to false
	for(i = 0; i < ncpu; ++i)
	{
		cpu_aff[i] = false;
	}
  
    // calculating the size of the file 
    long int file_size = ftell(fp); 
    printf("Size of the file:%ld\n",file_size);
    
    printf("Enter no.of forks needed:");
    scanf("%d",&number_forks);

	//Prompt for selecting CPUs
	do{
		printf("Enter CPU to bind to (0-%d), Enter -1 when finished:", ncpu-1)
		scanf("%d",&cpu_input);
		
		if(cpu_input == -1)
			continue;

		if(cpu_input < -1 && cpu_input > ncpu-1)
		{
			printf("Invalid CPU number entered\n Should be between 0-%d\n", ncpu-1);
			continue;
		}
		
		//Switch cpu bind selection value
		cpu_aff[cpu_input] = !cpu_aff[cpu_input];
		printf("CPU %d bind, set to %s", cpu_input, cpu_aff[cpu_input] ? "true\n" : "false\n");
		
	}while(cpu_input != -1);
	
	//Check whether all CPU bind values set to false, if so, terminate program
	cpu_input = 0;
	for(i = 0; i < ncpu; ++i)
	{
		if(cpu_aff[i] == true)
			cpu_input += 1;
	}
	if(cpu_input == 0)
	{
		printf("All CPU was set to not bind. Terminating program.");
		return 0;
	}
	//cpu_input is now number of cpus in the set, used for setaffinity later on
	
	//init CPU set according to the bind values
	CPU_ZERO(&mask);
	for(i = 0; i < ncpu; ++i)
	{
		if(cpu_aff[i] == true)
		{
			CPU_SET(i, &mask);
			printf("Mask value set 0x%x", mask);
		}
	}
    
	
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
		cPid = fork();
        if(cPid==0)
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
			//Parent Only, set CPU affinity of cPid
            sched_setaffinity(cPid, cpu_input, &mask);
        }
    }
	//Parent wait for child proc to finish...
    for(i=0;i<number_forks;i++)  
	    wait(NULL);
	
	return 0;
}

