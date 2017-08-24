#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "esp_err.h"
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <sys/socket.h>
#include "freertos/FreeRTOS.h"
#include "sd.h"
#include "ftp.h"

#define TAG  "ftp_server"

#define FTP_TIME_OUT        5           // Disconnect client after 5 minutes of inactivity
/* the variable which can set with `make menuconfig` */
#define FTP_CTRL_PORT       CONFIG_COMMAMD_PORT          // Command port on wich server is listening
#define FTP_DATA_PORT_PASV  CONFIG_DATA_PORT       // Data port for passive mode
#define FTP_USER            CONFIG_FTP_USER
#define FTP_PASS            CONFIG_FTP_PASS

/* the command and parameter sent by client */
static char      command[5];
static char      *parameter;
static char      strBuf[120];
/* the directory at start is root directory: /sdcard */
volatile char    *current_dir = "/sdcard";
volatile char    *curr;

FIL* f;
struct tcp_pcb *pcb_cmd;
struct tcp_pcb *pcb_data;
int FIRSTPRINT = 1;

typedef enum {
    NOTCONNECTED = 0,
    USERREQUIRED = 1,
    PASSREQUIRED = 2,
    ACCEPTINGCMD = 3,
    RNTOREQUIRED = 4,
    STORRECEIVED = 5,
    LISTRECEIVED = 6,
    RETRRECEIVED = 7
} cmdStatus_t;

typedef enum {
    NODATACONN   = 0,
    NOCONNECTION = 1,
    CONNECTED    = 2,
} dataStatus_t;
/* status of command and data connection at start */
cmdStatus_t cmdStatus = NOTCONNECTED;
dataStatus_t dataStatus = NODATACONN;
void dataConnCreate(void);
bool   dataTypeBinary = false;
/* the file numbers */
int    numberOfFiles = 0;
int    listflag = 0;
int    retrievefalg = 0;

/* send the response of command*/
void sendCmdConn(const char *fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);
    vsprintf(strBuf, fmt, arg);
    strcat(strBuf, "\n");
    va_end(arg);
    printf("Response:  %s", strBuf);
    fflush(stdout);
    tcp_write(pcb_cmd, strBuf, strlen(strBuf), 1);
}

/* list the file in the directory */
void doListFiles(void)
{
    numberOfFiles = 0;
    struct dirent *pStResult = NULL;
    struct dirent *pStEntry = NULL;
    DIR *pDir;
    vfs_stat_t st;

    // if (!strcmp(current_dir, "/sdcard"))
    pDir = opendir("/sdcard");
    //  else
    //      pDir = opennextdir(current_dir);
    if (NULL == pDir) {
        printf("Opendir failed!\n");
        return ;
    }
    cmdStatus = LISTRECEIVED;

    if (dataStatus == CONNECTED) {
        pStEntry = (struct dirent*)malloc(60);

        while (! readdir_r(pDir, pStEntry, &pStResult) && pStResult != NULL) {
            printf("file'name is %s\n", pStEntry->d_name);
            vfs_stat(pStEntry->d_name, &st);
            printf("the size is %ld\n", st.st_size);
            if (st.st_size != 0)
                sprintf(strBuf, "-rw-r--r-- 2 ftp ftp              %ld Jan 14 09:31 %-30s\r\n", st.st_size, pStEntry->d_name);
            else
                sprintf(strBuf, "drwxr-xr-x 2 ftp ftp              %ld Jan 14 09:31 %-30s\r\n", st.st_size, pStEntry->d_name);
            tcp_write(pcb_data, strBuf, strlen(strBuf), 1);
            numberOfFiles++;
        }
        listflag = 0;
        free(pStEntry);
        closedir(pDir);
        numberOfFiles++;
        printf("file end");
    }
}
    char  strBuff[1200];
/* Download the file */
void doRetrieve(void)
{
    printf("In Retrieve Loop!\n");
    cmdStatus = RETRRECEIVED;

    // strBuff = (char *)malloc(1024*5);

    // if ( strBuff == NULL )
    //     printf("wrong\n");

    if (dataStatus == CONNECTED) {

        vfs_read(strBuff, sizeof(strBuff), f);
        vfs_close(f);

        // fgets(strBuff, sizeof(strBuff), f);
        // fclose(f);
        char* pos = strBuff;
        tcp_write(pcb_data, strBuff, strlen(strBuff), 1);
        
    }
}

