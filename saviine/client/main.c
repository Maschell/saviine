#include "main.h"

#define DECL(res, name, ...) \
	extern res name(__VA_ARGS__); \
	res (* real_ ## name)(__VA_ARGS__)  __attribute__((section(".magicptr"))); \
	res my_ ## name(__VA_ARGS__)
#define DEBUG_LOG 0
#define BUFFER_SIZE        (1024)*100*2
#define BUFFER_SIZE_STEPS  0x200

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

static int strlen(char* path) {
    int i = 0;
    while (path[i++])
        ;
    return i;
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
DECL(int, FSWriteFile,FSClient *pClient, FSCmdBlock *pCmd, const void *source,int size, int count, int fileHandle, int flag,FSRetFlag error) {
return real_FSWriteFile(pClient,pCmd,source,size,count,fileHandle,flag,error);
}

DECL(int, FSFlushQuota,FSClient *pClient,FSCmdBlock *pCmd,const char *path,FSRetFlag error) {
return real_FSFlushQuota(pClient,pCmd,path,error);
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
						bss.logsock = bss.socket_fs[client];
					}else{
						goto error;
					}
					// Init command block.
					FSInitCmdBlock(pCmd);
					
					hook(pClient, pCmd,-1);
					bss.saveFolderChecked = 2;
				
				error:	real_FSDelClient(pClient);	
					free(pClient);
					free(pCmd);
				}
			}
            //cafiine_connect(&bss.socket_fs[client]);
        //}		
    }	
	
    return res;
}

void hook(void * pClient,void * pCmd, int error){
	log_string(bss.logsock, "hook", BYTE_LOG_STR);
	handle_saves(pClient, pCmd,-1);	
}

void handle_saves(void *pClient, void *pCmd,int error){
	log_string(bss.logsock, "init", BYTE_LOG_STR);
	int client = client_num(pClient);
	unsigned char slotNo;
	long id = getPesistentID(&slotNo);
	init_Save(slotNo);
	int mode;
	
	log_string(bss.logsock, "getting mode", BYTE_LOG_STR);		
	if(getMode(bss.socket_fsa[client],&mode)){
		if(id >= 0x80000000 && id <= 0x90000000){		
			char savepath[20];			
			__os_snprintf(savepath, sizeof(savepath), "/vol/save/%08x",id);	
			if(mode == BYTE_MODE_D){
				log_string(bss.logsock, "dump mode!", BYTE_LOG_STR);	
				dumpSavaData(pClient, pCmd,id,error);
			}else if(mode == BYTE_MODE_I){
				log_string(bss.logsock, "inject mode", BYTE_LOG_STR);					
				injectSaveData(pClient,pCmd,id,error);
			}
		}		
	}
}

void dumpSavaData(void *pClient, void *pCmd,long persistentID,int error){
	int client = client_num(pClient);	
	/*
		Allocate buffer for injection
	*/
	int buf_size = BUFFER_SIZE;
 	char * pBuffer;
	int failed = 0;
	do{
		buf_size -= BUFFER_SIZE_STEPS;
		if(buf_size < 0){
			log_string(bss.logsock, "error on buffer allocation", BYTE_LOG_STR);
			failed = 1;
			break;									
		}
		pBuffer = (char *)MEMAllocFromDefaultHeapEx(buf_size, 0x40);
	}while(!pBuffer);

	if(!failed){
		int mask = 0;
		char buffer[60];
		__os_snprintf(buffer, sizeof(buffer), "allocated %d bytes",buf_size);
		log_string(bss.logsock, buffer, BYTE_LOG_STR);
  		if(saviine_start_dump(bss.socket_fsa[client], persistentID,&mask)){
			if((mask & MASK_USER) == MASK_USER){
				char savepath[20];			
				__os_snprintf(savepath, sizeof(savepath), "/vol/save/%08x",persistentID);	
				log_string(bss.logsock, "user savedata", BYTE_LOG_STR);				
				if(dump_dir(pClient,pCmd,savepath,pBuffer,buf_size,-1,50) == -1){
					log_string(bss.logsock, "error dumping user dir", BYTE_LOG_STR);
					failed = 1;
				}
			}
			if((mask & MASK_COMMON) == MASK_COMMON && !failed){
				char * commonDir = "/vol/save/common";
				log_string(bss.logsock, "dumping common savedata", BYTE_LOG_STR);					
				if(dump_dir(pClient,pCmd,commonDir,pBuffer,buf_size,error,60) == -1){
					log_string(bss.logsock, "error dumping common dir (maybe the game has no common folder?)", BYTE_LOG_STR);
				}
			}
			log_string(bss.logsock, "done!", BYTE_LOG_STR);			
			
			if(!saviine_end_dump(bss.socket_fsa[client])) if(DEBUG_LOG) log_string(bss.logsock, "saviine_end_injection() failed", BYTE_LOG_STR);
			free(pBuffer);
			if(DEBUG_LOG) log_string(bss.logsock, "end of dump", BYTE_LOG_STR);	
		}else{
			log_string(bss.logsock, "saviine_start_dump() failed", BYTE_LOG_STR);
		}
	}
}

