#include "main.h"
#include "../common/fs_defs.h"
#define DECL(res, name, ...) \
	extern res name(__VA_ARGS__); \
	res (* real_ ## name)(__VA_ARGS__)  __attribute__((section(".magicptr"))); \
	res my_ ## name(__VA_ARGS__)

#define  BYTE_LOG_STR 0xfb


extern FSStatus FSOpenDir(FSClient *pClient, FSCmdBlock *pCmd, const char *path, int *dh, FSRetFlag errHandling);
extern FSStatus FSReadDir(FSClient *pClient, FSCmdBlock *pCmd, int dh, FSDirEntry *dir_entry, FSRetFlag errHandling);
extern FSStatus FSChangeDir(FSClient *pClient, FSCmdBlock *pCmd, const char *path, FSRetFlag errHandling);
extern FSStatus FSCloseDir(FSClient *pClient, FSCmdBlock *pCmd, int dh, FSRetFlag errHandling);
extern FSStatus FSReadFile(FSClient *pClient, FSCmdBlock *pCmd, void *buffer, int size, int count, int fd, int flag, int error);
extern FSStatus FSSetPosFile(FSClient *pClient, FSCmdBlock *pCmd, int fd, int pos, int error);
extern FSStatus FSCloseFile (FSClient *pClient, FSCmdBlock *pCmd, int fd, int error);
extern FSStatus FSOpenFile(FSClient *pClient, FSCmdBlock *pCmd,  char *path, const char *mode, int *handle, int error);

DECL(int, FSAInit, void) {
    if ((int)bss_ptr == 0x0a000000) {
        bss_ptr = memalign(sizeof(struct bss_t), 0x40);
        memset(bss_ptr, 0, sizeof(struct bss_t));
    }
    return real_FSAInit();
}
DECL(int, FSAShutdown, void) {
    return real_FSAShutdown();
}
DECL(int, FSAAddClient, void *r3) {
    int res = real_FSAAddClient(r3);

    if ((int)bss_ptr != 0x0a000000 && res < MAX_CLIENT && res >= 0) {
        cafiine_connect(&bss.socket_fsa[res]);
    }

    return res;
}
DECL(int, FSADelClient, int client) {
    if ((int)bss_ptr != 0x0a000000 && client < MAX_CLIENT && client >= 0) {
        cafiine_disconnect(bss.socket_fsa[client]);
    }

    return real_FSADelClient(client);
}
DECL(int, FSAOpenFile, int client, const char *path, const char *mode, int *handle) {   
    return real_FSAOpenFile(client, path, mode, handle);
}

static int client_num_alloc(void *pClient) {
    int i;

    for (i = 0; i < MAX_CLIENT; i++)
        if (bss.pClient_fs[i] == 0) {
            bss.pClient_fs[i] = pClient;
            return i;
        }
    return -1;
}
static void clietn_num_free(int client) {
    bss.pClient_fs[client] = 0;
}
static int client_num(void *pClient) {
    int i;

    for (i = 0; i < MAX_CLIENT; i++)
        if (bss.pClient_fs[i] == pClient)
            return i;
    return -1;
}


struct fs_async_t {
    void (*callback)(int status, int command, void *request, void *response, void *context);
    void *context;
    void *queue;
};

#define DUMP_BLOCK_SIZE (0x200 * 100)
#define DUMP_BLOCK_SIZE_SLOW (0x20 * 100)
static int dump_dir(void *pClient,int client, void *pCmd, char *path, int error, int handle){		
				int dir_handle = handle;
				int my_handle = handle +1;
				int ret = 0;
				 if ((ret = FSOpenDir(pClient, pCmd, path, &dir_handle, FS_RET_ALL_ERROR)) == FS_STATUS_OK)
                {
					char new_mode[2];
					new_mode[0] = 'r';
					new_mode[1] = '\0';					
					
                    FSDirEntry dir_entry;
                    while (FSReadDir(pClient, pCmd, dir_handle, &dir_entry, FS_RET_ALL_ERROR) == FS_STATUS_OK)
                    {
						char full_path[255];
						int i=0;
						char *path_ptr = (char *)path;
						while(*path_ptr) {
							full_path[i++] = *path_ptr++;
						}	
						full_path[i++] = '/';
						char *dir_name_ptr = (char *)dir_entry.name;
						while(*dir_name_ptr) {
							full_path[i++] = *dir_name_ptr++;
						}					
						full_path[i++] = '\0';
							
						log_string(bss.socket_fsa[client], full_path, BYTE_LOG_STR);
						if((dir_entry.stat.flag&FS_STAT_FLAG_IS_DIRECTORY) == FS_STAT_FLAG_IS_DIRECTORY){
							char type[4];
							type[0] = 'd';
							type[1] = 'i';
							type[2] = 'r';
							type[3] = '\0';
							log_string(bss.socket_fsa[client], type, BYTE_LOG_STR);
							dump_dir(pClient,client,pCmd,full_path,error,my_handle);							
						}else{
							char type[5];
							type[0] = 'f';
							type[1] = 'i';
							type[2] = 'l';
							type[3] = 'e';
							type[4] = '\0';
							log_string(bss.socket_fsa[client], type, BYTE_LOG_STR);
							//DUMP							  
							ret = FSOpenFile(pClient, pCmd, full_path, new_mode, &my_handle, error);		
							if (ret >= 0) {
								int my_ret = 1;
								log_string(bss.socket_fsa[client], full_path, BYTE_LOG_STR);
								int size = (my_ret == 1 ? DUMP_BLOCK_SIZE : DUMP_BLOCK_SIZE_SLOW);
								cafiine_send_handle(bss.socket_fsa[client], client, full_path, my_handle);
								void* buffer = memalign(sizeof(char) * size, 0x40);
								int ret2;
								while ((ret2 = FSReadFile(pClient, pCmd, buffer, 1, size, my_handle, 0, error)) > 0)
									cafiine_send_file(bss.socket_fsa[client], buffer, ret2, my_handle);
								cafiine_fclose(bss.socket_fsa[client], &ret2, my_handle);
								FSSetPosFile(pClient, pCmd, my_handle, 0, error);
								free(buffer);
								FSCloseFile(pClient, pCmd, my_handle, -1);
							}							
						}					   
                    }					
                    FSCloseDir(pClient, pCmd, dir_handle, FS_RET_NO_ERROR);
                }else{
					/*
					char error[3];
					error[0] =  '-';
					error[1] =  (ret*-1) + '0';
					error[2] = '\0'; */
					//log_string(bss.socket_fsa[client], foo, BYTE_LOG_STR);
					return -1;
				}
				return 0;
}

DECL(int, FSInit, void) {
    if ((int)bss_ptr == 0x0a000000) {
        bss_ptr = memalign(sizeof(struct bss_t), 0x40);
        memset(bss_ptr, 0, sizeof(struct bss_t));
    }
    return real_FSInit();
}
DECL(int, FSShutdown, void) {
    return real_FSShutdown();
}
DECL(int, FSAddClientEx, void *r3, void *r4, void *r5) {
    int res = real_FSAddClientEx(r3, r4, r5);

    if ((int)bss_ptr != 0x0a000000 && res >= 0) {
        int client = client_num_alloc(r3);
        if (client < MAX_CLIENT && client >= 0) {
            cafiine_connect(&bss.socket_fs[client]);
        }
    }	
    return res;
}
DECL(int, FSDelClient, void *pClient) {
    if ((int)bss_ptr != 0x0a000000) {
        int client = client_num(pClient);
        if (client < MAX_CLIENT && client >= 0) {
            cafiine_disconnect(bss.socket_fs[client]);
            clietn_num_free(client);
        }
    }
    return real_FSDelClient(pClient);
}

static void dump_saves(void *pClient, void *pCmd,int error, int client){
				int i = 0;				
				char save_path[255];
				char save_user[9];
				char save_base[11];
				char save_common[7];
				
				save_base[i++]  = '/';
				save_base[i++]  = 'v';
				save_base[i++]  = 'o';
				save_base[i++]  = 'l';
				save_base[i++]  = '/';
				save_base[i++]  = 's';
				save_base[i++]  = 'a';
				save_base[i++]  = 'v';
				save_base[i++]  = 'e';
				save_base[i++]  = '/';
				save_base[i++]  = '\0';
				i = 0;			
				
				save_user[i++] = '8';
				save_user[i++] = '0';
				save_user[i++] = '0';
				save_user[i++] = '0';
				save_user[i++] = '0';
				save_user[i++] = '0';
				save_user[i++] = '0';
				save_user[i++] = '0';
				save_user[i++]  = '\0';
				
				i = 0;
				save_common[i++] = 'c';
				save_common[i++] = 'o';
				save_common[i++] = 'm';
				save_common[i++] = 'm';
				save_common[i++] = 'o';
				save_common[i++] = 'n';
				save_common[i++] = '\0';
				
				i = 0;
				char *save_base_ptr = (char *)save_base;
				while(*save_base_ptr) {
					save_path[i++] = *save_base_ptr++;
				}
				
				int k = i;
				char *save_user_ptr = (char *)save_user;
				while(*save_user_ptr) {
					save_path[i++] = *save_user_ptr++;
				}					
				save_path[i++] = '\0';
				int id = 1;
				do{
					//log_string(bss.socket_fsa[client], save_path, BYTE_LOG_STR);
					if (dump_dir(pClient,client,pCmd,save_path,error,50) == 0);// id = 257; // break if successful
					int first = id/16;
					int seconds = id%16;
					if(first <= 9)
						save_path[16] = '0' + first;
					else
						save_path[16] = 'a' + (first - 10);
						
					if(seconds <= 9)
						save_path[17] = '0' + seconds;
					else
						save_path[17] = 'a' + (seconds - 10);
				
					id++;
				}while(id < 257);
				
				i = k;
				
				char *save_common_ptr = (char *)save_common;
				while(*save_common_ptr) {
					save_path[i++] = *save_common_ptr++;
				}					
				save_path[i++] = '\0';
				
				log_string(bss.socket_fsa[client], save_path, BYTE_LOG_STR);
				dump_dir(pClient,client,pCmd,save_path,error,60);
				
				i = 0;
				char info[6];				
				info[i++] = 'd';
				info[i++] = 'o';
				info[i++] = 'n';
				info[i++] = 'e';
				info[i++] = '!';
				info[i++] = '\0';
				log_string(bss.socket_fsa[client], info, BYTE_LOG_STR);
}

DECL(int, FSGetStat, void *pClient, void *pCmd, char *path, void *stats, int error) {
    if ((int)bss_ptr != 0x0a000000) {
        int client = client_num(pClient);
		
        if (client < MAX_CLIENT && client >= 0) {
			if(bss.savesDumped == 0){
				dump_saves(pClient,pCmd,error,client);
				bss.savesDumped = 2;
			}			
		}
    }
    return real_FSGetStat(pClient, pCmd, path, stats, error);
}

#define MAKE_MAGIC(x) { x, my_ ## x, &real_ ## x }

struct magic_t {
    const void *real;
    const void *replacement;
    const void *call;
} methods[] __attribute__((section(".magic"))) = {
    MAKE_MAGIC(FSAInit),
    MAKE_MAGIC(FSAShutdown),
    MAKE_MAGIC(FSAAddClient),
    MAKE_MAGIC(FSADelClient),
    MAKE_MAGIC(FSAOpenFile),
	MAKE_MAGIC(FSGetStat),  
    MAKE_MAGIC(FSInit),
    MAKE_MAGIC(FSShutdown),
    MAKE_MAGIC(FSAddClientEx),
    MAKE_MAGIC(FSDelClient),	
};
