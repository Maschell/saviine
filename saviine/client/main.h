/* string.h */
#include "../common/fs_defs.h"
#define NULL ((void *)0)

#define BYTE_NORMAL             0xff
#define BYTE_SPECIAL            0xfe
#define BYTE_OPEN             0x00
#define BYTE_READ             0x01
#define BYTE_CLOSE              0x02
#define BYTE_OK               0x03
#define BYTE_SETPOS           0x04
#define BYTE_STATFILE         0x05
#define BYTE_EOF              0x06
#define BYTE_GETPOS           0x07
#define BYTE_REQUEST        	0x08
#define BYTE_REQUEST_SLOW   	0x09
#define BYTE_HANDLE        		0x0A
#define BYTE_DUMP          		0x0B
#define BYTE_PING         	 	0x0C
#define BYTE_G_MODE 			0x0D
#define BYTE_MODE_D				0x0E
#define BYTE_MODE_I				0x0F
#define BYTE_CLOSE_DUMP         0x10
#define BYTE_LOG_STR            0xfb
#define BYTE_FILE 				0xC0
#define BYTE_FOLDER             0xC1
#define BYTE_GET_FILES          0xCC
#define BYTE_END                0xfd

void *memcpy(void *dst, const void *src, int bytes);
void *memset(void *dst, int val, int bytes);

/* malloc.h */
extern void *(* const MEMAllocFromDefaultHeapEx)(int size, int align);
extern void *(* const MEMAllocFromDefaultHeap)(int size);
extern void *(* const MEMFreeToDefaultHeap)(void *ptr);
#define memalign (*MEMAllocFromDefaultHeapEx)
#define malloc (*MEMAllocFromDefaultHeap)
#define free (*MEMFreeToDefaultHeap)
/* socket.h */
#define AF_INET 2
#define SOCK_STREAM 1
#define IPPROTO_TCP 6

/* FS Functions */
extern FSStatus FSAddClient(FSClient *pClient, FSRetFlag errHandling);
extern void FSInitCmdBlock(FSCmdBlock *pCmd);
extern FSStatus FSCloseDir(FSClient *pClient, FSCmdBlock *pCmd, int dh, FSRetFlag errHandling);
extern FSStatus FSOpenFile(FSClient *pClient, FSCmdBlock *pCmd, char *path,char *mode,int *dh,FSRetFlag errHandling);
				
extern void socket_lib_init();
extern int socket(int domain, int type, int protocol);
extern int socketclose(int socket);
extern int connect(int socket, void *addr, int addrlen);
extern int send(int socket, const void *buffer, int size, int flags);
extern int recv(int socket, void *buffer, int size, int flags);
extern int __os_snprintf(char* s, int n, const char * format, ...);
int getFiles(int sock, char * path,char * resultname, int * resulttype,int *filesize);
int injectFiles(void *pClient, void *pCmd, char * path,char * relativepath,char *  pBuffer, int buffer_size, int error);

struct in_addr {
	unsigned int s_addr;
};
struct sockaddr_in {
    short sin_family;
    unsigned short sin_port;
    struct in_addr sin_addr;
    char sin_zero[8];
};

/* OS stuff */
extern const long long title_id;

/* Forward declarations */
#define MAX_CLIENT 32
#define MASK_FD 0x0fff00ff

struct bss_t {
    int socket_fsa[MAX_CLIENT];
    void *pClient_fs[MAX_CLIENT];
    int socket_fs[MAX_CLIENT];
	char save_path[255];
	volatile int saveFolderChecked;
    volatile int lock;
	int logsock;
};

#define bss_ptr (*(struct bss_t **)0x100000e4)
#define bss (*bss_ptr)

void cafiine_connect(int *socket);
void cafiine_disconnect(int socket);
int getMode(int sock, int * result);
int cafiine_fopen(int socket, int *result, const char *path, const char *mode, int *handle);
int cafiine_send_handle(int sock, int client, const char *path, int handle);
void cafiine_send_file(int sock, char *file, int size, int fd);
int cafiine_fread(int socket, int *result, void *buffer, int size, int fd);
int cafiine_fclose(int socket, int *result, int fd, int dumpclose);
int cafiine_fsetpos(int socket, int *result, int fd, int set);
int cafiine_fgetpos(int socket, int *result, int fd, int *pos);
int cafiine_fstat(int sock, int *result, int fd, void *ptr);
int cafiine_feof(int sock, int *result, int fd);
void cafiine_send_ping(int sock, int val1, int val2);
void log_string(int sock, const char* str, char flag_byte);