#ifndef __FTP_H__
#define __FTP_H__

#include "esp_err.h"
#include "lwip/err.h"
/**
  * @brief      send the response of command
  *
  * @param      the content to send use "pcb_cmd"
  *
  * @return     
  */
void sendCmdConn(const char *fmt, ...);
/**
  * @brief      list the file in the directory
  *
  * @param      
  *
  * @return     
  */
void doListFiles(void);
/**
  * @brief      Download the file
  *
  * @param      
  *
  * @return     
  */
void doRetrieve(void);
/**
  * @brief      process the Command
  *
  * @param      
  *
  * @return     
  */
bool processCommand(void);
/** Function prototype for tcp sent callback functions. 
 *  To close data connection and sent some message with "pcb_cmd"
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb The connection pcb for which data has been acknowledged
 * @param len The amount of bytes acknowledged
 * @return ERR_OK: try to send some data by calling tcp_output
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
void dataConnSentCb(void *arg, struct tcp_pcb *tpcb, u16_t len);
/** Function prototype for tcp sent callback functions. 
 *  To printf the cut-off rule
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb The connection pcb for which data has been acknowledged
 * @param len The amount of bytes acknowledged
 * @return ERR_OK: try to send some data by calling tcp_output
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
void cmdConnSentCb(void *arg, struct tcp_pcb *tpcb, u16_t len);
/** Function prototype for tcp receive callback functions. Called when data has
 * been received.
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb The connection pcb which received data
 * @param p The received data (or NULL when the connection has been closed!)
 * @param err An error code if there has been an error receiving
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
err_t dataConnRecvCb(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
/** Function prototype for tcp accept callback functions. Called when a new
 * connection can be accepted on a listening pcb.
 * list or download by the status of listflag and retrive flag.
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param newpcb The new connection pcb
 * @param err An error code if there has been an error accepting.
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
err_t dataConnConnectCb(void *arg, struct tcp_pcb *tpcb, err_t err);
/**
  * @brief      creat the data tcp connection
  *
  * @param      
  *
  * @return     
  */
void dataConnCreate(void);
/**
  * @brief      close the data tcp connection
  *
  * @param      
  *
  * @return     
  */
void dataConnClose(void);
/** Function prototype for tcp receive callback functions. Called when data has
 * been received.
 * get the command and parameter
 *  
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb The connection pcb which received data
 * @param p The received data (or NULL when the connection has been closed!)
 * @param err An error code if there has been an error receiving
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
err_t cmdConnRecvCb(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
/** Function prototype for tcp accept callback functions. Called when a new
 * connection can be accepted on a listening pcb.
 * list or download by the status of listflag and retrive flag.
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param newpcb The new connection pcb
 * @param err An error code if there has been an error accepting.
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
err_t cmdConnConnectCb(void *arg, struct tcp_pcb *tpcb, err_t err);
/**
  * @brief      creat the command tcp connection
  *
  * @param      
  *
  * @return     
  */
void cmdConnCreate(void);
/**
  * @brief      close the command tcp connection
  *
  * @param      
  *
  * @return     
  */
void cmdConnClose(void);
/**
  * @brief      initilalise the FTP and start FTP.
  *
  * @param      
  *
  * @return     
  */

void ftp_start(void);

#endif
