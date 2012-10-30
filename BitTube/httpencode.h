#ifndef HTTPENCODE_H
#define HTTPENCODE_H



typedef int ssize_t;
typedef int socklen_t;
typedef unsigned __int64 u_int64_t;
typedef unsigned __int8 u_int8_t;

#define PATH_SP '\\'
#define MAXPATHLEN 1024
#define MAXHOSTNAMELEN 256

//#define mkdir(path,mode) _mkdir((path))

#define snprintf _snprintf
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#define ioctl ioctlsocket

#define RECV(fd,buf,len) recv((fd),(buf),(len),0)
#define SEND(fd,buf,len) send((fd),(buf),(len),0)
#define EWOULDBLOCK	WSAEWOULDBLOCK
#define EINPROGRESS	WSAEINPROGRESS
#define CLOSE_SOCKET(sk) closesocket((sk))

#define REQ_URL_P1_FMT "GET %s?info_hash=%s&peer_id=%s&port=%d"
#define REQ_URL_P2_FMT "%s&uploaded=%d&downloaded=%d&left=%d&event=%s&compact=1 HTTP/1.0"
#define REQ_URL_P3_FMT "%s&uploaded=%d&downloaded=%d&left=%d&compact=1 HTTP/1.0"

char* Http_url_encode(char *s,char *b,size_t n);
int Http_url_analyse(char *url,char *host,int *port,char *path);
size_t Http_split(char *b,size_t n,char **pd,size_t *dlen);
int Http_reponse_code(char *b,size_t n);
int Http_get_header(char *b,int n,char *header,char *v);

#endif
