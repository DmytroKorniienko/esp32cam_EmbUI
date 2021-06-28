#ifndef __FTP_SERVER
#define __FTP_SERVER

#include <FS.h>
#ifdef USE_LittleFS
  #define SPIFFS LITTLEFS
  #include <LITTLEFS.h> 
#else
  #include <SPIFFS.h>
#endif 

#include <FTPServer.h>
#include <LittleFS.h>
#include <FTPServer.h>

extern void ftp_setup(void);
extern void ftp_loop(void);
extern FTPServer ftpSrv;

#endif
