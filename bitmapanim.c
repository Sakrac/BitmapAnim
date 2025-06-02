#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <assert.h>

#define MAX_ARGS 32
#if defined(_MSC_VER)
#define FOpen(f, n, t) (fopen_s(&f, n, t) == 0)
#define StrNCmp(s1, s2, cnt) _strnicmp(s1, s2, cnt)
#else
#define FOpen(f, n, t) (f = fopen(n, t))
#define StrNCmp(s1, s2, cnt) strncmp(s1, s2, cnt)
#define _MAX_PATH 2048
#endif

// can be overwritten by -palette=<image>
static uint8_t palette[16][3] = {
	{ 0, 0, 0 },		// #000000
	{ 255, 255, 255 },	// #FFFFFF
	{ 137, 64, 54 },	// #880000
	{ 122, 191, 199 },	// #AAFFEE
	{ 138, 70, 174 },	// #CC44CC
	{ 104, 169, 65 },	// #00CC55
	{ 62, 49, 162 },	// #0000AA
	{ 208, 220, 113 },	// #EEEE77
	{ 144, 95, 37 },	// #DD8855
	{ 92, 71, 0 },		// #664400
	{ 187, 119, 109 },	// #FF7777
	{ 85, 85, 85 },		// #555555
	{ 128, 128, 128 },	// #808080
	{ 172, 234, 136 },	// #AAFF66
	{ 124, 112, 218 },	// #0088FF
	{ 171, 171, 171 }	// #ABABAB
};

char* GetSwitch(const char* match, char** swtc, int swtn)
{
	int l = (int)strlen(match);
	while (swtn)
	{
		if (StrNCmp(match, *swtc, l) == 0)
		{
			if ((*swtc)[l] == '=') return *swtc + l + 1;
			else if (!(*swtc)[l]) return *swtc;
		}
		++swtc;
		--swtn;
	}
	return 0;
}

typedef struct {
	uint16_t ch;
	uint8_t n;
} ModBytes;

#define MAX_MODS 256

