/*******************************************/
/************   Hasan Bilgin        ********/
/***********    121044077           ********/
/***********    CSE 244 MIDTERM     ********/
/***********    IntegralGen.c       ********/
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
#include <matheval.h>

#define FIFO_PERMS (S_IRWXU | S_IWGRP| S_IWOTH)
#define STRING_SIZE 4096

void interruptHandler (int signo);
double trapezoidal (char* pcEquation,int iTimeInterval,double dLowerBound,double dUpperBound); 

char pcFunctions[6][STRING_SIZE];
pid_t pAllClients[256];

int main ( int argc ,char** argv) {
    int i=0;
    if (argc != 3)  {
        fprintf(stderr,"USAGE : ");
        fprintf(stderr,"./IntegralGen  -<resolution>  -<max # of clients>\n");
        return -1;
    }
    char pcOperation[STRING_SIZE] = "";
    for ( i = 1 ; i < strlen(argv[1]) ; ++i)
        pcOperation[i-1] = argv[1][i];

    pcOperation[strlen(argv[1]) - 1] = '\0';
    int iResolution = atoi(pcOperation);
    for ( i = 1 ; i < strlen(argv[2]) ; ++i)
        pcOperation[i-1] = argv[2][i];

    pcOperation[strlen(argv[2]) - 1] = '\0';
    int iMaxNumberOfClient = atoi(pcOperation);

    FILE* fpFunc;
    // Fonksiyon dosyalarini pcFunctions arrayine koyar
    for ( i = 1 ; i < 7 ; ++i)  {
        char pcFileName[STRING_SIZE]="";
        sprintf (pcFileName,"f%d.txt",i);
        fpFunc = fopen(pcFileName,"r");
        if (fpFunc == NULL) {
            perror ("Dosya Acilamadi");
            exit(1);
        }
        int  j = 0;
        while (!feof(fpFunc))
            pcFunctions[i-1][j++] = fgetc(fpFunc);

        pcFunctions[i-1][j - 2] = '\0';
        fclose(fpFunc);    
    }

    fpFunc = fopen("IntegralGen.log","w");
    if (fpFunc == NULL) {
        perror ("Dosya Acilamadi");
        exit(1);
    }

    /* Clientlari servera baglamak icin main fifo olusturur */
    if ((mkfifo("IntegralServer", 0666) == -1) && (errno != EEXIST)) {
       perror("Server FIFO olusturamadi");
       return -1;
    }
    printf("RUNNING...\n");

    struct timeval  tStart;
    time_t tTemp;
    gettimeofday(&tStart, NULL);
    /*  Baslangic zamani milisaniye cinsinden   */
    double dStartTime =(tStart.tv_sec) * 1000 + (tStart.tv_usec) / 1000 ;
    time(&tTemp);
    fprintf(fpFunc, "Server baslangic zamani : %s\n", ctime(&tTemp));
    fflush(fpFunc);
    /*  Olusturulan FIFO calistirilir   */
    int iIntegralSFD = 0;
    if (( iIntegralSFD = open("IntegralServer",O_RDONLY)) == -1 ) {
        perror ("IntegralServer FIFO'su acilamadi");
        return -1;
    }

    int iConnectedClientNumber=0; 
    pid_t process;
/*--------------------------------------------------------------------*/
    while(1)
    {
        /*  Ctrl + C sinyali gelirse yakalar    */
    	signal (SIGINT,interruptHandler);
    	pid_t pClientPID;
        /*  Main argumani olarak gelen maximum client sayisina ulasilip
            ulasilmadigina bakar   */
    	if (iConnectedClientNumber < iMaxNumberOfClient)   {
            /*  Herhangi bir client in servera baglanirsa calisir  */
            if( read(iIntegralSFD, &pClientPID, sizeof(pid_t)) != 0 ){
                /*  Bagli olan client sayisini arttirir */
                pAllClients[iConnectedClientNumber] = pClientPID;
                iConnectedClientNumber++;
                /* Client e bilgi yollamak icin acilacak FIFO nun adini pcClientPID'ye atar */
                char pcClientPID[STRING_SIZE];
                sprintf(pcClientPID,"%ld",(long)pClientPID);
                    
                gettimeofday(&tStart, NULL);
                /*  Baslangic zamani milisaniye cinsinden   */
                double dClientTime =(tStart.tv_sec) * 1000 + (tStart.tv_usec) / 1000 ;
                time(&tTemp);
                fprintf(fpFunc, "%s pid'li client server ' a baglanis zamani : %s\n",pcClientPID,ctime(&tTemp));
                fflush(fpFunc);

                double dLowerBound = dClientTime - dStartTime ;    
                pid_t process;
                process = fork();

                if (process == -1)    {
                    perror("Fork Error !!");
                    exit(0);
                }
                // Server child */
                else if (process == 0)  {
                    int iF1,iF2,iTimeInterval,iClientRead;
                    /*  Client dan bilgileri almak icin FIFO'yu acar    */
                    if (( iClientRead = open(pcClientPID,O_RDONLY)) == -1 ) {
                        perror ("Client FIFO'su acilamadi");
                        return -1;
                    }
                    /* Client dan bilgileri okur    */
                    read(iClientRead,&iF1,sizeof(int));
                    read(iClientRead,&iF2,sizeof(int));
                    read(iClientRead,&iTimeInterval,sizeof(int));
                    read(iClientRead,pcOperation,STRING_SIZE);
                    close(iClientRead);
                    /*  Integrali alinacak denklemi olusturur   */
                    char pcEquation[STRING_SIZE] = "";
                    strcpy(pcEquation,pcFunctions[ iF1 - 1 ]);
                    strcat(pcEquation,pcOperation);
                    strcat(pcEquation,pcFunctions[ iF2 - 1 ]);
                    
                    int iClientWrite;
                    double dUpperBound = dLowerBound + iResolution;
                    while (1)    {
                        /*  Client a integralin sonucunu yollamak icin FIFO'yu acar */
                        if (( iClientWrite = open(pcClientPID,O_WRONLY)) == -1 ) {
                            perror ("Client FIFO'su acilamadi");
                            return -1;
                        }
                        /* Resolution kadar bekler */
                        usleep(iResolution*1000);
                        double dResult = 0;
                        dResult = trapezoidal (pcEquation,iResolution,dLowerBound,dUpperBound);
                        write (iClientWrite,&dResult,sizeof(double));
                        close(iClientWrite);
                        dLowerBound += iResolution;
                        dUpperBound += iResolution;
                    }
                        exit(0);
                    }
                    else    {
                        //Server parent
                    }
            }            
        }
    }
    return 0;
   
}
/*  Ctrl + C sinyali olusunca server i ve butun client lari kapatir */
void interruptHandler (int signo)   {
   	int i = 0;

    FILE* fp = fopen ("IntegralGen.log","a");
    if (fp == NULL)
        printf("Log File acilamadi\n");
    fprintf(fp, "Ctrl + C komutu verildi server kapandi.");
    fflush(fp);
    fclose(fp);
    fprintf(stderr, "Ctrl + C komutu verildi server kapandi.");
    unlink("IntegralServer");
 
    for ( i = 0 ; i < 256 ; ++i )
    	kill(pAllClients[i],SIGINT);
    exit(EXIT_FAILURE);
}
/*  Integrali trapezoidal   yontemi ile alir    */
/*  pcEquation integrali alinacak denklem   */
double trapezoidal (char* pcEquation,int iTimeInterval,double dLowerBound,double dUpperBound) {
    void *fn;
    fn = evaluator_create(pcEquation);
    int i = 0;
    double dH = (dUpperBound - dLowerBound) / iTimeInterval;
    double dS = 0;

    for ( i = 1 ; i < iTimeInterval - 1 ; ++i)
        dS = dS + evaluator_evaluate_x (fn,dLowerBound + i*dH);

    double dY = (evaluator_evaluate_x(fn,dLowerBound) +  evaluator_evaluate_x(fn,dUpperBound) + 2 * dS ) * dH / 2;
    evaluator_destroy(fn);
    return dY;
}
