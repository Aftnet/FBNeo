// input encoder for replay.cpp  dink aug.2022

#include <stdio.h>
#include "burner.h"
#include "burnint.h"

static FILE *input_f = NULL;
static INT32 input_f_embed_pos = 0; // embeded position in file

static UINT8 *buffer = NULL;
static INT32 buffer_pos;
static INT32 buffer_size;
static INT32 buffer_eof = 0;

static void inputbuf_init() // called internally!
{
	buffer_size = 1024 * 1024; // 1meg starting
	buffer = (UINT8*)malloc(buffer_size);
	memset(buffer, 0, buffer_size);

	buffer_pos = 0;
	buffer_eof = 0;
}

void inputbuf_exit()
{
	free(buffer);

	buffer = NULL;
	buffer_eof = 1;
}

#define FREEZE_EXTRA (sizeof(INT32)*4)

INT32 inputbuf_freeze(UINT8 **buf, INT32 *size)
{
	UINT8 *b = (UINT8*)malloc(buffer_pos + FREEZE_EXTRA);
	*buf = b;

	if (b == NULL) return 1;

	memcpy(b, &buffer_pos, sizeof(buffer_pos));
	b += sizeof(buffer_pos);

	memcpy(b, buffer, buffer_pos);
	*size = buffer_pos + FREEZE_EXTRA;

	return 0;
}

INT32 inputbuf_unfreeze(UINT8 *buf, INT32 size)
{
	if (size > buffer_size) {
		buffer = (UINT8*)realloc(buffer, size);

		if (buffer == NULL) return 1;

		buffer_size = size;
	}

	memcpy(&buffer_pos, buf, sizeof(buffer_pos));
	memcpy(buffer, buf + sizeof(buffer_pos), size - FREEZE_EXTRA);

	//bprintf(0, _T("inputbuf unfrozen, pos %d\n"), buffer_pos);

	return 0;
}

INT32 inputbuf_embed(FILE *fp)
{
	input_f = fp;
	input_f_embed_pos = ftell(fp);

	return 0;
}

void inputbuf_load()
{
	inputbuf_init();

	// seek to embedded position
	fseek(input_f, input_f_embed_pos, SEEK_SET);

	INT32 packet_len = 0;
	INT32 data_len = 0;

	fread(&packet_len, sizeof(packet_len), 1, input_f); // packet length (data len rounded to multiple of 4 / dword aligned)
	fread(&data_len, sizeof(data_len), 1, input_f);

	bprintf(0, _T("inputbuf_load() - loading %d bytes (%d data)\n"), packet_len, data_len);

	buffer = (UINT8*)realloc(buffer, packet_len);
	buffer_size = data_len;

	fread(buffer, packet_len, 1, input_f);
}

void inputbuf_save()
{
	// seek to embedded position
	fseek(input_f, input_f_embed_pos, SEEK_SET);

	INT32 packet_len = (buffer_pos + 3) & -4;
	INT32 data_len = buffer_pos;
	INT32 difference = packet_len - data_len;
	INT32 align = 0;

	fwrite(&packet_len, sizeof(packet_len), 1, input_f);
	fwrite(&data_len, sizeof(data_len), 1, input_f);

	bprintf(0, _T("inputbuf_save() - saving %d bytes (%d data)\n"), packet_len, data_len);

	fwrite(buffer, data_len, 1, input_f);
	if (difference) {
		fwrite(&align, difference, 1, input_f);
		bprintf(0, _T("alignment of + %d\n"), difference);
	}
}

INT32 inputbuf_eof()
{
	return buffer_eof;
}

void inputbuf_addbuffer(UINT8 c)
{
	if (buffer == NULL) {
		bprintf(0, _T("inputbuf_addbuffer: init!\n"));
		inputbuf_init();
	}

	if (buffer_pos + 1 < buffer_size) {
		// yay!
	} else {
		// realloc buffer!!
		INT32 previous_size = buffer_size;
		buffer_size *= 2;
		buffer = (UINT8*)realloc(buffer, buffer_size);
		bprintf(0, _T("inputbuf_addbuffer: reallocing buffer, was / new:  %d   %d\n"), previous_size, buffer_size);
	}

	buffer[buffer_pos++] = c;
}

UINT8 inputbuf_getbuffer()
{
	if (buffer_pos + 1 < buffer_size) {
		return buffer[buffer_pos++];
	}

	// implied else
	buffer_eof = 1;
	return buffer[buffer_pos];
}
