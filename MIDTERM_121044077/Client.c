/*******************************************/
/************   Hasan Bilgin        ********/
/***********    121044077           ********/
/***********    CSE 244 MIDTERM     ********/
/***********    Client.c            ********/
/*******************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

#define FIFO_PERMS (S_IRWXU | S_IWGRP| S_IWOTH)
#define STRING_SIZE 1024

void interruptHandler (int signo);

int main ( int argc ,char** argv) {
    int i=0;
    if (argc != 5)  {
        fprintf(stderr,"USAGE : ");
        fprintf(stderr,"./client -<fi> -<fj> -<time interval> -<operation>\n");
        return -1;
    }
    signal(SIGINT,interruptHandler);
 
    /*  fi ve fj degerlerinin 1 ile 6 arasinda olup olmadigini kontrol eder*/
    if (argv[1][2] < '1' || argv[1][2] > '6' || argv[2][2] < '1' || argv[2][2] > '6')   {
        fprintf(stderr," Fonksiyonlarin dosya numarasi yanlis girildi\n");
        fprintf(stderr,"fi ve fj degerleri 1 ile 6 arasinda olmalidir\n");
        fprintf(stderr,"USAGE : ");
        fprintf(stderr,"./client <fi> <fj> <time interval> <operation>\n");
        return -1;
    }
    /*  Servera bilgileri yollamak icin kendi pid ile FIFO acar */
    char pcClientFIFO [STRING_SIZE];
    sprintf(pcClientFIFO,"%ld",(long)getpid());
    if ((mkfifo(pcClientFIFO, 0666) == -1) && (errno != EEXIST)) {
       perror("Server FIFO olusturamadi");
       return -1;
    }
    /*  main FIFO acilir    */
    int iIntegralSFD=0;
        if (( iIntegralSFD = open("IntegralServer",O_WRONLY)) == -1 ) {
        perror ("IntegralServer FIFO'su acilamadi");
        return -1;
    }

   /* Client pid sini servera yollar   */
    pid_t process = getpid();
    write (iIntegralSFD,&process, sizeof(pid_t));
    close(iIntegralSFD);
    
    /*  Servera hesaplama icin gerekli bilgiler yollanir    */
    int iClientWrite = 0;
    if (( iClientWrite= open(pcClientFIFO,O_WRONLY)) == -1 ) {
        perror ("Servera bilgi yazilamadi");
        return -1;
    }
    int iF1 = argv[1][2] - '0';
    write (iClientWrite,&iF1,sizeof(int));
    int iF2 = argv[2][2] - '0';
    write (iClientWrite,&iF2,sizeof(int));
    char pcOperation[STRING_SIZE] = "";
    for ( i = 1 ; i < strlen(argv[3]) ; ++i)
        pcOperation[i-1] = argv[3][i];
    pcOperation[strlen(argv[3]) - 1] = '\0';
    int iTimeInterval = atoi (pcOperation);
    write (iClientWrite,&iTimeInterval,sizeof(int));
    for ( i = 1 ; i < strlen(argv[4]) ; ++i)
        pcOperation[i-1] = argv[4][i];
    pcOperation[strlen(argv[4]) - 1];
    write (iClientWrite,pcOperation,STRING_SIZE);
    close (iClientWrite);
    
    char pcLog[STRING_SIZE];
    int iClientFIFO=0;
    /*  Serverin hesapladigi sonuclari acilir*/
    sprintf(pcLog,"%ld.log",(long)getpid());
    FILE* fp = fopen(pcLog,"w");
    while (1)   {
        if (( iClientFIFO= open(pcClientFIFO,O_RDONLY)) == -1 ) {
            perror ("Serverdan bilgi alinamadi");
            return -1;
        }
        double dResult = 0;
        ssize_t readingBytes;
        readingBytes = read (iClientFIFO,&dResult,sizeof(double));
        close(iClientFIFO);
        if (readingBytes > 0)   {
            fprintf(fp, "Result : %lf\n",dResult);
            fflush(fp);
        }
    }
    return 0;
}

void interruptHandler (int signo)   {
    fprintf(stderr,"Ctrl + C komutu verildi client kapandi\n");
    char pcClientFIFO[STRING_SIZE];
    sprintf(pcClientFIFO,"%ld.log",(long)getpid());
    FILE* fp = fopen (pcClientFIFO,"a");
    if (fp == NULL)
        printf("Log File acilamadi\n");
    fprintf(fp, "Ctrl + C komutu verildi client kapandi.");
    fflush(fp);
    fclose(fp);
    sprintf(pcClientFIFO,"%ld",(long)getpid());
    unlink (pcClientFIFO);
    exit(EXIT_FAILURE);
}
