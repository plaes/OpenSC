
/* Copyright (C) 2001  Juha Yrj�l� <juha.yrjola@iki.fi> 
 * All rights reserved.
 */

#include "sc-test.h"
#include "opensc.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

int main(int argc, char *argv[])
{
	int i, c, r;
	int freq[39];
	struct timeval tv1, tv2;
	u8 buf[14];

	i = sc_test_init(&argc, argv);

	for (i = 0; i < 39; i++)
		freq[i] = 0;
	c = 0;
	while (1) {
		u8 alkiot[39];
		
		for (i = 0; i < 39; i++) {
			alkiot[i] = i + 1;
		}
		if (c == 0)
			gettimeofday(&tv1, NULL);
		sc_lock(card);
		r = sc_get_challenge(card, buf, 14);
		sc_unlock(card);
		if (r == 0) {
			int i, jaljella = 39;
			
			printf("Lottorivi: ");
			for (i = 0; i < 7; i++) {
				unsigned short s = buf[2*i] + (buf[2*i+1] << 8);
				int lot = s % (jaljella+1);
				int num = alkiot[lot];
				
				alkiot[lot] = alkiot[jaljella-1];
				jaljella--;
				
				freq[num-1]++;
				
				printf("%3d ", num);
			}
			printf("\n");
		} else {
			fprintf(stderr, "get_random() failed: %s\n", sc_strerror(r));
			sc_test_cleanup();
			return 1;
		}
		c++;
		if (c == 50) {
			unsigned long long foo, foo2;
			gettimeofday(&tv2, NULL);
			foo = tv2.tv_sec * 1000 + tv2.tv_usec / 1000;
			foo2 = tv1.tv_sec * 1000 + tv1.tv_usec / 1000;
			printf("Time per one: %lld ms\n", (foo - foo2)/50);
			printf("Frequencies:\n");
			for (i = 0; i < 39; i++) {
				printf("%3d: %-5d", i+1, freq[i]);
				if (((i+1) % 10) == 0)
					printf("\n");
			}
			printf("\n");
			c = 0;
		}
	}
	sc_test_cleanup();
	return 0;
}
