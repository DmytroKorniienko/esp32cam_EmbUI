#include "ftpSrv.h"

FTPServer ftpSrv(SPIFFS); // construct with SPIFFS

void ftp_setup(void){
 
  /////FTP Setup, ensure SPIFFS is started before ftp;  /////////
  if (SPIFFS.begin()) {
      //LOG(println, F("FS opened!"));
      ftpSrv.begin(F("esp32"),F("esp32"));    //username, password for ftp
  }    
}

void ftp_loop(void){
  ftpSrv.handleFTP();        //make sure in loop you call handleFTP()!!  
}