#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/time.h>
#include "ipdb.h"

int main(const int argc, const char ** argv) {
	int i1, i2, i3, i4;
	char * defaultDB = "ipv4_cn.ipdb";
	char * db;
	char ip[16] = {0};
	int start = 0;
	int end = 255;

	if (argc == 1) {
		db = defaultDB;
	}
	else {
		db = (char *) argv[1];
		if (argc == 4) {
			start = atoi(argv[2]);
			end = atoi(argv[3]);
		}
	}

	//printf("=========start=%d, end=%d==========\n", start, end);

    ipdb_reader *reader;
    int err = ipdb_reader_new(db, &reader);
    //printf("new ipdb reader err: %d\n", err);
    if (!err) {
        char body[512];
        char tmp[64];
        char *lang[2];
		char info[128] = {0};
		/*
        printf("ipdb build time: %li\n", reader->meta->build_time);
        printf("ipdb ipv4 support: %i\n", ipdb_reader_is_ipv4_support(reader));
        printf("ipdb ipv6 support: %i\n", ipdb_reader_is_ipv6_support(reader));
        printf("ipdb language: ");
        for (int i = 0; i < reader->meta->language_length; ++i) {
            printf("%s ", reader->meta->language[i].name);
        }
        printf("\n");
        printf("ipdb fields: ");
        for (int i = 0; i < reader->meta->fields_length; ++i) {
            printf("%s ", reader->meta->fields[i]);
        }
        printf("\n");

        lang[0] = "CN";
        lang[1] = "EN";
        for (int i = 0; i < 1; ++i) {
            err = ipdb_reader_find(reader, "182.139.215.89", lang[i], body);
            printf("%s find err: %d\n", lang[i], err);
            if (err) continue;
            printf("%s\n", body);
            int f = 0, p1 = 0, p2 = -1;
            do {
                if (*(body + p1) == '\t' || !*(body + p1)) {
                    strncpy(tmp, body + p2 + 1, (size_t) p1 - p2);
                    tmp[p1 - p2] = 0;
                    printf("%d: %s: %s\n", f + 1, reader->meta->fields[f], tmp);
                    p2 = p1;
                     ++f;
                } 
            } while (*(body + p1++));
        }
		*/

		for (i1 = start; i1 <= end; ++i1) {
			for (i2 = 0; i2 <= 255; ++i2) {
				for (i3 = 0; i3 <= 255 ; ++i3) {
					for (i4 = 0; i4 <=  255; ++i4) {
						sprintf(ip, "%d.%d.%d.%d" , i1, i2, i3, i4);
						err = ipdb_reader_find(reader, ip, "CN", body);
						if (err) continue;
						if (memcmp(body, "保留", sizeof("保留") - 1) == 0) continue;
			 			//printf("%s %s\n", ip, body);
						int f = 0, p1 = 0, p2 = -1;
						int offset = 0;
						int d = 0;
						do {
							if (*(body + p1) == '\t' || !*(body + p1)) {
								strncpy(tmp, body + p2 + 1, (size_t) p1 - p2);
								d = p1 - p2;
								if (tmp[d-1] == '\t') {
									tmp[d-1] = 0;
								} else {
									tmp[d] = 0;
								}

								//printf("%d: %s: %s\n", f + 1, reader->meta->fields[f], tmp);
								++f;
								if (f == 1 || f == 2 || f == 12) {
									//printf("strlen(tmp) = %ld\n", strlen(tmp));
									offset += sprintf(info + offset, "<>%s", tmp[0] == 0?"-":tmp);
									//printf("+++++offset=%d\n", offset);
									//printf("+++++info=%s\n", info);
								}

								if (f == 12) break;

								p2 = p1;
							} 
						} while (*(body + p1++));

						info[offset] = 0;
						printf("%s%s\n", ip, info);
			 		}
			 	}
			} 
		}
		
		/*
        int x = 1000000;
        struct timeval start, end;
        gettimeofday(&start, NULL);
        do {
            ipdb_reader_find(reader, "182.139.215.89", "CN", body);
        } while (--x);
        gettimeofday(&end, NULL);
        long timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        printf("time used: %ldus.\n", timeuse);
		*/
    }
    ipdb_reader_free(&reader);
    return 0;
}