bool processCommand(void)
{
    /*************************************************************/
    /*  USER - Username for Authentication                       */
    /*************************************************************/
    if (cmdStatus == USERREQUIRED) {
        if (strcmp(command, "USER"))
            sendCmdConn("530 Please login with USER and PASS.");
        else if (strcmp(parameter, FTP_USER))
            sendCmdConn("530 user not found");
        else {
            printf("the user is esp32\n");
            sendCmdConn("331 User user accepted, provide password.");
            cmdStatus = PASSREQUIRED;
            return true;
        }
        return false;
    }
    /*************************************************************/
    /*  PASS - Password for Authentication                       */
    /*************************************************************/
    else if (cmdStatus == PASSREQUIRED) {
        if (strcmp(command, "PASS"))
            sendCmdConn("530 Please login with USER and PASS.");
        else if (strcmp(parameter, FTP_PASS))
            sendCmdConn("530 Login incorrect");
        else {
            sendCmdConn("230 User user logged in.");
            cmdStatus = ACCEPTINGCMD;
            return true;
        }
        return false;
    }
    /*************************************************************/
    /*  QUIT - End of Client Session                             */
    /*************************************************************/
    else if (!strcmp(command, "QUIT")) {
        cmdConnClose();
        return false;
    }
    /*************************************************************/
    /*  PWD - Print Directory                                    */
    /*************************************************************/
    else if (!strcmp(command, "PWD")) {
        if (FIRSTPRINT == 1) {
            sendCmdConn("257 \"sdcard\" is your current location");
            FIRSTPRINT = 0;
        }
        else {
//               if (!strcmp(curr, "/sdcard"))
            sendCmdConn("257 \"sdcard\" is your current location");
//               else {
//          printf("current_dir = %s\n", curr);
//          sprintf(strBuf, "257 \"%s\" is your current location", curr);
//          tcp_write(pcb_cmd, strBuf, strlen(strBuf), 1);
//}
        }
    }
    /*************************************************************/
    /*  PASV - Passive Connection management                     */
    /*************************************************************/
    else if (!strcmp(command, "PASV")) {
        if (dataStatus != NODATACONN)
            dataConnClose();

        sendCmdConn("227 Entering Passive Mode (192,168,4,1,5,20).");
        dataConnCreate();
        return true;
    }
    /*************************************************************/
    /*  LIST - List                                              */
    /*************************************************************/
    else if (!strcmp(command, "LIST")) {
        listflag = 1;
        printf("Ready to list");
        if (dataStatus == CONNECTED)
            doListFiles();
        else
            cmdStatus = LISTRECEIVED;
        return true;
    }
    /*************************************************************/
    /*  ABOR - Abort                                             */
    /*************************************************************/
    else if (!strcmp(command, "ABOR")) {
        dataConnClose();
        sendCmdConn("226 Data connection closed");
        return false;
    }
    /*************************************************************/
    /*  RETR - Retrieve                                          */
    /*************************************************************/
    else if (!strcmp(command, "RETR")) {
        retrievefalg = 1;
        if (strlen(parameter) == 0)
            sendCmdConn("%s", "501 No file name");
        else if (dataStatus == NODATACONN)
            sendCmdConn("%s", "425 No data connection");
        else {
            f = vfs_open(parameter);
//            f = openfile_read(parameter);
            if (f == NULL) {
                sendCmdConn("Can't open %s: No such file or directory", parameter);
            } else {
                if (dataStatus == CONNECTED)
                    doRetrieve();
                else
                    cmdStatus = RETRRECEIVED;
            }
            return true;
        }
        return false;
    }
    /*************************************************************/
    /*  TYPE - Data Type                                         */
    /*************************************************************/
    else if (!strcmp(command, "TYPE")) {
        if (!strcmp(parameter, "A")) {
            sendCmdConn("200 TYPE is now ASCII");
            dataTypeBinary = false;
        }
        else if (!strcmp(parameter, "I")) {
            sendCmdConn("200 TYPE is now 8-bit binary");
            dataTypeBinary = true;
        }
        else {
            sendCmdConn("504 Unknow TYPE");
            return false;
        }
        return true;
    }
    /*************************************************************/
    /*  RNFR - Rename File From                                  */
    /*************************************************************/
    else if (!strcmp(command, "RNFR")) {
        if (strlen(parameter) == 0)
            sendCmdConn("%s", "501 file name required for RNFR");
        else {
            FILE* testfd = openfile_read(parameter);
            if (!testfd) {
                sendCmdConn("550 File %s does not exist", parameter);
                return false;
            } else {
                fclose(testfd);
                sendCmdConn("350 RNFR accepted - file exists, ready for destination");
                strcpy(strBuf, parameter);
                cmdStatus = RNTOREQUIRED;
                return true;
            }
        }
        return false;
    }
    /*************************************************************/
    /*  CWD - Change Working Directory                           */
    /*************************************************************/
    else if (!strcmp(command, "CWD")) {
        if (!strcmp(parameter, "/scard"))
            current_dir = "/scard";
        else if (strcmp(parameter, " ")) {
            current_dir = parameter;
            curr = current_dir;
            printf("current_dir = %s\n", current_dir);
        }
        sendCmdConn("250 Directory is changed");
    }
    /*************************************************************/
    /*  MKD - Make Directory                                     */
    /*************************************************************/
    else if (!strcmp(command, "MKD")) {
        sendCmdConn("501 Directory create not possible");
    }
    /*************************************************************/
    /*  RMD - Remove a Directory                                 */
    /*************************************************************/
    else if (!strcmp(command, "RMD")) {
        sendCmdConn("501 Directory remove not possible");
    }
    /*************************************************************/
    /*  CDUP - Change to Parent Directory                        */
    /*************************************************************/
    else if (!strcmp(command, "CDUP")) {
        sendCmdConn("501 Directory change not possible");
    }

    /*************************************************************/
    /*  MODE - Transfer Mode                                     */
    /*************************************************************/
    else if (!strcmp(command, "MODE")) {
        sendCmdConn("502 MODE Command not implemented.");
    }
    /*************************************************************/
    /*  PORT - Data Port                                         */
    /*************************************************************/
    else if (!strcmp(command, "PORT")) {
        sendCmdConn("502 PORT Command not implemented.");
    }
    /*************************************************************/
    /*  STRU - File Structur                                     */
    /*************************************************************/
    else if (!strcmp(command, "STRU")) {
        sendCmdConn("502 STRU Command not implemented.");
    }
    /*************************************************************/
    /*  MLSD - Lists directory contents if directory is named    */
    /*************************************************************/
    else if (!strcmp(command, "MLSD")) {
        sendCmdConn("502 MLSD Command not implemented.");
    }
    /*************************************************************/
    /*  NLST - List of file names in a specified directory       */
    /*************************************************************/
    else if (!strcmp(command, "NLST")) {
        sendCmdConn("502 NLST Command not implemented.");
    }
    /*************************************************************/
    /*  NOOP - No operation                                      */
    /*************************************************************/
    else if (!strcmp(command, "NOOP")) {
        sendCmdConn("502 NOOP Command not implemented.");
    }
    /*************************************************************/
    /*  FEAT - Feature list implemented by server                */
    /*************************************************************/
    else if (!strcmp(command, "FEAT")) {
        sendCmdConn("502 FEAT Command not implemented.");
    }
    /*************************************************************/
    /*  MDTM - Return last-modified time of a file               */
    /*************************************************************/
    else if (!strcmp(command, "MDTM")) {
        sendCmdConn("502 MDTM Command not implemented.");
    }
    /*************************************************************/
    /*  SITE - File Structure                                    */
    /*************************************************************/
    else if (!strcmp(command, "SITE")) {
        sendCmdConn("502 SITE Command not implemented.");
    }
    /*************************************************************/
    /*  SYST - System                                            */
    /*************************************************************/
    else if (!strcmp(command, "SYST")) {
        sendCmdConn("214 UNIX system type.");
    }
    /*************************************************************/
    /*  Other Unrecognized commands ...                          */
    /*************************************************************/
    else
        sendCmdConn("500 Unknown Command %s. Not Implemented.", command);
    return false;
}

