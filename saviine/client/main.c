#include "main.h"

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
extern void OSDynLoad_Acquire (char* rpl, unsigned int *handle);
extern void OSDynLoad_FindExport (unsigned int handle, int isdata, char *symbol, void *address);
void GX2WaitForVsync(void);
static void dump_saves(void *pClient, void *pCmd,int error, int client);

static int strlen(char* path) {
    int i = 0;
    while (path[i++])
        ;
    return i;
}

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
DECL(int, FSAddClientEx, void *r3, void *r4, void *r5) {
    int res = real_FSAddClientEx(r3, r4, r5);
	if(bss.saveFolderChecked == 1) return res;
    if ((int)bss_ptr != 0x0a000000 && res >= 0) {
        //int client = client_num_alloc(r3);
        //if (client < MAX_CLIENT && client >= 0) {
		// create game save path	
			if(bss.saveFolderChecked < 2){
				bss.saveFolderChecked = 1;
				// Create client and cmd block
				FSClient* pClient = (FSClient*)malloc(sizeof(FSClient));
				FSCmdBlock* pCmd = (FSCmdBlock*)malloc(sizeof(FSCmdBlock));
				if (pClient && pCmd)
					{
					// Add client to FS.
					FSAddClient(pClient, FS_RET_NO_ERROR);
					int client = client_num_alloc(pClient);
					if(client < MAX_CLIENT && client >= 0) {
						cafiine_connect(&bss.socket_fs[client]);						
					}else{
						goto error;
					}
					// Init command block.
					FSInitCmdBlock(pCmd);
					
					dump_saves(pClient, pCmd,-1, client);
					bss.saveFolderChecked = 2;
					
					
			error:		
					real_FSDelClient(pClient);	
					free(pClient);
					free(pCmd);
				}
			}
            //cafiine_connect(&bss.socket_fs[client]);
        //}
    }	
    return res;
}


static void init_Save(){
	int (*SAVEInit)();
	unsigned int save_handle;
	OSDynLoad_Acquire("nn_save.rpl", &save_handle);
	OSDynLoad_FindExport(save_handle, 0, "SAVEInit", &SAVEInit);
	SAVEInit();					
}

static long getPesistentID(){
	unsigned int nn_act_handle;
	unsigned long (*GetPersistentIdEx)(unsigned char);	
	int (*GetSlotNo)(void);
	void (*nn_Initialize)(void);
	void (*nn_Finalize)(void);
	OSDynLoad_Acquire("nn_act.rpl", &nn_act_handle);	
	OSDynLoad_FindExport(nn_act_handle, 0, "GetPersistentIdEx__Q2_2nn3actFUc", &GetPersistentIdEx);
	OSDynLoad_FindExport(nn_act_handle, 0, "GetSlotNo__Q2_2nn3actFv", &GetSlotNo);
	OSDynLoad_FindExport(nn_act_handle, 0, "Initialize__Q2_2nn3actFv", &nn_Initialize);
	OSDynLoad_FindExport(nn_act_handle, 0, "Finalize__Q2_2nn3actFv", &nn_Finalize);
	
	nn_Initialize(); // To be sure that it is really Initialized
	
	unsigned char slotno = GetSlotNo();
	long idlong = GetPersistentIdEx(slotno);
	
	
	nn_Finalize(); //must be called an equal number of times to nn_Initialize
	return idlong;
}

#define DUMP_BLOCK_SIZE (0x200 * 100)
#define DUMP_BLOCK_SIZE_SLOW (0x20 * 100)
static int dump_dir(void *pClient,int client, void *pCmd, char *path, int error, int handle){		
				int dir_handle = handle;
				int my_handle = handle +1;
				int ret = 0;
				 if ((ret = FSOpenDir(pClient, pCmd, path, &dir_handle, FS_RET_ALL_ERROR)) == FS_STATUS_OK)
                {		
					char buffer[strlen(path) + 25];
								
					__os_snprintf(buffer, sizeof(buffer), "open dir %s",path);	
					log_string(bss.socket_fsa[client], buffer, BYTE_LOG_STR);
                    FSDirEntry dir_entry;
                    while (FSReadDir(pClient,  pCmd, dir_handle, &dir_entry, FS_RET_ALL_ERROR) == FS_STATUS_OK)
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
							
						
						
						if((dir_entry.stat.flag&FS_STAT_FLAG_IS_DIRECTORY) == FS_STAT_FLAG_IS_DIRECTORY){							
							log_string(bss.socket_fsa[client], "-> dir", BYTE_LOG_STR);
							dump_dir(pClient,client, pCmd,full_path,-1,my_handle);							
						}else{						
							//DUMP							  
							ret = FSOpenFile(pClient,  pCmd, full_path, "r", &my_handle, FS_RET_ALL_ERROR);								
							if (ret >= 0) {	
								__os_snprintf(buffer, sizeof(buffer), "dumping %s",dir_entry.name);	
								log_string(bss.socket_fsa[client], buffer, BYTE_LOG_STR);
								int  my_ret = cafiine_send_handle(bss.socket_fsa[client], client, full_path, my_handle);
								
								
								
								int size = (my_ret == 1 ? DUMP_BLOCK_SIZE : DUMP_BLOCK_SIZE_SLOW);
								void * buffer = memalign(sizeof(char) * size, 0x40);
								int ret2;								
								while ((ret2 = FSReadFile(pClient,  pCmd, buffer, 1, size, my_handle, 0, 0)) > 0)
									cafiine_send_file(bss.socket_fsa[client], buffer, ret2, my_handle);
								cafiine_fclose(bss.socket_fsa[client], &ret2, my_handle);								
								FSSetPosFile(pClient,  pCmd, my_handle, 0, FS_RET_ALL_ERROR);
								free(buffer);
								FSCloseFile(pClient,  pCmd, my_handle, -1);
							}else{
								char type[2];
								type[0] = '9' + ret;
								type[1] = '\0';
								log_string(bss.socket_fsa[client], type, BYTE_LOG_STR);
							}
						}	
						
                    }					
                    FSCloseDir(pClient,  pCmd, dir_handle, FS_RET_NO_ERROR);
                }
				return 0;
}

static void dump_saves(void *pClient, void *pCmd,int error, int client){
				init_Save();
				long id = getPesistentID();
				
				log_string(bss.socket_fsa[client], "dumping user savedata", BYTE_LOG_STR);
				
				if(id >= 0x80000000 && id <= 0x90000000){		
					char savepath[20];
					__os_snprintf(savepath, sizeof(savepath), "/vol/save/%08x",id);						
					dump_dir(pClient,client,pCmd,savepath,-1,50);
				}
				
				log_string(bss.socket_fsa[client], "dumping common savedata", BYTE_LOG_STR);
				dump_dir(pClient,client,pCmd,"/vol/save/common/",error,60);				
				
				log_string(bss.socket_fsa[client], "done!", BYTE_LOG_STR);
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
    MAKE_MAGIC(FSInit),
    MAKE_MAGIC(FSShutdown),
    MAKE_MAGIC(FSAddClientEx),
    MAKE_MAGIC(FSDelClient),	
};