int dump_dir(void *pClient, void *pCmd, char *path, void * pBuffer, int size,int error, int handle){
	int client = client_num(pClient);
	int dir_handle = handle;
	int my_handle = handle +1;
	int ret = 0;
	int final_result = 0;
	if ((ret = FSOpenDir(pClient, pCmd, path, &dir_handle, FS_RET_ALL_ERROR)) == FS_STATUS_OK){		
		char buffer[strlen(path) + 25];					
		__os_snprintf(buffer, sizeof(buffer), "open dir %s",path);	
		log_string(bss.logsock, buffer, BYTE_LOG_STR);
		FSDirEntry dir_entry;		
		while (FSReadDir(pClient,  pCmd, dir_handle, &dir_entry, FS_RET_ALL_ERROR) == FS_STATUS_OK && final_result == 0)
		{
			char full_path[strlen(path) + 1 + strlen(dir_entry.name) +1];
			__os_snprintf(full_path, sizeof(full_path), "%s/%s",path,dir_entry.name);	
			
			if((dir_entry.stat.flag&FS_STAT_FLAG_IS_DIRECTORY) == FS_STAT_FLAG_IS_DIRECTORY){							
				log_string(bss.logsock, "-> dir", BYTE_LOG_STR);
				if(dump_dir(pClient, pCmd,full_path,pBuffer,size,-1,my_handle) == -1){
					log_string(bss.logsock, "error", BYTE_LOG_STR);
					final_result = -1;
				}
			}else{						
				//DUMP							  
				ret = FSOpenFile(pClient,  pCmd, full_path, "r", &my_handle, FS_RET_ALL_ERROR);								
				if (ret >= 0) {	
					__os_snprintf(buffer, sizeof(buffer), "dumping %s",dir_entry.name);	
					log_string(bss.logsock, buffer, BYTE_LOG_STR);

					int buf_size = BUFFER_SIZE;					
					int ret2;

					int  my_ret = cafiine_send_handle(bss.socket_fsa[client], client, full_path, my_handle);
					if(my_ret != -1){
					while ((ret2 = FSReadFile(pClient,  pCmd, pBuffer, 1, buf_size, my_handle, 0, 0)) > 0)
							cafiine_send_file(bss.socket_fsa[client], pBuffer, ret2, my_handle);
						cafiine_fclose(bss.socket_fsa[client], &ret2, my_handle,1);	
					}else{
						log_string(bss.logsock, "error on opening file on pc" , BYTE_LOG_STR);
						final_result = -1;
					}
															
					FSSetPosFile(pClient,  pCmd, my_handle, 0, FS_RET_ALL_ERROR);					
					FSCloseFile(pClient,  pCmd, my_handle, -1);
				}else{
					char buffer[100];
					__os_snprintf(buffer, sizeof(buffer), "error on FSOpenFile: %d",ret);	
					log_string(bss.logsock, buffer, BYTE_LOG_STR);
					final_result = -1;
				}
			}
		}					
		FSCloseDir(pClient,  pCmd, dir_handle, FS_RET_NO_ERROR);
	}else{
		log_string(bss.logsock, "error on FSOpenDir()", BYTE_LOG_STR);
		final_result = -1;
	}
	return final_result;
}

