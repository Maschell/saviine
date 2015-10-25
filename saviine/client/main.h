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
#define BYTE_READ_DIR           0xCC
#define BYTE_INJECTSTART        0x40
#define BYTE_INJECTEND          0x41
#define BYTE_DUMPSTART          0x42
#define BYTE_DUMPEND            0x43
#define BYTE_END                0xfd

#define MASK_NORMAL          0x8000
#define MASK_USER          0x0100
#define MASK_COMMON        0x0200
#define MASK_COMMON_CLEAN  0x0400

void GX2WaitForVsync(void);

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
extern FSStatus FSOpenDir(FSClient *pClient, FSCmdBlock *pCmd, const char *path, int *dh, FSRetFlag errHandling);
extern FSStatus FSReadDir(FSClient *pClient, FSCmdBlock *pCmd, int dh, FSDirEntry *dir_entry, FSRetFlag errHandling);
extern FSStatus FSChangeDir(FSClient *pClient, FSCmdBlock *pCmd, const char *path, FSRetFlag errHandling);
extern FSStatus FSCloseDir(FSClient *pClient, FSCmdBlock *pCmd, int dh, FSRetFlag errHandling);
extern FSStatus FSReadFile(FSClient *pClient, FSCmdBlock *pCmd, void *buffer, int size, int count, int fd, int flag, int error);
extern FSStatus FSSetPosFile(FSClient *pClient, FSCmdBlock *pCmd, int fd, int pos, int error);
extern FSStatus FSCloseFile (FSClient *pClient, FSCmdBlock *pCmd, int fd, int error);
extern FSStatus FSMakeDir(FSClient *pClient, FSCmdBlock *pCmd,const char *path, FSRetFlag errHandling);
extern FSStatus FSRemove(FSClient *pClient, FSCmdBlock *pCmd, const char *path, FSRetFlag errHandling);
extern FSStatus FSRollbackQuota(FSClient *pClient, FSCmdBlock *pCmd, const char *path, FSRetFlag errHandling);
extern void OSDynLoad_Acquire (char* rpl, unsigned int *handle);
extern void OSDynLoad_FindExport (unsigned int handle, int isdata, char *symbol, void *address);
extern void _Exit();

extern void socket_lib_init();
extern int socket(int domain, int type, int protocol);
extern int socketclose(int socket);
extern int connect(int socket, void *addr, int addrlen);
extern int send(int socket, const void *buffer, int size, int flags);
extern int recv(int socket, void *buffer, int size, int flags);

extern int __os_snprintf(char* s, int n, const char * format, ...);

int saviine_readdir(int sock, char * path,char * resultname, int * resulttype,int *filesize);
int injectFiles(void *pClient, void *pCmd, char * path,char * relativepath, char * basepath, char *  pBuffer, int buffer_size, int error);
void doFlushOrRollback(void *pClient,void *pCmd,int result,char *savepath);
void injectSaveData(void *pClient, void *pCmd,long persistentID,int error);
void dumpSavaData(void *pClient, void *pCmd,long persistentID,int error);
long getPesistentID(unsigned char * slotno);
void init_Save(unsigned char slotNo);
int doInjectForFile(void * pClient, void * pCmd,int handle,char * filepath,int filesize, char * basepath,void * pBuffer,int buf_size);
void handle_saves(void *pClient, void *pCmd,int error);
void hook(void * pClient,void * pCmd, int error);
int dump_dir(void *pClient, void *pCmd, char *path, void * pBuffer, int size,int error, int handle);
int remove_files_in_dir(void * pClient,void * pCmd, char * path, int handle);

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
	volatile void * on_start_hook_client;
    volatile int lock;
	volatile int saveinitfs;
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

int saviine_end_injection(int sock);
int saviine_start_injection(int sock, long persistentID,int * mask);
int saviine_start_dump(int sock, long persistentID,int * mask);
int saviine_end_dump(int sock);