/* datasent callback function */
void dataConnSentCb(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    if (cmdStatus == LISTRECEIVED) {
        dataConnClose();
        sendCmdConn("226 %d matches total", numberOfFiles);
        cmdStatus = ACCEPTINGCMD;
    }
    else if (cmdStatus == RETRRECEIVED) {
        dataConnClose();
        sendCmdConn("226 0 seconds (measured here and unkown), uknown Mbytes per second");
        cmdStatus = ACCEPTINGCMD;
    }
}

/* datareceive callback function */
err_t dataConnRecvCb(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
    printf("In dataConnRecvCb\n");
    if (cmdStatus == STORRECEIVED) {
        printf("In dataConnRecvCb ");
    }
    return ERR_OK;
}

/* dataconnect callback function */
err_t dataConnConnectCb(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    printf("In DataConnConnect!\n");
    sendCmdConn("150 Accepted data connection");
    tcp_recv(tpcb, dataConnRecvCb);
    tcp_sent(tpcb, dataConnSentCb);
    pcb_data = tpcb;
    dataStatus = CONNECTED;
    if (listflag == 1)
        doListFiles();
    if (retrievefalg == 1)
        doRetrieve();
    return ERR_OK;
}

void dataConnCreate(void)
{
    pcb_data = tcp_new();
    tcp_bind(pcb_data, IP_ADDR_ANY, FTP_DATA_PORT_PASV);
    pcb_data = tcp_listen(pcb_data);
    tcp_accept(pcb_data, dataConnConnectCb);
    dataStatus = NOCONNECTION;
}