/**************************
	Injection functions
**************************/
void injectSaveData(void *pClient, void *pCmd,long persistentID,int error){	
	int client = client_num(pClient);
	char logbuffer[255];
	/*
		Allocate buffer for injection
	*/
	int buf_size = BUFFER_SIZE;
 	char * pBuffer;
	int failed = 0;
	do{
		buf_size -= BUFFER_SIZE_STEPS;
		if(buf_size < 0){
			log_string(bss.logsock, "error on buffer allocation", BYTE_LOG_STR);
			failed = 1;
			break;									
		}
		pBuffer = (char *)MEMAllocFromDefaultHeapEx(buf_size, 0x40);
	}while(!pBuffer);
	
	if(!failed){
		int result = 0;
		int mask = 0;
		if((result = saviine_start_injection(bss.socket_fsa[client], persistentID,&mask))){					
			if((mask & MASK_USER) == MASK_USER){
				char savepath[20];			
				__os_snprintf(savepath, sizeof(savepath), "/vol/save/%08x",persistentID);	
				__os_snprintf(logbuffer, sizeof(logbuffer), "injecting new userdata in %08x",persistentID);		
				log_string(bss.logsock, logbuffer, BYTE_LOG_STR);
				log_string(bss.logsock, "deleting userdata save", BYTE_LOG_STR);						
				if(remove_files_in_dir(pClient,pCmd,savepath,0) == -1){
					failed = 1;									
				}else{
					/*
					Inject Save
					*/
					result = injectFiles(pClient,pCmd,savepath,"/",pBuffer,buf_size,-1);
					doFlushOrRollback(pClient,pCmd,result,savepath);									
				}
			}							
			if((mask & MASK_COMMON) == MASK_COMMON && !failed){			
				char * commonDir = "/vol/save/common";
				
				if((mask & MASK_COMMON_CLEAN) == MASK_COMMON_CLEAN){		
					log_string(bss.logsock, "deleting common save", BYTE_LOG_STR);
					if(remove_files_in_dir(pClient,pCmd,commonDir,0) == -1){
						failed = 1;									
					}
				}
				if(!failed){					
					/*
					Inject common
					*/
					result = injectFiles(pClient,pCmd,commonDir,"/",pBuffer,buf_size,-1);
					doFlushOrRollback(pClient,pCmd,result,commonDir);	
				}
			}							
			if(!saviine_end_injection(bss.socket_fsa[client])) if(DEBUG_LOG) log_string(bss.logsock, "saviine_end_injection() failed", BYTE_LOG_STR);
			free(pBuffer);
			if(DEBUG_LOG)log_string(bss.logsock, "end of injection", BYTE_LOG_STR);	
		}else{
			log_string(bss.logsock, "saviine_start_injection() failed", BYTE_LOG_STR);
		}
	}
}

int injectFiles(void *pClient, void *pCmd, char * path,char * relativepath,char *  pBuffer, int buffer_size, int error){
	int client = client_num(pClient);
	int failed = 0;
	int filesinjected = 0;
	int type = 0;
	log_string(bss.logsock, "injecting files", BYTE_LOG_STR);
	char namebuffer[255];
	char logbuffer[255];
	int filesize = 0;

	if(!failed){
		__os_snprintf(logbuffer, sizeof(logbuffer), "buffer size: %d bytes",buffer_size);	
		log_string(bss.logsock, logbuffer, BYTE_LOG_STR);
		
		while(saviine_readdir(bss.socket_fsa[client],path,namebuffer, &type,&filesize) && !failed){			
			if(DEBUG_LOG)log_string(bss.logsock, "got a file", BYTE_LOG_STR);
			char newpath[strlen(path) + 1 + strlen(namebuffer)];
			__os_snprintf(newpath, sizeof(newpath), "%s/%s",path,namebuffer);	
			if(type == BYTE_FILE){
				__os_snprintf(logbuffer, sizeof(logbuffer), "file: %s%s size: %d",relativepath,namebuffer,filesize);					
				log_string(bss.logsock, logbuffer, BYTE_LOG_STR);
				if(DEBUG_LOG) log_string(bss.logsock, "downloading it", BYTE_LOG_STR);
				
				int handle = 10;
				if(FSOpenFile(pClient, pCmd, newpath,"w+",&handle,-1) >= 0){
					if(DEBUG_LOG) log_string(bss.logsock, "file opened and created", BYTE_LOG_STR);
					
					if(filesize > 0){
						failed = doInjectForFile(pClient,pCmd,handle,newpath,filesize,pBuffer,buffer_size);
					}else{
						if(DEBUG_LOG) log_string(bss.logsock, "filesize is 0", BYTE_LOG_STR);						
					}
					
					if((FSCloseFile (pClient, pCmd, handle, -1)) < 0){
						log_string(bss.logsock, "FSCloseFile failed", BYTE_LOG_STR);
						failed = 1;
					}					
				}else{
					log_string(bss.logsock, "opening the file failed", BYTE_LOG_STR);
					failed = 1;
				}
				if(!failed) filesinjected++;
			}else if( type == BYTE_FOLDER){	
				__os_snprintf(logbuffer, sizeof(logbuffer), "dir: %s",namebuffer);				
				log_string(bss.logsock, logbuffer, BYTE_LOG_STR);					
				if(DEBUG_LOG) log_string(bss.logsock, newpath, BYTE_LOG_STR);
				if(FSMakeDir(pClient, pCmd, newpath, -1) == 0){
					char op_offset[strlen(relativepath) + strlen(namebuffer)+ 1 + 1];
					__os_snprintf(op_offset, sizeof(op_offset), "%s%s/",relativepath,namebuffer);	
					int injectedsub = injectFiles(pClient, pCmd, newpath,op_offset,pBuffer,buffer_size,error);
					if(injectedsub == -1){
						failed = 1;						
					}else{
						filesinjected += injectedsub;
					}
				}else{
					log_string(bss.logsock, "folder creation failed", BYTE_LOG_STR);
					failed = 1;
				}
			}
		}		
		if(failed) return -1;
		else return filesinjected;
	}else{
		return -1;
	}	
}

