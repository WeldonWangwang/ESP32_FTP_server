# ESP32 FTP_Server

## 概述
- FTP（File Transfer Protocal），是文件传输协议的简称。  
FTP 是一个客户机/服务器系统。用户通过一个支持 FTP 协议的客户机程序，连接到在远程主机上的 FTP 服务器程序。用户通过客户机程序向服务器程序发出命令，服务器程序执行用户所发出的命令，并将执行的结果返回到客户机。 当 FTP 客户端与服务器建立 FTP 连接时，将与服务器上的两个端口建立联系：端口20和21（非固定）。FTP 使用不同的端口号传输不同的内容，会建立不同的 TCP 连接。首先，使用 TCP 生成一个虚拟连接用于控制信息，然后再生成一个单独的 TCP 连接用于数据传输。
- 将 ESP32 设计为 FTP_Server:
  - 首先需要将 ESP32 设置为 **SoftAP** 模式;
  - 将 SDcard 连接至 ESP32，初始化 SDcard，将 SDcard 作为 FTP_Server 的文件存储介质；
  - 在客户端(PC)进行联网，使得 PC 可以通过 WIFI 连接至 ESP32; 
  - 在客户端(PC)使用 FTP 客户端软件（本文使用 **WinSCP**）进行登录。登录主机名为 ESP32 的IP地址， 端口号为默认的21端口，用户名和密码皆设置为： *esp32*   
    *注： 主机名、用户名和密码均可在程序中进行修改。*

## 软硬件环境搭建说明
### 硬件:  

|名称          |描述    |备注        |   
|:------------:|:------:|:----------:|  
|ESP-WROVER-KIT|服务器  |            |  
|SDcard        |存储设备|            |
|PC            |客户端  |配有无线网卡|

### 软件:  

 - WinSCP 软件:   
WinSCP是一个用于Windows的开源免费SFTP客户端，FTP客户端，WebDAV客户端和SCP客户端。 它的主要功能是在本地和远程计算机之 间进行文件传输。 除此之外，WinSCP提供脚本和基本文件管理器功能。  
WinSCP包括图形用户界面，提供多种语言，与Windows集成，批处理文件脚本和命令行界面集成，以及各种其他有用的功能。  
 - 程序中代码关键设置   
  - IP 地址:  
          IP4_ADDR(&esp_ip[TCPIP_ADAPTER_IF_AP].ip, 192, 168 , 4, 1);
          IP4_ADDR(&esp_ip[TCPIP_ADAPTER_IF_AP].gw, 192, 168 , 4, 1); 
          IP4_ADDR(&esp_ip[TCPIP_ADAPTER_IF_AP].netmask, 255, 255 , 255, 0);  
  - 用户名及密码设置:  
  我们使用两个宏定义实现:  
          #define FTP_USER  "esp32"  
          #define FTP_PASS  "esp32"  
  - 被动模式设置   
          else if (!strcmp(command, "PASV")) {
            if (dataStatus != NODATACONN)
                dataConnClose();
            sendCmdConn("227 Entering Passive Mode (192,168,4,1,5,20).");
            dataConnCreate();
            return true;
          }    
  在服务器接收到来自客户端的 PASV 命令后,利用命令端口(21端口)返回服务器的IP地址和开启的数据传输端口。  
         IP 地址: 192.168.4.1  
         数据端口: 5*256 + 20 = 1300
  - 命令端口及数据端口设置:  
         #define FTP_CTRL_PORT       21          // Command port on wich server is listening  
         #define FTP_DATA_PORT_PASV  1300       // Data port for passive mode   

