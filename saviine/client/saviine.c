#include "main.h"

static int recvwait(int sock, void *buffer, int len);
static int recvbyte(int sock);
static int sendwait(int sock, const void *buffer, int len);

static int cafiine_handshake(int sock);

#define CHECK_ERROR(cond) if (cond) { goto error; }

#define BYTE_NORMAL         0xff
#define BYTE_SPECIAL        0xfe
//#define BYTE_OPEN           0x00
//#define BYTE_READ           0x01
#define BYTE_CLOSE          0x02
//#define BYTE_OK             0x03
//#define BYTE_SETPOS         0x04
//#define BYTE_STATFILE       0x05
//#define BYTE_EOF            0x06
//#define BYTE_GETPOS         0x07
#define BYTE_REQUEST        0x08
#define BYTE_REQUEST_SLOW   0x09
#define BYTE_HANDLE         0x0A
#define BYTE_DUMP           0x0B
#define BYTE_PING           0x0C

#define BYTE_LOG_STR            0xfb

void GX2WaitForVsync(void);

void cafiine_connect(int *psock) {
    extern unsigned int server_ip;
    struct sockaddr_in addr;
    int sock, ret;

    socket_lib_init();

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    CHECK_ERROR(sock == -1);

    addr.sin_family = AF_INET;
    addr.sin_port = 7332;
    addr.sin_addr.s_addr = server_ip;

    ret = connect(sock, (void *)&addr, sizeof(addr));
    CHECK_ERROR(ret < 0);
    ret = cafiine_handshake(sock);
    CHECK_ERROR(ret < 0);
    CHECK_ERROR(ret == BYTE_NORMAL);

    *psock = sock;
    return;
error:
    if (sock != -1)
        socketclose(sock);
    *psock = -1;
    return;
}

void cafiine_disconnect(int sock) {
    CHECK_ERROR(sock == -1);
    socketclose(sock);
error:
    return;
}

static int cafiine_handshake(int sock) {
    int ret;
    unsigned char buffer[16];

    memcpy(buffer, &title_id, 16);

    ret = sendwait(sock, buffer, sizeof(buffer));
    CHECK_ERROR(ret < 0);
    ret = recvbyte(sock);
    CHECK_ERROR(ret < 0);
    return ret;
error:
    return ret;
}


int cafiine_send_handle(int sock, int client, const char *path, int handle)
{
    while (bss.lock) GX2WaitForVsync();
    bss.lock = 1;
	
    CHECK_ERROR(sock == -1);

    // create and send buffer with : [cmd id][handle][path length][path data ...]
    {
        int ret;
        int len_path = 0;
        while (path[len_path++]);
        char buffer[1 + 4 + 4 + len_path];

        buffer[0] = BYTE_HANDLE;
        *(int *)(buffer + 1) = handle;
        *(int *)(buffer + 5) = len_path;
        for (ret = 0; ret < len_path; ret++)
            buffer[9 + ret] = path[ret];

        // send buffer, wait for reply
        ret = sendwait(sock, buffer, 1 + 4 + 4 + len_path);
        CHECK_ERROR(ret < 0);

		 // wait reply
        ret = recvbyte(sock);
		if(ret == BYTE_REQUEST){
			ret = 1;
		}else{
			ret = 2;
		}
        // wait reply
        int special_ret = recvbyte(sock);
        CHECK_ERROR(special_ret != BYTE_SPECIAL);
		bss.lock = 0;
		return ret;
    }

error:
    bss.lock = 0;
    return -1;
}

void cafiine_send_file(int sock, char *file, int size, int fd) {
    while (bss.lock) GX2WaitForVsync();
    bss.lock = 1;

    CHECK_ERROR(sock == -1);

    int ret;
    
    // create and send buffer with : [cmd id][fd][size][buffer data ...]
    {
        char buffer[1 + 4 + 4 + size];

        buffer[0] = BYTE_DUMP;
        *(int *)(buffer + 1) = fd;
        *(int *)(buffer + 5) = size;
        for (ret = 0; ret < size; ret++)
            buffer[9 + ret] = file[ret];

        // send buffer, wait for reply
        ret = sendwait(sock, buffer, 1 + 4 + 4 + size);

        // wait reply
        ret = recvbyte(sock);
        CHECK_ERROR(ret != BYTE_SPECIAL);
    }

error:
    bss.lock = 0;
    return;
}



int cafiine_fclose(int sock, int *result, int fd) {
    while (bss.lock) GX2WaitForVsync();
    bss.lock = 1;

    CHECK_ERROR(sock == -1);

    int ret;
    char buffer[1 + 4];
    buffer[0] = BYTE_CLOSE;
    *(int *)(buffer + 1) = fd;
    ret = sendwait(sock, buffer, 1 + 4);
    CHECK_ERROR(ret < 0);
    ret = recvbyte(sock);
    CHECK_ERROR(ret < 0);
    CHECK_ERROR(ret == BYTE_NORMAL);
    ret = recvwait(sock, result, 4);
    CHECK_ERROR(ret < 0);

    bss.lock = 0;
    return 0;
error:
    bss.lock = 0;
    return -1;
}

void cafiine_send_ping(int sock, int val1, int val2) {
    while (bss.lock) GX2WaitForVsync();
    bss.lock = 1;
    
    int ret;
    char buffer[1 + 4 + 4];
    buffer[0] = BYTE_PING;
    *(int *)(buffer + 1) = val1;
    *(int *)(buffer + 5) = val2;

    ret = sendwait(sock, buffer, 1 + 4 + 4);
    CHECK_ERROR(ret < 0);

    error:
    bss.lock = 0;
    return;
}

static int recvwait(int sock, void *buffer, int len) {
    int ret;
    while (len > 0) {
        ret = recv(sock, buffer, len, 0);
        CHECK_ERROR(ret < 0);
        len -= ret;
        buffer += ret;
    }
    return 0;
error:
    return ret;
}

static int recvbyte(int sock) {
    unsigned char buffer[1];
    int ret;

    ret = recvwait(sock, buffer, 1);
    if (ret < 0) return ret;
    return buffer[0];
}

static int sendwait(int sock, const void *buffer, int len) {
    int ret;
    while (len > 0) {
        ret = send(sock, buffer, len, 0);
        CHECK_ERROR(ret < 0);
        len -= ret;
        buffer += ret;
    }
    return 0;
error:
    return ret;
}

void log_string(int sock, const char* str, char flag_byte) {
    if(sock == -1) {
		return;
	}
    while (bss.lock) GX2WaitForVsync();
    bss.lock = 1;

    int i;
    int len_str = 0;
    while (str[len_str++]);

    //
    {
        char buffer[1 + 4 + len_str];
        buffer[0] = flag_byte;
        *(int *)(buffer + 1) = len_str;
        for (i = 0; i < len_str; i++)
            buffer[5 + i] = str[i];

        buffer[5 + i] = 0;

        sendwait(sock, buffer, 1 + 4 + len_str);
    }

    bss.lock = 0;
}