int doInjectForFile(void * pClient, void * pCmd,int handle,char * filepath,int filesize,void * pBuffer,int buf_size){
	int client = client_num(pClient);
	int failed = 0;
	int myhandle;
	int ret = 0;
	char logbuffer[255];
	if((cafiine_fopen(bss.socket_fsa[client], &ret, filepath, "r", &myhandle)) == 0 && ret == 0){
		if(DEBUG_LOG)__os_snprintf(logbuffer, sizeof(logbuffer), "cafiine_fopen with handle %d",myhandle);	
		if(DEBUG_LOG) log_string(bss.logsock, logbuffer, BYTE_LOG_STR);			
			int retsize = 0;
			int pos = 0;
			while(pos < filesize){
				if(DEBUG_LOG) log_string(bss.logsock, "reading", BYTE_LOG_STR);
				if(cafiine_fread(bss.socket_fsa[client], &retsize, pBuffer, buf_size , myhandle) == 0){							
					if(DEBUG_LOG)__os_snprintf(logbuffer, sizeof(logbuffer), "got %d",retsize);					
					if(DEBUG_LOG) log_string(bss.logsock, logbuffer, BYTE_LOG_STR);	
					int fwrite = 0;
					if((fwrite = my_FSWriteFile(pClient, pCmd,  pBuffer,sizeof(char),retsize,handle,0,0x0200)) >= 0){
						if(DEBUG_LOG)__os_snprintf(logbuffer, sizeof(logbuffer), "wrote %d",retsize);					
						if(DEBUG_LOG) log_string(bss.logsock, logbuffer, BYTE_LOG_STR);
					}else{
						if(DEBUG_LOG)__os_snprintf(logbuffer, sizeof(logbuffer), "my_FSWriteFile failed with error: %d",fwrite);					
						if(DEBUG_LOG) log_string(bss.logsock, logbuffer, BYTE_LOG_STR);
						log_string(bss.logsock, "error while FSWriteFile", BYTE_LOG_STR);
						failed = 1;
					}	
					if(DEBUG_LOG)__os_snprintf(logbuffer, sizeof(logbuffer), "old p %d new p %d",pos,pos+retsize);					
					if(DEBUG_LOG) log_string(bss.logsock, logbuffer, BYTE_LOG_STR);								
					pos += retsize;							
				}else{
					log_string(bss.logsock, "error while recieving file", BYTE_LOG_STR);
					failed = 1;
					break;
				}
			}
		
			int result = 0;
			if((cafiine_fclose(bss.socket_fsa[client], &result, myhandle,0)) == 0 && result == 0){
				if(DEBUG_LOG) log_string(bss.logsock, "cafiine_fclose success", BYTE_LOG_STR);
			}else{
				log_string(bss.logsock, "cafiine_fclose failed", BYTE_LOG_STR);
				failed = 1;
			}
			

	}else{
		log_string(bss.logsock, "cafiine_fopen failed", BYTE_LOG_STR);
		failed = 1;
	}
	return failed;
}

/*************************
	Util functions
**************************/

/*flush if result != -1*/
void doFlushOrRollback(void *pClient, void *pCmd,int result,char *savepath){
	char logbuffer[50 + strlen(savepath)];
	if(result != -1){							
		__os_snprintf(logbuffer, sizeof(logbuffer), "injected %d files",result);								
		log_string(bss.logsock, logbuffer, BYTE_LOG_STR);
		log_string(bss.logsock, "Flushing data now", BYTE_LOG_STR);		
		if(FSFlushQuota(pClient,pCmd,savepath,-1) == 0){
			log_string(bss.logsock, "success", BYTE_LOG_STR);
		}else{
			log_string(bss.logsock, "failed", BYTE_LOG_STR);
		}					
	}else{
		log_string(bss.logsock, "injection failed, trying to restore the data", BYTE_LOG_STR);								
		if(FSRollbackQuota(pClient,pCmd,savepath,-1) == 0){
			log_string(bss.logsock, "rollback done", BYTE_LOG_STR);
		}else{
			log_string(bss.logsock, "rollback failed", BYTE_LOG_STR);
		}
	}
}