int main(int argc, char* argv[]) {
	const char* args[MAX_ARGS];
	char* swtc[MAX_ARGS];
	int argn = 0;
	int swtn = 0;
	for (int a = 0; a < argc; ++a) {
		if (argv[a][0] == '-') { swtc[swtn++] = argv[a] + 1; }
		else { args[argn++] = argv[a]; }
	}

	const char* bgcol = GetSwitch("bg", swtc, swtn);
	uint8_t bg = bgcol ? (uint8_t)atoi(bgcol) : 0;

	// numbered bitmaps name##.png
	// sizes should be identical
	char file[_MAX_PATH];
	int found = 0;
	int num = 0;
	uint8_t* prev = 0;
	int w0, h0, bpp0;
	ModBytes mods[MAX_MODS];
	uint8_t frames[256];
	uint8_t* stripes[256];
	int nMods = 0;
	int nFrames = 0;
	int nStripes = 0;
	do {
		found = 0;
		sprintf_s(file, sizeof(file), "%s%d.png", args[1], num+1);
		int w, h, bpp;
		uint8_t* raw = stbi_load(file, &w, &h, &bpp, 0), *src = raw;
		if (raw == 0) { break; }
		uint8_t* img = (uint8_t*)malloc((size_t)w * (size_t)h), * dst = img;
		if (raw && img) {
			for (int p = 0, np = w * h; p < np; ++p) {
				int r = src[0], g = src[1], b = src[2];
				int bst = (1 << 30) - 1;
				int bc = 0;
				for (int c = 0; c < 16; ++c) {
					int or = r - palette[c][0];
					int og = g - palette[c][1];
					int ob = b - palette[c][2];
					int o = or *or +og * og + ob * ob;
					if (o < bst) {
						bst = o;
						bc = c;
					}
				}
				*dst++ = (uint8_t)bc;
				src += bpp;
			}
		} else {
			return -1;
		}
		free(raw);
		size_t wc = w / 8;
		size_t hc = h / 8;

		if (!prev) {
			bpp0 = bpp;
			w0 = w;
			h0 = h;
			prev = img;
			found = 1;
		} else if (w0 == w && h0 == h) {

			uint16_t ch = 0;
			frames[nFrames++] = nMods;
			size_t last_y_change = 0xffff;

			// scan for changes from prev
			for (size_t y = 0; y < hc; ++y) {
				const uint8_t* pc = prev + y * 8 * w;
				const uint8_t* cc = img + y * 8 * w;
				for (size_t x = 0; x < wc; ++x) {

					// detect if identical (skip)
					const uint8_t* pp = pc + x * 8, *cp = cc + x * 8;
					int startY = -1, endY = -1;
					for (size_t yd = 0; yd < 8; ++yd) {
						for (size_t xd = 0; xd < 4; ++xd) {
							if (pp[xd*2] != cp[xd*2]) {
								if (startY < 0) { startY = (int)yd; endY = startY; }
								else { endY = (int)yd; }
								break;
							}
						}
						pp += w; cp += w;
					}
					if (startY >= 0) {
						if (last_y_change != y) { printf("%02d: ", (int)y); last_y_change = y; }
						printf("%02d ", (int)x);
						if (nMods > 0 && (mods[nMods - 1].ch+mods[nMods-1].n) == ch) {
							mods[nMods - 1].n++;
						} else {
							mods[nMods].ch = ch; mods[nMods].n = 1;
							++nMods;
						}
					}
					++ch;
				}
				if (last_y_change == y) { printf("\n"); }
			}

			// generate the stripes for this frame
			for (int s = frames[nFrames - 1]; s < nMods; ++s) {
				int chars = mods[s].n;
				// bytes: 1 (# chars), 2 (scrn), #chars (scrn), #chars (col),
				// 2 (bitmap), 8*#chars
				int size = 1 + 2 + chars + chars + 2 + 8 * chars;
				uint8_t* stripe = (uint8_t*)calloc(1, size), *so = stripe;
				stripes[nStripes++] = stripe;
				*so++ = chars;
				uint16_t scrn = 0x4000 + mods[s].ch - 1;
				uint16_t bm = 0x6000 + mods[s].ch * 8;
				*so++ = (uint8_t)scrn;
				*so++ = (uint8_t)(scrn >> 8);
				uint8_t* scro = so, * colo = so + chars;
				so += 2 * chars;
				*so++ = (uint8_t)bm;
				*so++ = (uint8_t)(bm >> 8);
				for (int ch = mods[s].ch, ec = ch + chars; ch < ec; ++ch) {
					uint8_t* chr = img + (ch / 40) * 8 * w + (ch % 40) * 8;
					uint8_t hist[16];
					memset(hist, 0, 16);
					uint8_t cols[4] = { bg, 2, 3, 4 };
					for (int py = 0; py < 8; ++py) for(int px=0; px<4; ++px) {
						hist[chr[(px * 2 + py * w)] & 15]++;
					}
					hist[bg] = 0;
					for (int cn = 1; cn < 4; ++cn) {
						uint8_t top = 0, ct = cols[cn];
						for (int h = 0; h < 16; ++h) {
							if (hist[h] > top) {
								top = hist[h];
								ct = h;
							}
						}
						hist[ct] = 0;
						cols[cn] = ct;
					}
					*scro++ = (cols[1] << 4) | cols[2];
					*colo++ = cols[3];
					for (int cy = 0; cy < 8; ++cy) {
						uint8_t b = 0;
						for (int cx = 0; cx < 4; ++cx) {
							b <<= 2;
							uint8_t cl = chr[cx * 2 + cy * w];
							uint8_t ci = 0;
							for (; ci < 4; ++ci) { if (cols[ci] == cl) break; }
							b |= ci & 3;
						}
						*so++ = b;
					}
				}
				assert((so - stripe) == size);
			}

			free(prev);

//			for (int i = 0; i < nMods; ++i) {
//				printf("change: %d, %d + %d\n", mods[i].ch % 40, mods[i].ch / 40, mods[i].n);
//			}
			prev = img;
			found = 1;
		} else {
			printf("image %d is not correct size\n", num);
		}
		++num;
	} while (found);

	// put in total number of mods at the end of the frames list
	frames[nFrames] = nMods;

	const char* expfile = GetSwitch("out", swtc, swtn);
	if (expfile) {
		FILE* f = 0;
		if (fopen_s(&f, expfile, "w") == 0) {
			fprintf(f, "\t; stripes generated from %s\n", args[1]);
			fprintf(f, "\tdc.b %d ; number of frames in this anim\n", nFrames);
			for (int r = 0; r < nFrames; ++r) {
				fprintf(f, "\n\t ; frame %d\n", r+1);
				uint8_t ns = frames[r + 1] - frames[r];// *frame++;
				fprintf(f, "\tdc.b %d ; this frame has %d stripes\n", ns, ns);

				for (int s = 0; s < ns; ++s) {
					uint8_t* frame = stripes[frames[r]+s];
					uint8_t nc = *frame++;
					fprintf(f, "\tdc.b %d ; stripe %d is %d characters\n", nc, s, nc);
					uint16_t sa = *frame++; sa |= ((uint16_t)*frame++) << 8;
					fprintf(f, "\tdc.w $%x ; screen addr = $%x (%d, %d)\n\tdc.b ", sa, sa,
						(sa&0x3ff)%40, (sa&0x3ff)/40);
					for (int sc = 0; sc < nc; ++sc) {
						fprintf(f, "$%02x%s", *frame++, sc == (nc - 1) ? " ; screen data\n\tdc.b " : ", ");
					}
					for (int sc = 0; sc < nc; ++sc) {
						fprintf(f, "$%02x%s", *frame++, sc == (nc - 1) ? " ; color data\n" : ", ");
					}
					uint16_t ba = *frame++; ba |= ((uint16_t)*frame++) << 8;
					fprintf(f, "\tdc.w $%x ; bitmap addr = $%x (%d, %d)\n", ba, ba,
						((ba>>3)&0x3ff)%40, ((ba>>3)&0x3ff)/40);
					for (int sc = 0; sc < nc; ++sc) {
						fprintf(f, "\tdc.b ");
						for (int ch = 0; ch < 8; ++ch) {
							fprintf(f, "$%02x%s", *frame++, ch == 7 ? "" : ", ");
						}
						fprintf(f, " ; char %d\n", sc);
					}
				}
			}
			fclose(f);
		}
	}

	// export
/*
 ; each frame
	dc.b stripes
 ; each stripe..
	dc.b chars
	dc.b screenlo, screenhi
	dc.b screen_chars, ...
	dc.b color_chars, ...
	dc.b bitmaplo, bitmaphi
	dc.b bitmapdata...

*/




	return 0;
}