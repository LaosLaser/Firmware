// Simple TFTP Server based on UDP echo server
// files are written to internal /local/ storage
#include "mbed.h"
#include "cmsis_os.h"
#include "pins.h"
#include "global.h"
#include "ConfigFile.h"
#include "EthernetInterface.h"
#include "laosfilesystem.h"
#include "LaosMotion.h"

enum TFTPState { idle, receiving } tftpState = idle;

void test_sd();
void tftpthread(void const *args);
void diskiothread(void const *args);
void diskiothread2(void const *args);
void sd_writefile(char *filename);
void sd_readfile(char *filename);
LaosMotion *mot;

LocalFileSystem local("local");   //File System
LaosFileSystem sd(p11, p12, p13, p14, "sd");
Serial        pc(USBTX, USBRX);

EthernetInterface eth;
UDPSocket server;
// Config
GlobalConfig *cfg;

// osThreadDef(diskiothread2, osPriorityNormal, DEFAULT_STACK_SIZE*3);
osThreadDef(tftpthread, osPriorityNormal, DEFAULT_STACK_SIZE*2);

const int DATA_SIZE = 256;
extern void plan_get_current_position_xyz(float *x, float *y, float *z);

int main (void) {
    extern LaosFileSystem sd;
    extern DigitalOut led3;

    test_sd();
    if (SDcheckFirmware()) mbed_reset();
    cfg =  new GlobalConfig("config.txt");
    printf("READ CONFIG\n\r");
    led3  = !led3;

    osThreadCreate(osThread(tftpthread), NULL);

    mot = new LaosMotion();
    if ( cfg->autohome )
    {
        printf("WAIT FOR COVER...\n");
        wait(1);

        // Start homing
        //if ( cfg->waitforstart ) 
        while ( !mot->isStart() );
        printf("HOME...\n");

        // NO MACHINE, SKIP THIS AT HOME
        mot->home(cfg->xhome,cfg->yhome, cfg->zhome);
        printf("HOME DONE. (%d,%d, %d)\n",cfg->xhome,cfg->yhome,cfg->zhome);
    }
    else
        printf("Homing skipped: %d\n", cfg->autohome);

    // osThreadCreate(osThread(diskiothread2), NULL);

    printf("READY\n\r");
    while (true) {
        osDelay(2500);
        led3  = !led3;
        printf(".");
    }
}

void test_sd() {
  extern LaosFileSystem sd;
  printf("TEST SD...\n");
  char testfile[] = "test.txt";
  FILE *fp = NULL;
  int cnt = 0;
  while (( fp == NULL ) && ( cnt < 3 )) 
  {
    osDelay(100);
    fp = sd.openfile(testfile, "wb");
  } 
  if ( fp == NULL )
  {
    mbed_reset();
  }
  else
  {
    printf("SD: READY...\n");
    fclose(fp);
    removefile(testfile);
  }
}