void init_Save(unsigned char slotNo){
	int (*SAVEInit)();
	int (*SAVEInitSaveDir)(unsigned char accountSlotNo);
	unsigned int save_handle;
	OSDynLoad_Acquire("nn_save.rpl", &save_handle);
	OSDynLoad_FindExport(save_handle, 0, "SAVEInit", (void **)&SAVEInit);
	OSDynLoad_FindExport(save_handle, 0, "SAVEInitSaveDir", (void **)&SAVEInitSaveDir);

	SAVEInit();	
	SAVEInitSaveDir(slotNo);
	SAVEInitSaveDir(255U);
}

long getPesistentID(unsigned char * slotno){
	unsigned int nn_act_handle;
	unsigned long (*GetPersistentIdEx)(unsigned char);	
	int (*GetSlotNo)(void);
	void (*nn_Initialize)(void);
	void (*nn_Finalize)(void);
	OSDynLoad_Acquire("nn_act.rpl", &nn_act_handle);	
	OSDynLoad_FindExport(nn_act_handle, 0, "GetPersistentIdEx__Q2_2nn3actFUc", (void **)&GetPersistentIdEx);
	OSDynLoad_FindExport(nn_act_handle, 0, "GetSlotNo__Q2_2nn3actFv", (void **)&GetSlotNo);
	OSDynLoad_FindExport(nn_act_handle, 0, "Initialize__Q2_2nn3actFv", (void **)&nn_Initialize);
	OSDynLoad_FindExport(nn_act_handle, 0, "Finalize__Q2_2nn3actFv", (void **)&nn_Finalize);
	
	nn_Initialize(); // To be sure that it is really Initialized
	
	*slotno = GetSlotNo();
	
	long idlong = GetPersistentIdEx(*slotno);
	
	nn_Finalize(); //must be called an equal number of times to nn_Initialize
	return idlong;
}

int remove_files_in_dir(void * pClient,void * pCmd, char * path, int handle){
	int ret = 0;
	int my_handle = handle +1;
	char buffer[strlen(path) + 50];
	if ((ret = FSOpenDir(pClient, pCmd, path, &handle, FS_RET_ALL_ERROR)) == FS_STATUS_OK){			
		__os_snprintf(buffer, sizeof(buffer), "remove files in dir %s",path);	
		log_string(bss.logsock, buffer, BYTE_LOG_STR);
		FSDirEntry dir_entry;
		while (FSReadDir(pClient,  pCmd, handle, &dir_entry, FS_RET_ALL_ERROR) == FS_STATUS_OK)
		{
			char full_path[strlen(path) + 1 + strlen(dir_entry.name) +1];
			__os_snprintf(full_path, sizeof(full_path), "%s/%s",path,dir_entry.name);	
			if((dir_entry.stat.flag&FS_STAT_FLAG_IS_DIRECTORY) == FS_STAT_FLAG_IS_DIRECTORY){							
				log_string(bss.logsock, "recursive deletion", BYTE_LOG_STR);
				if(remove_files_in_dir(pClient,pCmd,full_path,my_handle) == -1) return -1;
			}else{	
				char buffer[strlen(full_path) + 50];		
				__os_snprintf(buffer, sizeof(buffer), "deleting %s",full_path);	
				log_string(bss.logsock, buffer, BYTE_LOG_STR);
				if((ret = FSRemove(pClient,pCmd,full_path,-1)) < 0){
					__os_snprintf(buffer, sizeof(buffer), "error: %d on removing %s",ret,full_path);
					log_string(bss.logsock, buffer, BYTE_LOG_STR);
					return -1;
				}
			}
		}					
		if((FSCloseDir(pClient,  pCmd, handle, FS_RET_NO_ERROR)) < 0 ){
			log_string(bss.logsock, "error while closing dir", BYTE_LOG_STR);
			return -1;
		}
	}else{
		__os_snprintf(buffer, sizeof(buffer), "error: %d on opening %s",ret,path);
		log_string(bss.logsock, buffer, BYTE_LOG_STR);
		return -1;
	}
	return 0;
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
	MAKE_MAGIC(FSWriteFile),
	MAKE_MAGIC(FSFlushQuota),
};
