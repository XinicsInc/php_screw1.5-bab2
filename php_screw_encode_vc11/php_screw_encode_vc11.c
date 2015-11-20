// zencode_test_write.cpp : �ܼ� ���� ���α׷��� ���� �������� �����մϴ�.
//

#include "stdafx.h"

#include <string.h>
#include <sys/stat.h>

#include "php_screw.h"
#include "my_screw.h"

int main(int argc, char* argv[])
{
	FILE *fp;
	struct stat stat_buf;
	char *datap, *screw_datap;
	int datalen, screw_datalen;
	int cryptkey_len = sizeof pm9screw_mycryptkey / 2;
	char screw_filename[256];
	int i;

	// �������� �غ�
	if (argc != 2) {
		fprintf(stderr, "Usage: filename.\n");
		exit(0);
	}

	fp = fopen((char *)argv[1], "r");
	if (fp == NULL) {
		fprintf(stderr, "File not found(%s)\n", argv[1]);
		exit(0);
	}

#ifdef WIN32
	fstat(_fileno(fp), &stat_buf);
#else
	fstat(fileno(fp), &stat_buf);
#endif

	datalen = stat_buf.st_size;
	//datap = (char*)malloc(datalen + PM9SCREW_LEN);
	datap = (char*)malloc(datalen);
	memset(datap, 0, datalen);
	fread(datap, datalen, 1, fp);
	fclose(fp);

	datalen = strlen(datap); // ��ȣȭ�� Filesize�� ���� ���̿� �޶� strlen���� ���� ����.

	// ��ȣȭ������ ó��
	screw_datap = zencode(datap, datalen, &screw_datalen);
	for (i = 0; i < screw_datalen; i++) {
		screw_datap[i] = (char)pm9screw_mycryptkey[(screw_datalen - i) % cryptkey_len] ^ (~(screw_datap[i]));
	}
#ifdef WIN32
	strncpy_s(screw_datap, screw_datalen + 1, screw_datap, _TRUNCATE);
#else
	strncpy(screw_datap, screw_datap, screw_datalen + 1);
#endif

	sprintf(screw_filename, "%s.screwed", argv[1]);
	if (memcmp(datap, PM9SCREW, PM9SCREW_LEN) == 0) {
		fprintf(stderr, "Already Crypted(%s)\n", screw_filename);
		exit(0);
	}

	fp = fopen(screw_filename, "w");
	fwrite(PM9SCREW, PM9SCREW_LEN, 1, fp);
	fwrite(screw_datap, screw_datalen, 1, fp);
	fclose(fp);

	return 0;
}