void tftpthread(void const *args) {
    const int TFTP_SERVER_PORT = 69;
    const int BUFFER_SIZE = 592;
    eth.init(); //Use DHCP
    eth.connect();
    server.bind(TFTP_SERVER_PORT);
    printf("TFTP Server listening on IP Address %s port %d\n\r", eth.getIPAddress(), TFTP_SERVER_PORT);
    server.set_blocking(false,1); // Set non-blocking

    extern LaosFileSystem sd;
    char buffer[BUFFER_SIZE] = {0};
    char ack[4]; ack[0] = 0x00; ack[1] = 0x04; ack[2] = 0x00; ack[3] = 0x00;
    char filename[32], mode[6], errmsg[32];
    unsigned int cnt = 0;
    Endpoint client;
    FILE *fp = NULL;
    short int timeout = 0;

    while (true) {
        int n = server.receiveFrom(client, buffer, sizeof(buffer));
        if (n > 0) {
            if (cnt>603235) cnt = 0;    // package count is only 2 bytes in TFTP
            ack[2] = cnt >> 8;          // store current count in ACK for sending
            ack[3] = cnt & 255;         //
            if (tftpState == idle) {
                if (buffer[1] == 0x02) {    // 0x02 means "request to write file"
                    printf("Received WRQ, %d bytes\n\r", n);
                    snprintf(filename, 32, "%s", &buffer[2]); // filename starts at byte 2
                    snprintf(mode, 6, "%s", &buffer[3+strlen(filename)]); // mode is after filename
                    printf("Received WRQ for file %s, mode %s.\n\r", filename, mode);
                    if (strcmp(mode, "octet") == 0) { // we only support octet/binary mode!
                        tftpState = receiving;
                        fp = sd.openfile(filename, "wb");
                        if (fp == NULL) {
                            printf("Error opening file %s\n\r", filename);
                            snprintf(errmsg, 32, "%c%c%c%cError opening file %s",ack[0], 0x05, ack[2], ack[3], filename);
                            server.sendTo(client, errmsg, strlen(errmsg));
                            tftpState  = idle;
                            led2 = !led2;
                        } else {
                            printf("Receiving file opened\n\r");
                            ack[1] = 0x04;
                            server.sendTo(client, ack, 4);
                            led2 = !led2;
                            cnt++;
                            timeout=0;
                        }
                    } else {
                        printf("No octet\n\r");
                        snprintf(errmsg, 32, "%c%c%c%cOnly octet files accepted",ack[0], 0x05, ack[2], ack[3]);
                        server.sendTo(client, errmsg, strlen(errmsg));
                        led2 = !led2;
                    }
                }
            } else if (tftpState == receiving) {
                if(buffer[1] == 0x03) {
                    if ((buffer[2] == ack[2]) && (buffer[3] == ack[3])) {
                        fwrite(&buffer[4], 1, n-4, fp);
                        server.sendTo(client,ack, 4);
                        led2 = !led2;
                        cnt++;
                        timeout = 0;
                        if (n < 516) {
                            printf("Received %d packages\n\r", cnt+1);
                            fclose(fp);
                            tftpState = idle;
                            cnt = 0;
                        }
                    } else {
                        printf("Order mismatch\n\r");
                        snprintf(errmsg, 32, "%c%c%c%cPacket order mismatch",ack[0], 0x05, ack[2], ack[3]);
                        server.sendTo(client, errmsg, strlen(errmsg));
                        led2 = !led2;
                    }
                } else {
                    printf("Unexpected packet type: %d\n\r",buffer[1]);
                    snprintf(errmsg, 32, "%c%c%c%cUnexpected packet type %d",ack[0], 0x05, ack[2], ack[3], buffer[1]);
                    server.sendTo(client, errmsg, strlen(errmsg));
                    led2 = !led2;
                }
            }
        } else {
            if (timeout>10) {
                if (tftpState == receiving) {
                    printf("Timeout!\n\r");
                    fclose(fp);
                    // removefile(filename);
                    tftpState = idle;
                } else {
                    timeout = 0;
                    led2 = !led2;
                }
            }
            timeout++;
            osDelay(500);
        }
    }
}

void diskiothread(void const *args) {
    extern TFTPState tftpState;
    extern DigitalOut led1;
    extern LaosMotion *mot;
    char *filename;
    float x, y, z = 0;
    if (tftpState == idle) {
        filename = getLaosFile();
        if ( filename != NULL ) {
            printf("STARTING FILE: %s", filename);
            mot->reset();
            plan_get_current_position_xyz(&x, &y, &z);
            FILE *in = sd.openfile(filename, "r");
            while (!feof(in))
            {
                while (!mot->ready() );
                mot->write(readint(in));
            }
            fclose(in);
            removefile(filename);
            printf("DONE!...\n");
            while (!mot->ready() );
            mot->moveTo(cfg->xrest, cfg->yrest, cfg->zrest);
        }
        osDelay(1000);
        led1 = !led1;
    }
}

void diskiothread2(void const *args) {
    extern TFTPState tftpState;
    extern DigitalOut led1;
    char file[] = "out.txt";
    char *filename;
    filename = file;
    while (true) {
        osDelay(2500);
        if (tftpState == idle) sd_writefile(filename);
        osDelay(1000);
        if (tftpState == idle) sd_readfile(filename);
        led1 = !led1;
    }
}

void sd_writefile(char *filename) {
    extern LaosFileSystem sd;
    FILE *f = sd.openfile(filename, "w");
    printf("SD: Writing ... ");
    if (f != NULL) {
        for (int i = 0; i < DATA_SIZE; i++)
            fprintf(f, "%c", rand() % 0XFF);
        printf("[OK]\r\n");
        fclose(f);
    } else {
        printf("[FAILED]\r\n");
    }
}

void sd_readfile(char *filename) {
    extern LaosFileSystem sd;
    uint8_t data;
    printf("SD: Reading ...");
    FILE *f = sd.openfile(filename, "r");
    if (f != NULL) {
        while (! feof(f)) {
            data = fgetc(f);
            if (data == '\t') printf("TAB");
        }
        printf("[OK]\r\n");
        fclose(f);
    } else {
        printf("[FAILED]\r\n");
    }
}