void dataConnClose(void)
{
    if (dataStatus != NODATACONN) {
        printf("In dataConnClose. Closing down data connection!\n");
        tcp_close(pcb_data);
        cmdStatus = ACCEPTINGCMD;
        dataStatus = NODATACONN;
    }
}

err_t cmdConnRecvCb(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
    char *text;
    if (err == ERR_OK && p != NULL) {
        tcp_recved(pcb, p->tot_len);
        text = malloc(p->tot_len + 1);
        if (text) {
            struct pbuf *q;
            char *pt = text;

            for (q = p; q != NULL; q = q->next) {
                bcopy(q->payload, pt, q->len);
                pt += q->len;
            }
            *pt = '\0';

            pt = &text[strlen(text) - 1];

            while (((*pt == '\r') || (*pt == '\n')) && pt >= text)
                *pt-- = '\0';

            printf("query: %s\n", text);
            fflush(stdout);
            strncpy(command, text, 4);

            for (pt = command; isalpha((unsigned char)*pt) && pt < &command[4]; pt++)
                *pt = toupper((unsigned char) * pt);
            *pt = '\0';

            if (strlen(text) < (strlen(command) + 1))
                pt = "";
            else
                pt = &text[strlen(command) + 1];
            parameter = pt;
            printf("Command: '%-4s'|Parameter: '%s'\n", command, parameter);
            fflush(stdout);
            processCommand();
            free(text);
        }
        pbuf_free(p);
    } else if (err == ERR_OK && p == NULL) {
        tcp_close(pcb);
    }
    return ERR_OK;
}

void cmdConnSentCb(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    printf("\n---------------------------------------------------------\n");
    fflush(stdout);
}

err_t cmdConnConnectCb(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    tcp_recv(tpcb, cmdConnRecvCb);
    tcp_sent(tpcb, cmdConnSentCb);
    pcb_cmd = tpcb;
    if (cmdStatus == NOTCONNECTED) {
        char *welcome = "220 -- Welcome Connect to ESP32 FTP\n";
        tcp_write(tpcb, welcome, strlen(welcome), 1);
        cmdStatus = USERREQUIRED;
    }
    return ERR_OK;
}

void cmdConnCreate(void)
{
    pcb_cmd = tcp_new();
    tcp_bind(pcb_cmd, IP_ADDR_ANY, 21);
    pcb_cmd = tcp_listen(pcb_cmd);
    tcp_accept(pcb_cmd, cmdConnConnectCb);
    cmdStatus = NOTCONNECTED;
}

void cmdConnClose()
{
    dataConnClose();
    if (cmdStatus != NOTCONNECTED) {
        sendCmdConn("221 Goodbye");
        tcp_close(pcb_cmd);
        cmdStatus = NOTCONNECTED;
    }
}

void ftp_start(void)
{
    if (cmdStatus != NOTCONNECTED)
        cmdConnClose();
    cmdConnCreate();
}
