FTP Proxy with Rate Control
==
Simple FTP Proxy for Rate control

Introduction
==
This progeam add delay function to control download and upload rate. User can assign upload or download rate when a proxy begins running

Running
==
```sh
$ gcc FtpProxy.c -o proxy
$ ./proxy <ProxyIP> <ProxyPort> <Rate>
```
Please note that your proxy should control the transmission rate by given `<Rate>`.  

Hint
==
You can set MAXSIZE, FTP_PORT, FTP_PASV_CODE and FTP_ADDR at the beginning of the code.
```sh
#define MagicNumber 2
#define MAXSIZE 512
#define FTP_PORT 8740
#define FTP_PASV_CODE 227
#define FTP_ADDR "A.B.C.D"
```
Through experiment, we found out that  proxy may have same effect on different OS.  
So we set up a variable called "Magic Number". User can change it to fit different OS.  
For example, setting it to 2 for Mac.
```sh
#define MagicNumber 2
```
The ratecontrol function looks like this.
```sh
rate_control(t, rate, old_size, MagicNumber);
```
