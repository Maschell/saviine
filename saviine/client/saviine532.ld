OUTPUT(saviine532.elf);

SECTIONS {
	.text 0x011e0000 : {
		server_ip = .;
		. = . + 4;
		*(.text._start);
		*(.text*);
		*(.magicptr*);
	}	
	.magic : {
		*(.magic*);
	}
	.rodata : {
		*(.rodata*);
	}
	/DISCARD/ : {
		*(*);
	}
}

/* FSA methods */
PROVIDE(FSAInit = 0x10608ac);
PROVIDE(FSAShutdown = 0x1060974);
PROVIDE(FSAAddClient = 0x106546c);
PROVIDE(FSADelClient = 0x1060aa4);
PROVIDE(FSAOpenFile = 0x10621f8);

/* FS methods */
PROVIDE(FSInit = 0x10683c8);
PROVIDE(FSShutdown = 0x1068538);
PROVIDE(FSAddClientEx = 0x10685fc);
PROVIDE(FSAddClient = 0x010689fc);
PROVIDE(FSDelClient = 0x1068a08);
PROVIDE(FSOpenFile = 0x106ef7c);
PROVIDE(FSOpenFileAsync = 0x0106a434);
PROVIDE(FSInitCmdBlock = 0x01068c54);
PROVIDE(FSMakeDir = 0x0106f8e0);
PROVIDE(FSCloseFile = 0x106f088);
PROVIDE(FSReadFile = 0x106f108);
PROVIDE(FSReadFileWithPos = 0x106f194);
PROVIDE(FSGetPosFile = 0x106f4c0);
PROVIDE(FSSetPosFile = 0x106f530);
PROVIDE(FSGetStatFile = 0x106f5a0);
PROVIDE(FSIsEof = 0x106f610);
PROVIDE(FSWriteFile = 0x106F228);
PROVIDE(FSFlushQuota = 0x106FAC8);


/* */
PROVIDE(FSGetStat = 0x0106fdc8);
PROVIDE(FSGetStatAsync = 0x0106bff4);
PROVIDE(FSOpenFileAsync = 0x0106a434);

PROVIDE(FSOpenDir = 0x0106f690);
PROVIDE(FSReadDir = 0x0106f780);
PROVIDE(FSCloseDir = 0x0106f700);
PROVIDE(FSChangeDir = 0x0106eefc);
PROVIDE(FSCloseDir = 0x0106f700);
PROVIDE(FSRemove  = 0x0106F960);

/* GX2 methods */
PROVIDE(GX2WaitForVsync = 0x1151964);

/* Socket methods */
PROVIDE(socket_lib_init = 0x10c02f4);
PROVIDE(socket = 0x10c21c8);
PROVIDE(socketclose = 0x10c2314);
PROVIDE(connect = 0x10c0828);
PROVIDE(send = 0x10c16ac);
PROVIDE(recv = 0x10c0aec);  

/* Standard library methods */
PROVIDE(memcpy = 0x1035a68);
PROVIDE(memset = 0x1035a54);
PROVIDE(MEMAllocFromDefaultHeapEx = 0x1004e9c0);
PROVIDE(MEMFreeToDefaultHeap = 0x100b487c);
PROVIDE(MEMAllocFromDefaultHeapEx = 0x1004e9c0);
PROVIDE(MEMAllocFromDefaultHeap = 0x100b4878);
PROVIDE(MEMFreeToDefaultHeap = 0x100b487c);

/* OS data */
PROVIDE(title_id = 0x100136D0);

/* OS methods */
PROVIDE(__os_snprintf = 0x102f09c);
PROVIDE(OSDynLoad_FindExport = 0x102b790);
PROVIDE(OSDynLoad_Acquire = 0x102a31c);