## 软件设计说明
### FTP协议连接概述  
FTP屏蔽了各计算机系统的细节，因而适合在异构网络中任意计算机之间传送文件。FTP只提供文件传送的一些基本服务，它使用TCP可靠地运输服务，FTP主要功能是减小或消除在不同系统下处理文件的不兼容性。  
FTP使用客户端-服务器模型，一个FTP服务器进程可以为多个客户进程提供服务。FTP服务器有两大部分组成：一个主进程，负责接受新的请求；还有若干从属进程，负责处理单个请求。FTP客户端与服务器之间要建立双重连接，一个是控制连接，一个是数据连接。  
![](http://i.imgur.com/ZF5EGeU.png)  
FTP使用2个TCP端口，一个数据端口和一个命令端口（也可叫做控制端口）。  
在建立命令连接时,一般用21端口作为命令端口,客户机从一个任意的非特权端口N（N≥1024），连接到FTP服务器的命令端口，也就是21端口，建立一个控制连接。这个连接用于传递客户端的命令和服务器端对命令的响应，生存期是整个FTP会话时间。   
  
 在建立数据连接时,如果期间需要传输文件和其它数据，例如：目录列表等，客户端就需要建立数据连接了。这种连接在需要数据传输时建立，而一旦数据传输完毕就关闭，整个FTP期间可能会建立多次。在主动模式下，建立数据连接时，客户端会开始监听端口N+1，并发送FTP命令“port N+1”到FTP服务器。接着服务器会从它自己的数据端口（20）连接到客户端指定的数据端口（N+1），开始进行数据传输。  
 *当混入主动/被动模式的概念时，数据端口就有可能不是20了.*

### 主动模式FTP：  
主动模式下，FTP客户端从任意的非特殊的端口（N > 1023）连入到FTP服务器的命令端口--21端口。然后客户端在N+1（N+1 >= 1024）端口监听，并且通过N+1（N+1 >= 1024）端口发送命令给FTP服务器。服务器会反过来连接用户本地指定的数据端口，比如20端口。  
以服务器端防火墙为立足点，要支持主动模式FTP需要打开如下交互中使用到的端口：  

     1. FTP服务器命令（21）端口接受客户端任意端口（客户端初始连接）  
     2. FTP服务器命令（21）端口到客户端端口（>1023）（服务器响应客户端命令）  
     3. FTP服务器数据（20）端口到客户端端口（>1023）（服务器初始化数据连接到客户端数据端口)  
     4. FTP服务器数据（20）端口接受客户端端口（>1023）（客户端发送ACK包到服务器的数据端口）

