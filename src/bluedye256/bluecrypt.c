#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bluedye.c"
#include "reddye_kdf.c"

int keylen = 32;
int k[32] = {0};
int s[256];
int j = 0;
int temp;

void keysetup(unsigned char *key, unsigned char *nonce) {
    int c;
    for (c = 0; c < 256; c++) {
        s[c] = c; 
    }
    for (c=0; c < keylen; c++) {
        k[c] = (k[c] + key[c]) & 0xff;
        j = (j + k[c]) & 0xff; }
    for (c = 0; c < 768; c++) {
        k[c % keylen] = (k[c % keylen] + j) & 0xff;
        j = (j + k[c % keylen] + c) & 0xff; }
        temp = s[c & 0xff];
	s[c & 0xff] = s[j];
	s[j] = temp;
    for (c = 0; c < strlen(nonce); c++) {
        k[c] = (k[c] + nonce[c]) & 0xff;
        j = (j + k[c]) & 0xff; }
    for (c = 0; c < 768; c++) {
        k[c % keylen] = (k[c % keylen] + j) & 0xff;
        j = (j + k[c % keylen] + c) & 0xff; }
        temp = s[c & 0xff];
	s[c & 0xff] = s[j];
	s[j] = temp;
}

void usage() {
    printf("bluedye <encrypt/decrypt> <input file> <output file> <password>\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    FILE *infile, *outfile, *randfile;
    char *in, *out, *mode;
    unsigned char *data = NULL;
    unsigned char *buf = NULL;
    int x = 0;
    int i = 0;
    int output;
    int ch;
    int buflen = 131072;
    int bsize;
    unsigned char *key[keylen];
    unsigned char *password;
    int nonce_length = 8;
    int iterations = 10000;
    unsigned char *salt = "RedDyeCipher";
    unsigned char *nonce[nonce_length];
    unsigned char block[buflen];
    if (argc != 5) {
        usage();
    }
    mode = argv[1];
    in = argv[2];
    out = argv[3];
    password = argv[4];
    infile = fopen(in, "rb");
    fseek(infile, 0, SEEK_END);
    long fsize = ftell(infile);
    fseek(infile, 0, SEEK_SET);
    outfile = fopen(out, "wb");
    int c = 0;
    if (strcmp(mode, "encrypt") == 0) {
        long blocks = fsize / buflen;
        long extra = fsize % buflen;
        if (extra != 0) {
            blocks += 1;
        }
	bluedye_random(nonce, nonce_length);
        fwrite(nonce, 1, nonce_length, outfile);
	bluedye_kdf(password, key, salt, iterations, keylen);
        keysetup(key, nonce);
        for (int d = 0; d < blocks; d++) {
            fread(block, buflen, 1, infile);
            bsize = sizeof(block);
            for (int b = 0; b < bsize; b++) {
                k[i] = (k[i] + k[(i + 1) % keylen] + j) & 0xff;
                j = (j + k[i] + c) & 0xff;
		output = s[j] ^ k[i];
                block[b] = block[b] ^ output;
                c = (c + 1) & 0xff;
                i = (i + 1) % keylen;
		temp = s[c];
		s[c] = s[j];
		s[j] = temp;
            }
            if (d == (blocks - 1) && extra != 0) {
                bsize = extra;
            }
            fwrite(block, 1, bsize, outfile);
        }
    }
    else if (strcmp(mode, "decrypt") == 0) {
        long blocks = (fsize - nonce_length) / buflen;
        long extra = (fsize - nonce_length) % buflen;
        if (extra != 0) {
            blocks += 1;
        }
        fread(nonce, 1, nonce_length, infile);
	bluedye_kdf(password, key, salt, iterations, keylen);
        keysetup(key, nonce);
        for (int d = 0; d < blocks; d++) {
            fread(block, buflen, 1, infile);
            bsize = sizeof(block);
            for (int b = 0; b < bsize; b++) {
                k[i] = (k[i] + k[(i + 1) % keylen] + j) & 0xff;
                j = (j + k[i] + c) & 0xff;
		output = s[j] ^ k[i];
                block[b] = block[b] ^ output;
                c = (c + 1) & 0xff;
		i = (i + 1) % keylen;
		temp = s[c];
		s[c] = s[j];
		s[j] = temp;
            }
            if ((d == (blocks - 1)) && extra != 0) {
                bsize = extra;
            }
            fwrite(block, 1, bsize, outfile);
        }
    }
    fclose(infile);
    fclose(outfile);
    return 0;
}
