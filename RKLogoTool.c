/*   RKLogoTool.c
 *      
 *   Copyright 2014 Bartosz Jankowski
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */
 
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <libgen.h>

struct color {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

unsigned char data_name[] = {
	0x6C, 0x6F, 0x67,
	0x6F, 0x5F, 0x52,
	0x4B, 0x6C, 0x6F,
	0x67, 0x6F, 0x5F,
	0x64, 0x61, 0x74,
	0x61
};

unsigned char clut_name[] = {
	0x6C, 0x6F, 0x67,
	0x6F, 0x5F, 0x52,
	0x4B, 0x6C, 0x6F,
	0x67, 0x6F, 0x5F,
	0x63, 0x6C, 0x75,
	0x74
};

#define LINUX_LOGO_MONO		1	/* monochrome black/white */
#define LINUX_LOGO_VGA16	2	/* 16 colors VGA text palette */
#define LINUX_LOGO_CLUT224	3	/* 224 colors */
#define LINUX_LOGO_GRAY256	4	/* 256 levels grayscale */

static const char *logo_types[LINUX_LOGO_GRAY256+1] = {
    [LINUX_LOGO_MONO] = "LINUX_LOGO_MONO",
    [LINUX_LOGO_VGA16] = "LINUX_LOGO_VGA16",
    [LINUX_LOGO_CLUT224] = "LINUX_LOGO_CLUT224",
    [LINUX_LOGO_GRAY256] = "LINUX_LOGO_GRAY256"
};

#define MAX_LINUX_LOGO_COLORS	224


static int logo_type = LINUX_LOGO_CLUT224;
static unsigned int logo_width;
static unsigned int logo_height;
static struct color **logo_data;
static struct color logo_clut[MAX_LINUX_LOGO_COLORS];
static unsigned int logo_clutsize;

static inline int is_black(struct color c)
{
    return c.red == 0 && c.green == 0 && c.blue == 0;
}

static inline int is_white(struct color c)
{
    return c.red == 255 && c.green == 255 && c.blue == 255;
}

static inline int is_gray(struct color c)
{
    return c.red == c.green && c.red == c.blue;
}

static inline int is_equal(struct color c1, struct color c2)
{
    return c1.red == c2.red && c1.green == c2.green && c1.blue == c2.blue;
}

static unsigned int get_number(FILE *fp)
{
    int c, val;

    /* Skip leading whitespace */
    do {
	c = fgetc(fp);
	if (c == EOF)
	    fprintf(stderr, "end of file\n");
	if (c == '#') {
	    /* Ignore comments 'till end of line */
	    do {
		c = fgetc(fp);
		if (c == EOF)
		    fprintf(stderr, "end of file\n");
	    } while (c != '\n');
	}
    } while (isspace(c));

    /* Parse decimal number */
    val = 0;
    while (isdigit(c)) {
	val = 10*val+c-'0';
	c = fgetc(fp);
	if (c == EOF)
	    fprintf(stderr, "end of file\n");
    }
    return val;
}
uint16_t swap_uint16( uint16_t val ) 
{
    return (val << 8) | (val >> 8 );
}

static unsigned int get_number255(FILE *fp, unsigned int maxval)
{
    unsigned int val = get_number(fp);
    return (255*val+maxval/2)/maxval;
}

static int read_ppm(char *szFile)
{
	FILE *fp;
	unsigned int i, j;
	int magic;
	unsigned int maxval;

	/* open image file */
	fp = fopen(szFile, "r");
	if (!fp)
	{
		printf("Cannot open file %s: %s\n", szFile, strerror(errno));
		return 0;
	}
    magic = fgetc(fp);
    if (magic != 'P') 
    {
		printf("%s is not a PNM file\n", szFile);
		fclose(fp);
		return 0;
	}
    
    magic = fgetc(fp);
	if(magic == 3) 
	{
		fprintf(stderr,"Image type %d is not supported. Please use ASCII PPM\n", magic);
		fclose(fp);
		return 0;
	}

	logo_width = get_number(fp);
	logo_height = get_number(fp);

	logo_data = (struct color **)malloc(logo_height * sizeof(struct color *));
	if (!logo_data) 
	{
		fprintf(stderr, "%s\n", strerror(errno));
		return 0;
	}
	for (i = 0; i < logo_height; i++)
	{
		logo_data[i] = (struct color *)malloc(logo_width * sizeof(struct color));
		if (!logo_data[i]) 
		{
			fprintf(stderr, "%s\n", strerror(errno));
			return 0;
		}
	}

	maxval = get_number(fp);
	for (i = 0; i < logo_height; i++)
		for (j = 0; j < logo_width; j++) 
		{
			logo_data[i][j].red = get_number255(fp, maxval);
			logo_data[i][j].green = get_number255(fp, maxval);
			logo_data[i][j].blue = get_number255(fp, maxval);
		}
    fclose(fp);
    return 1;
}

static void free_clut_data()
{
	if(!logo_data) 
		return;
	int i;
	for (i = 0; i < logo_height; i++)
	{
		free(logo_data[i]);
	}
	free(logo_data);
}

static int write_clut(unsigned int address, char* szKernelFile)
{
	unsigned int i, j, k;
	unsigned char b;
	logo_clutsize = 0;

	for (i = 0; i < logo_height; i++)
		for (j = 0; j < logo_width; j++) 
		{
			for (k = 0; k < logo_clutsize; k++)
				if (is_equal(logo_data[i][j], logo_clut[k]))
					break;
					
			if (k == logo_clutsize)
			{
				if (logo_clutsize == MAX_LINUX_LOGO_COLORS) 
				{
					fprintf(stderr, 
						"Image has more than %d colors\n"
						"Use ppmquant to reduce the number of colors\n",
						MAX_LINUX_LOGO_COLORS);
					free_clut_data();
					return;
				}
				logo_clut[logo_clutsize++] = logo_data[i][j];
			}
		}
	
	FILE *fp = fopen(szKernelFile, "rb+");	
	if(!fp)
	{
		fprintf(stderr, "Cannot open %s for writing: %s\n", szKernelFile, strerror(errno));
		free_clut_data();
		exit(EXIT_FAILURE);
	}
	
	if(fseek(fp, address, SEEK_SET))
	{
		fprintf(stderr, "Cannot set cursor on kernel.img @0x%X\n", address);
		free_clut_data();
		exit(EXIT_FAILURE);
	}
	
	//Get size of old clut
	fseek(fp, 0x10, SEEK_CUR);
	
	unsigned char nClutItems = fgetc(fp);
	int nClutSectionSize = nClutItems*3;
	fprintf(stderr, "Original number of colors: %d\n", nClutItems);
	fseek(fp, nClutSectionSize, SEEK_CUR);
	
	for(i = 0; i < 0x500; ++i, ++nClutSectionSize)
	{
		if(fgetc(fp) == 'l')
		{
			++nClutSectionSize;
			if(fgetc(fp) == 'o')
			{
				++nClutSectionSize;
				if(fgetc(fp) == 'g')
				{
					++nClutSectionSize;
					if(fgetc(fp) == 'o')
					{
						++nClutSectionSize;
						break;
					}
				}
			}
		}
		
		if(i == 0x499) 
		{
			fprintf(stderr, "Critical error: rk_logo_data section not found!\n");
			free_clut_data();
			exit(EXIT_FAILURE);
		}
	}
	
	nClutSectionSize-=8;
	
	
	
	// if(logo_clutsize > nClutItems)
	// {
		// fprintf(stderr, 
				// "Original image has %d colors, but your image has %d\n"
				// "You need oto use ppmquant to reduce the number of colors\n",
				// nClutItems, logo_clutsize);
			// free_clut_data();
			// return;
	// }
		
	if(fseek(fp, address, SEEK_SET))
	{
		fprintf(stderr, "Cannot set cursor on kernel.img @0x%X\n", address);
		free_clut_data();
		exit(EXIT_FAILURE);
	}
	
	for (i = 0; i < sizeof(clut_name); i++)
	{
			fputc(clut_name[i],fp);
	}
	
	fputc(logo_clutsize,fp);

	for (i = 0; i < logo_clutsize; i++)
	{
		fputc(logo_clut[i].red,fp);
		fputc(logo_clut[i].green,fp);
		fputc(logo_clut[i].blue,fp);
		nClutSectionSize-=3;
	}

	for (i = 0; i < nClutSectionSize; ++i)
	{
		fputc(0x20,fp);
	}
	
	fprintf(stderr, "Written CLUT hearder. Number of colors: %d\n", logo_clutsize);
	if(fgetc(fp) == 0)
	{
		fseek(fp, -1, SEEK_CUR);
		fputc(0,fp);
		fputc(0,fp);
	}
	else
	fseek(fp, -1, SEEK_CUR);
	
	b = (unsigned char)(logo_width >> 8);
	fputc(b,fp);
	b = (unsigned char)logo_width;
	fputc(b,fp);
	b = (unsigned char)(logo_height >> 8);
	fputc(b,fp);
	b = (unsigned char)logo_height;
	fputc(b,fp);

			
	for (i = 0; i < sizeof(data_name); i++)
	{
		fputc(data_name[i],fp);
	}
	
	b = (unsigned char)(logo_width >> 8);
	fputc(b,fp);
	b = (unsigned char)logo_width;
	fputc(b,fp);
	b = (unsigned char)(logo_height >> 8);
	fputc(b,fp);
	b = (unsigned char)logo_height;
	fputc(b,fp);

	/* write logo data */
	for (i = 0; i < logo_height; i++)
		for (j = 0; j < logo_width; j++) {
	 		for (k = 0; k < logo_clutsize; k++)
				if (is_equal(logo_data[i][j], logo_clut[k]))
					break;
			fputc(k+32,fp);		
		}
		fclose(fp);
		free_clut_data();
}

static void read_clut(int off, char* outdir, unsigned char* buf)
{
	unsigned int i, j;
	unsigned int maxval;

	if (!buf)
	{
		fprintf(stderr,"Buffer is empty\n");
		return;
	}
	
	unsigned char nClutItems = buf[0x10];
	int nClutSectionSize = (nClutItems * 3) + ((MAX_LINUX_LOGO_COLORS * 3) - nClutItems) + 0x11;

	char magic[17] = {0};
	if(buf[nClutSectionSize + 6] != 'l')
		nClutSectionSize-=2;
	
	strncpy(magic, &buf[nClutSectionSize + 6], 16);
	
	short wWidth  = swap_uint16(*(short*)&buf[nClutSectionSize + 2]);
	short wHeight = swap_uint16(*(short*)&buf[nClutSectionSize + 4]);

	fprintf(stderr,"Processing an image @0x%X.Format: %ux%u@%d\n",off, wWidth, wHeight, nClutItems);
	if(strcmp(magic,"logo_RKlogo_data"))
	{
		fprintf(stderr,"Wrong magic, skipping...\n");
		return;
	}
	
	//Read clut table
	memset(logo_clut, 0, MAX_LINUX_LOGO_COLORS * sizeof(struct color));
	for (i = 0; i < nClutItems; ++i)
	{
		logo_clut[i].red = buf[0x11 + (i *3 )];
		logo_clut[i].green = buf[0x12 + (i * 3)];
		logo_clut[i].blue = buf[0x13 + (i * 3)];
	//	fprintf(stderr,"Writting #%d (%u,%u,%u)\n", i, logo_clut[i].red , logo_clut[i].green,logo_clut[i].blue);
	}
	char szFile[224];
	sprintf(szFile, "%s/%08X.ppm",outdir, off);
	FILE *o = fopen(szFile, "wb+");
	if(!o)
	{
		fprintf(stderr,"Cannot open %s for write!\n", szFile);
		return;
	}
	
	fprintf(o,"P3\n%d %d\n%d\n", wWidth, wHeight, 255); // Write header
	int x, y;
	for(y = 0;y < wHeight; ++y)
	{
		for(x = 1;x < wWidth+1; ++x)
		{
			unsigned char nClut = buf[nClutSectionSize + 26 + (x + (y * wWidth))];
			nClut-=0x20;
			if(nClut > nClutItems)
				fprintf(stderr,"Warning clut #%u @0x%X is out of table range!\n", nClut, 26 + (x + (y * wWidth)));
			fprintf(o,"%d %d %d",
			logo_clut[nClut].red,
			logo_clut[nClut].green,
			logo_clut[nClut].blue); 
			if(x % 6 && x != wWidth +1) fprintf(o, "  "); else fprintf(o,"\n");
		}
		fprintf(o, "\n");
	}
	fclose(o);
}

static void usage(char **argv)
{
	fprintf(stderr,"\t%s kernel.img  output_dir\n", argv[0]);
	fprintf(stderr,"\t%s 0xaddress.ppm  kernel.img\n", argv[0]);
	exit(EXIT_FAILURE);
}

const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

int main(int argc, char *argv[])
{
	int c, i_opt = 0;
	FILE * f , *o;
	char *outname = 0, *filename = 0;
	
	if (argc == 3 )
	{
		filename = argv[1];
		outname = argv[2];
		
		f = fopen(filename, "rb");
		if(!f)
		{
			printf("File %s doesn't exists!\n", argv[1]);
			exit(EXIT_FAILURE);
		}
		fclose(f);
	}
	else	
		usage(argv);
		
	fprintf(stderr,
		   "===========================================\n"
		   "Rockchip Logo Tool v. 0.2 by lolet\n"
		   "===========================================\n");
	fprintf(stderr,"Input file: %s\n", filename);
	fprintf(stderr,"Output file: %s\n", outname);
	
	if(!strcmp(get_filename_ext(filename), "ppm")) 
	{
		fprintf(stderr,"Reading image...\n");
		unsigned int addr;
		char* fn = basename(filename);
		sscanf(fn,"%08X.ppm", &addr);
		if(addr == 0 || fn[0] != '0')
		{
			fprintf(stderr,"Cannot read address from filename. Use 32bit hex format i.e. 1234ABCD.ppm!\n");
			exit(EXIT_FAILURE);
		}
		
		if(read_ppm(filename))
		{
			fprintf(stderr, "Writing image @0x%X...\n", addr);
			write_clut(addr, outname);
		}
	}
	else if(!strcmp(get_filename_ext(filename), "img"))
	{
		FILE * fclut = fopen(filename,"rb+");
		if(!fclut) 
		{
			fprintf(stderr,"File %s doesn't exists!\n",filename);
			exit(EXIT_FAILURE);
		}
		fprintf(stderr,"Reading image...");
		fseek (fclut , 0 , SEEK_END);
		int s = ftell (fclut);
		rewind (fclut);
		fprintf(stderr,"%d bytes\n", s);
		fflush(stdout);
		char * clutdata = malloc(s);
		int r = fread(clutdata, 1, s, fclut);
		fprintf(stderr, "Read %d bytes of data\n", r);
		int i;
		#if defined  _WIN32 || defined _WIN64
			mkdir(outname);
		#else
			mkdir(outname, 777);
		#endif
		for(i = 0; i< r-16; ++i)
		{
			if(clutdata[i] == 'l' && clutdata[i+1] == 'o' && clutdata[i+2] == 'g' && clutdata[i+3] == 'o' && clutdata[i+4] == '_' && 
			clutdata[i+5] == 'R' && clutdata[i+6] == 'K' && clutdata[i+7] == 'l' &&	clutdata[i+8] == 'o' && clutdata[i+9] == 'g' && 
			clutdata[i+10] == 'o' && clutdata[i+11] == '_' && clutdata[i+12] == 'c' && clutdata[i+13] == 'l' && clutdata[i+14] == 'u' &&
			clutdata[i+15] == 't')
			{
				read_clut(i, outname, &clutdata[i]);
				i+=15;
			}
		}		

		
		free(clutdata);
		fclose(fclut);
	}
	else
	{
		fprintf(stderr,"Unrecognised file format");
		usage(argv);
	}

	fprintf(stderr,"Finished!\n");
	return EXIT_SUCCESS;
}
	