用图表示如下：  
  
  ![](http://i.imgur.com/xLR4LRL.jpg)    
  
    在第1步中，客户端的命令端口与FTP服务器的命令端口建立连接，并发送命令“PORT 1027”;  
    在第2步中，FTP服务器给客户端的命令端口返回一个"ACK";  
    在第3步中，FTP服务器发起一个从它自己的数据端口（20）到客户端先前指定的数据端口（1027）的连接;  
    在第4步中,最后客户端给服务器端返回一个"ACK"。  
        
 **主动方式FTP的主要问题: **   
 对于客户端的防火墙来说，这是从外部系统建立到内部客户端的连接，这是通常会被阻塞的.  
  
### 被动模式FTP:  
  
 **本例程中软件设计采用被动模式.**  
被动模式首先需要客户端告知服务器客户端处于被动模式,服务器则会启用对应的被动模式(PASV).  
  
 在被动方式FTP中，命令连接和数据连接都由客户端，这样就可以解决从服务器到客户端的数据端口的入方向连接被防火墙过滤掉的问题。当开启一个FTP连接时，客户端打开两个任意的非特权本地端口（N >1024和N+1）。第一个端口连接服务器的21端口，但与主动方式的FTP不同，客户端不会提交PORT命令并允许服务器来回连它的数据端口，而是提交PASV命令。这样做的结果是服务器会开启一个任意的非特权端口（P >1024），并发送PORT P 和服务器的 IP 地址给客户端。然后客户端发起从本地端口N+1到服务器的端口P的连接用来传送数据。
用图表示如下:  

 ![](http://i.imgur.com/Uhk2Wz3.jpg)  
   
    在第1步中，客户端的命令端口与服务器的命令端口建立连接，并发送命令“PASV”;  
    在第2步中，服务器返回端口2024和服务器 IP，告诉客户端用哪个端口侦听数据连接;  
    在第3步中，客户端初始化一个从自己的数据端口到服务器端指定的数据端口的数据连接;  
    在第4 步中给客户端的数据端口返回一个"ACK"响应。  

## 软件设计说明  
在连接和数据传输过程中,客户端会发送多种命令至服务器,服务器首先会对命令进行解析,然后根据不同的命令进行相应的处理.  
**主要用到的 FTP 命令有: ** 

|命令   |说明               |备注                                                                        |  
|:-----:|:-----------------:|:---------------------------------------------------------------------------|  
|USER   |指定用户名         |通常是控制连接后第一个发出的命令。“USER esp32\r\n”： 用户名为 esp32 登录    |
|PASS   |指定用户密码       |该命令紧跟 USER 命令后。“PASS esp32\r\n”：密码为 esp32                      |
|SIZE   |返回指定文件的大小 |“SIZE file.txt\r\n”：如果 file.txt 文件存在，则返回该文件的大小             |
|CWD   |改变工作目录        |如：“CWD dirname\r\n”                                                       |
|PASV   |进入被动模式       |如：“PASV\r\n”                                                              |
|RETR   |下载文件           |“RETR file.txt \r\n”：下载文件 file.txt                                     |  
|PWD   |打印当前目录        |如：“PWD\r\n”                                                               | 
|LIST   |列出目录           |"LIST -a\r\n": 列出所有目录                                                 |   

**用户响应码: **  
  
客户端向服务器发送 FTP 命令后，服务器返回三位数字编码的响应码。  

    第一个数字给出了命令状态的一般性指示，比如响应成功、失败或不完整。  
    第二个数字是响应类型的分类，如 2 代表跟连接有关的响应，3 代表用户认证。  
    第三个数字提供了更加详细的信息。
  
例如:   

    331 User esp32 accepted, provide password.   
    230 User esp32 logged in.   
    257 \"sdcard\" is your current location  
  
**LIST 返回数据格式: **   

 - MS-DOS文件列表格式解析  
        02-23-05  09:24AM                 33 readme.md  
        05-25-04  08:56AM             33 VC.c    
 - UNIX文件列表格式解析    
        -rwxrw-r--   1 user     user        3014 Nov 12 14:57 readme.md  
        -rwxrw-r--   1 user     user       20480 Mar  3 11:25 VC.c 
 - Windows自带FTP:  
        -rwxrwxrwx   1 owner    group        19041660 May 25  2004 VC.ESn  
 
在本例程中,采用 UNIX 文件格式,在程序中 SYST 命令返回函数中进行设置。  

        else if (!strcmp(command, "SYST")) {
            sendCmdConn("214 UNIX system type.");
        }

## 测试示例
我们利用客户端（PC）使用 Wireshark 进行抓包，配合服务器的串口打印，进行实验验证。
### 登录  
将 ESP32 开启 SoftAP 模式，客户端（PC）开启 WinSCP， 设置站点，进行登录:  
 - 选择文件协议为 FTP，加密方式呢选择不加密；
 - 主机名为服务器 IP 地址：192.168.4.1；
 - 端口号为命令传输端口：21；
 - 用户名和密码均为程序中设置的 *esp32*  

![](http://i.imgur.com/uUgWd54.png)  

**点击登录，对应抓包分析：**  
  
![](http://i.imgur.com/iSrPvYx.png)

1. 首先为服务器和客户端在端口52281和21之间进行的三次 TCP 握手，建立起命令传输连接。客户端的 IP 地址为：192.168.4.2
  
  ![](http://i.imgur.com/co8Cim6.png) 

2. 紧接着为服务器对客户端通过命令端口发送欢迎消息：  

        220 -- Welcom connect to ESP32 FTP  
    
3. 下来是客户端发送 USER 和 PASS 命令进行登录名和密码的验证，并获得服务器认证响应。  


**在服务端的 ESP32 进行串口信息打印：**

    ---------------------------------------------------------
    query: USER esp32  
    Command: 'USER'|Parameter: 'esp32'  
    the user is esp32  
    Response:  331 User esp32 accepted, provide password.
    ---------------------------------------------------------
    query: PASS esp32  
    Command: 'PASS'|Parameter: 'esp32'  
    Response:  230 User esp32 logged in.
    ---------------------------------------------------------   

### 服务器属性获取   

**进行抓包分析：**  

![](http://i.imgur.com/lRTqkfS.png)  

在此可以看到服务器的系统设置为 UNIX 系统，对命令 FEAT 为进行识别处理。

**查看串口打印：**  
  
	---------------------------------------------------------
	query: SYST  
    Command: 'SYST'|Parameter: ''  
    Response:  214 UNIX system type.
	
	---------------------------------------------------------
	query: FEAT  
    Command: 'FEAT'|Parameter: ''  
    Response:  502 FEAT Command not implemented.
	
	---------------------------------------------------------

### 打印 FTP 当前目录  

我们将 SDcard 的目录设置为登陆的默认首页目录，因此首先获取的为 SDcard 的目录。  

**进行抓包分析：**  

![](http://i.imgur.com/EDGZPk4.png)  

这个过程可以分为4步:  
  
第一步：对 PWD 和 CWD 命令进行处理，在服务器收到 PWD 命令后，会对客户端返回当前目录名称，即 **sdcard**，在客户端收到返回后会对目录名称提取并发出 CWD 命令，即将目录改变至 **/sdcard**，然后在改变目录后再次发送进行打印目录指令;  

第二步：获取服务器数据传输类型，获得服务器返回，数据传输类型为 ASCII 类型；  

第三步：设置进入 PASV 被动模式，在改，模式下开始数据传输，服务器返回当前的 IP 地址和开启的数据端口号：1300，并发送LIST 命令请求列出目录内容；   

第四步：客户端得到服务器的地址和端口后，发起数据 TCP 三握手，建立起数据连接；  

第五步：服务器利用数据连接向客户端发送当前目录信息。  

- 对目录返回信息数据包进行打开：  

![](http://i.imgur.com/LFrHwqE.png)

数据表明数据格式为 UNIX 文件类型，当前目录下只有一个文件 FOO.TXT，文件大小为1637字节。 

**查看串口打印：**  

	---------------------------------------------------------
	query: CWD /sdcard  
	Command: 'CWD '|Parameter: '/sdcard'  
	Response:  250 Directory is changed
	---------------------------------------------------------
	query: PWD  
	Command: 'PWD '|Parameter: ''  
	Response:  257 "sdcard" is your current location
	---------------------------------------------------------
	query: TYPE A  
	Command: 'TYPE'|Parameter: 'A'  
	Response:  200 TYPE is now ASCII
	---------------------------------------------------------
	query: PASV  
	Command: 'PASV'|Parameter: ''  
	Response:  227 Entering Passive Mode (192,168,4,1,5,20).  
    ---------------------------------------------------------
    query: LIST -a  
	Command: 'LIST'|Parameter: '-a'  
	Ready to list  
	In DataConnConnect!  
	Response:  150 Accepted data connection  
	file'name is FOO.TXT  
	file end  
	In dataConnClose. Closing down data connection!  
	Response:  226 1 matches total
	---------------------------------------------------------  
    
客户端显示如下：  

![](http://i.imgur.com/s20C8y4.png)


### 文件下载  

在 FTP 目录下的文件右键选择下载，进入下载设置页面：  

![](http://i.imgur.com/k0TaMKq.png)  

**进行抓包分析：**  

![](http://i.imgur.com/wN76q8h.png)  

首先客户端对服务器发出 RETR 命令，并指明下载对象为 FOO.TXT，下来进行新的一次数据传输 TCP 三握手，连接建立后利用新的数据连接向客户端发送文件 FOO.TXT 的内容，并在客户端完成接收。  

**查看串口打印：**  

    ---------------------------------------------------------
	query: PASV  
	Command: 'PASV'|Parameter: ''  
	Response:  227 Entering Passive Mode (192,168,4,1,5,20).
	---------------------------------------------------------
	query: RETR FOO.TXT  
	Command: 'RETR'|Parameter: 'FOO.TXT'  
	file = /sdcard/FOO.TXT  
	In DataConnConnect!  
	Response:  150 Accepted data connection  
	In Retrieve Loop!  
	In dataConnClose. Closing down data connection!  
	In dataConnRecvCb
    ---------------------------------------------------------  

至此，一次完整的 FTP 服务器文件读取与下载传输完成。