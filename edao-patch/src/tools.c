#include <stdio.h>
#include <string.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/appmgr.h>
#include <psp2/kernel/sysmem.h>

#include "tools.h"
#include "log.h"

#define SCE_APPMGR_APP_PARAM_TITLE_ID 12

int get_title_id(char title_id[VITA_TITLE_ID_LEN])
{
	if (!title_id)
	{
		return -1;
	}

	SceUID pid = sceKernelGetProcessId();
	if (pid < 0)
	{
		return -1;
	}

	int ret = sceAppMgrAppParamGetString(pid, SCE_APPMGR_APP_PARAM_TITLE_ID, title_id, VITA_TITLE_ID_LEN);
	if (ret < 0)
	{
		return -1;
	}


	
	return 0;
}

void hex_to_string(unsigned char* data, int data_len, char* string, int str_len)
{
	if (!data || !string || !data_len || !str_len)
	{
		return;
	}

	if (data_len * 3  > str_len - 1)
	{
		return;
	}

	int i = 0;
	for (; i != data_len; ++i)
	{
		snprintf(&string[i * 3], str_len - i * 3, "%02X ", data[i]);
	}
	string[(i + 1)* 3] = 0;
}


#define DUMP_LINE_BUFF_SIZE (16 * 3 + 1)
void dump_mem(char* tag, unsigned char* data, int len)
{
	if (!data || !len)
	{
		return;
	}

	if (tag)
	{
		DEBUG_PRINT("%s\n", tag);	
	}

	char buffer[DUMP_LINE_BUFF_SIZE] = {0};
	int i = 0;
	for (; i < len / 16; ++i)
	{
		hex_to_string(data + i * 16, 16, buffer, DUMP_LINE_BUFF_SIZE);
		DEBUG_PRINT("%s\n", buffer);
	}

	int pad = len % 16;
	if (pad)
	{
		hex_to_string(data + i * 16, pad, buffer, DUMP_LINE_BUFF_SIZE);
		DEBUG_PRINT("%s\n", buffer);
	}
}


void* vita_malloc(unsigned int size)
{
	if (!size)
	{
		return NULL;
	}

	unsigned int inner_size = size + sizeof(SceUID) + sizeof(unsigned int);
	SceUID block = sceKernelAllocMemBlock("asuka", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, ALIGN(inner_size, PAGE_SIZE), NULL);
	if (block <= 0)
	{
		return NULL;
	}

	void* base = NULL;
	sceKernelGetMemBlockBase(block, &base);
	if (!base)
	{
		sceKernelFreeMemBlock(block);
		return NULL;
	}

	*(unsigned int*)base = block;
	*(unsigned int*)((unsigned char*)base + sizeof(SceUID)) = size;

	void* user_base = (void*)((unsigned char*)base + sizeof(SceUID) + sizeof(unsigned int));

	return user_base;
}

void* vita_calloc(unsigned int num, unsigned int size)
{
	uint32_t buffer_size = num * size;
	if (!buffer_size)
	{
		return NULL;
	}

	void* buffer = vita_malloc(buffer_size);
	if (!buffer)
	{
		return NULL;
	}

	memset(buffer, 0, buffer_size);
	return buffer;
}

void* vita_realloc(void* ptr, unsigned int size)
{
	if (!ptr)
	{
		return vita_malloc(size);
	}

	if (!size)
	{
		vita_free(ptr);
		return NULL;
	}

	unsigned int old_size = *(unsigned int*)((unsigned char*)ptr - sizeof(unsigned int));
	if (old_size == size)
	{
		return ptr;
	}

	unsigned int copy_size = old_size < size ? old_size : size;
	void* buffer = vita_malloc(size);
	if (!buffer)
	{
		return NULL;
	}

	memcpy(buffer, ptr, copy_size);
	vita_free(ptr);
	return buffer;
}

void vita_free(void* mem)
{
	if (!mem)
	{
		return;
	}

	unsigned char* inner_mem = (unsigned char*)mem - sizeof(SceUID) - sizeof(unsigned int);
	SceUID block = 0;
	block = *(unsigned int*)inner_mem;
	if (block <= 0)
	{
		return;
	}
	sceKernelFreeMemBlock(block);
}


//string tools



int is_acsii_char(unsigned char c)
{
	if (c >= 0x20 && c <= 0x7E)
	{
		return 1;
	}
	if (c == '\t' || c == '\n' || c == '\r')
	{
		return 1;
	}
	return 0;
}

int is_sjis_char(unsigned char c)
{
	//half-width sjis char
	if (c >= 0xA1 && c <= 0xDF)
	{
		return 1;
	}

	//sjis first char
	if ((c >= 0x81 && c <= 0x9F) ||
		(c >= 0xE0 && c <= 0xEF))
	{
		return 1;
	}

	//sjis second char
	if ((c >= 0x40 && c <= 0x7E) ||
		(c >= 0x80 && c <= 0xFC))
	{
		return 1;
	}
	return 0;
}

int is_opcode(unsigned char c)
{
	return (!is_acsii_char(c) && !is_sjis_char(c));
}


#define EDAO_STR_BUFF_LEN 1024

char* rip_string(char* old_str)
{
	static char string_buff[EDAO_STR_BUFF_LEN] = {0};

	if (!old_str)
	{
		return NULL;
	}

	int len = strlen(old_str);
	int j = 0;
	for(int i = 0; i != len; ++i)
	{
		if (!is_opcode(old_str[i]))
		{
			string_buff[j++] = old_str[i];
		}
	}
	string_buff[j] = 0;
	return string_buff;
}




