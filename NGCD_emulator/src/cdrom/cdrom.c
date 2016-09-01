/**************************************
****    CDROM.C  -  File reading   ****
**************************************/

//-- Include files -----------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <ctype.h>

#include <string.h>

#include "cdrom.h"
#include "../neocd.h"


#ifdef LOWERCASEFILES

#define CHANGECASE  tolower
#define IPL_TXT     "ipl.txt"
#define PRG     "prg"
#define FIX     "fix"
#define SPR      "spr"
#define Z80      "z80"
#define PAT      "pat"
#define PCM      "pcm"
#define JUE      "jue"
#define TITLE_X_SYS "title_x.sys"

#else

#define CHANGECASE  toupper
#define IPL_TXT  "IPL.TXT"
#define PRG     "PRG"
#define FIX     "FIX"
#define SPR      "SPR"
#define Z80      "Z80"
#define PAT      "PAT"
#define PCM      "PCM"
#define JUE      "JUE"
#define TITLE_X_SYS "TITLE_X.SYS"

#endif

/*-- Definitions -----------------------------------------------------------*/
#define    BUFFER_SIZE    131072
#define PRG_TYPE    0
#define FIX_TYPE    1
#define SPR_TYPE    2
#define Z80_TYPE    3
#define PAT_TYPE    4
#define PCM_TYPE    5
#define    min(a, b) ((a) < (b)) ? (a) : (b)

/*-- Exported Functions ----------------------------------------------------*/
int    cdrom_init1(void);
int    cdrom_load_prg_file(char *, unsigned int);
int    cdrom_load_z80_file(char *, unsigned int);
int    cdrom_load_fix_file(char *, unsigned int);
int    cdrom_load_spr_file(char *, unsigned int);
int    cdrom_load_pcm_file(char *, unsigned int);
int    cdrom_load_pat_file(char *, unsigned int, unsigned int);
void    cdrom_load_files(void);
int    cdrom_process_ipl(void);
void    cdrom_shutdown(void);
void    fix_conv(unsigned char *, unsigned char *, int, unsigned char *);
void    spr_conv(unsigned char *, unsigned char *, int, unsigned char *);
void    neogeo_upload(void);
void    cdrom_load_title(void);
void    cdrom_apply_patch(short *source, int offset, int bank);

//-- Private Variables -------------------------------------------------------
static char    cdrom_buffer[BUFFER_SIZE];
static char    cdpath[256];

//-- Private Function --------------------------------------------------------
static    int    recon_filetype(char *);

//-- Exported Variables ------------------------------------------------------
int        cdrom_current_drive;
SDL_Surface    *loading_pict;
int        img_display = 1;

//----------------------------------------------------------------------------
int    cdrom_init1(void)
{    
    FILE *fp;
    char Path[256];
    int found=0,number;

    printf("\nDetecting CD-ROM drives... ");

    /* Check for CD drives */
    if(!(number=SDL_CDNumDrives())){
        /* None found */
        fprintf(stderr, "No CDROM devices available\n");
        exit(-1);
    }

    cdrom_current_drive=0;

    printf("%d CD-ROM drives found\n",number);


    //find 1st neogeoCD in system
    while(!found && cdrom_current_drive<number)
    {
        printf("Trying %s\n",SDL_CDName(cdrom_current_drive));

        if(getMountPoint(cdrom_current_drive,cdpath))
        {
            strcpy(Path, cdpath);
            strcat(Path, IPL_TXT);

            if((fp=fopen(Path,"r"))!=NULL)
            {
                found=1;
                fclose(fp);
                printf("Found Neogeo CD in drive: %s\n",SDL_CDName(cdrom_current_drive));
            } else 
                cdrom_current_drive++;
        } else {
            printf ("CD drive %s is not mounted\n",SDL_CDName(cdrom_current_drive));
            
            cdrom_current_drive++;
        }
    }

    if(!found)
    {
        printf("No NeogeoCD Discs available\n");
        exit(-1);
    }

    loading_pict=SDL_LoadBMP("loading.bmp");
    return 1;
}

//----------------------------------------------------------------------------
void    cdrom_shutdown(void)
{
    /* free loading picture surface ??? */
}

//----------------------------------------------------------------------------
int    cdrom_load_prg_file(char *FileName, unsigned int Offset)
{
    FILE    *fp;
    char    Path[256];
    char    *Ptr;
    int        Readed;

    strcpy(Path, cdpath);
    strcat(Path, FileName);

    fp = fopen(Path, "rb");
    if (fp==NULL) {
        sprintf(global_error, "Could not open %s", Path);
        return 0;
    }
    
    Ptr = neogeo_prg_memory + Offset;
    
    do {
        Readed = fread(cdrom_buffer, 1, BUFFER_SIZE, fp);
        swab(cdrom_buffer, Ptr, Readed);
        Ptr += Readed;
    } while(  !feof(fp) );//Readed==BUFFER_SIZE &&
    
    fclose(fp);

    return 1;
}

//----------------------------------------------------------------------------
int    cdrom_load_z80_file(char *FileName, unsigned int Offset)
{
    FILE    *fp;
    char    Path[256];

    strcpy(Path, cdpath);
    strcat(Path, FileName);

    fp = fopen(Path, "rb");
    if (fp==NULL) {
        sprintf(global_error, "Could not open %s", Path);
        return 0;
    }

    fread( &subcpu_memspace[Offset], 1, 0x10000, fp);

    fclose(fp);

    return 1;
}

//----------------------------------------------------------------------------
int    cdrom_load_fix_file(char *FileName, unsigned int Offset)
{
    FILE    *fp;
    char    Path[256];
    char    *Ptr, *Src;
    int        Readed;

    strcpy(Path, cdpath);
    strcat(Path, FileName);
    
    fp = fopen(Path, "rb");
    if (fp==NULL) {
        sprintf(global_error, "Could not open %s", Path);
        return 0;
    }
    
    Ptr = neogeo_fix_memory + Offset;
    
    do {
        memset(cdrom_buffer, 0, BUFFER_SIZE);
        Readed = fread(cdrom_buffer, 1, BUFFER_SIZE, fp);
        Src = cdrom_buffer;
        fix_conv(Src, Ptr, Readed, video_fix_usage + (Offset>>5));
        Ptr += Readed;
        Offset += Readed;
    } while( Readed == BUFFER_SIZE );
    
    fclose(fp);
    
    return 1;
}

//----------------------------------------------------------------------------
int    cdrom_load_spr_file(char *FileName, unsigned int Offset)
{
    FILE    *fp;
    char    Path[256];
    char    *Ptr;
    int        Readed;

    strcpy(Path, cdpath);
    strcat(Path, FileName);
    
    fp = fopen(Path, "rb");
    if (fp==NULL) {
        sprintf(global_error, "Could not open %s", Path);
        return 0;
    }
    
    Ptr = neogeo_spr_memory + Offset;
    
    do {
        memset(cdrom_buffer, 0, BUFFER_SIZE);
        Readed = fread(cdrom_buffer, 1, BUFFER_SIZE, fp);
        spr_conv(cdrom_buffer, Ptr, Readed, video_spr_usage + (Offset>>7));
        Offset += Readed;
        Ptr += Readed;
    } while( Readed == BUFFER_SIZE );
    
    fclose(fp);
    
    return 1;
}

//----------------------------------------------------------------------------
int    cdrom_load_pcm_file(char *FileName, unsigned int Offset)
{
    FILE        *fp;
    char        Path[256];
    char        *Ptr;

    strcpy(Path, cdpath);
    strcat(Path, FileName);

    fp = fopen(Path, "rb");
    if (fp==NULL) {
        sprintf(global_error, "Could not open %s", Path);
        return 0;
    }

    Ptr = neogeo_pcm_memory + Offset;
    fread(Ptr, 1, 0x100000, fp);

    fclose(fp);

    return 1;
}

//----------------------------------------------------------------------------
int    cdrom_load_pat_file(char *FileName, unsigned int Offset, unsigned int Bank)
{
    FILE    *fp;
    char    Path[256];
    int        Readed;

    strcpy(Path, cdpath);
    strcat(Path, FileName);
    
    fp = fopen(Path, "rb");
    if (fp==NULL) {
        sprintf(global_error, "Could not open %s", Path);
        return 0;
    }
    
    Readed = fread(cdrom_buffer, 1, BUFFER_SIZE, fp);
    swab(cdrom_buffer, cdrom_buffer, Readed);
    cdrom_apply_patch((short*)cdrom_buffer, Offset, Bank);

    fclose(fp);

    return 1;
}




int hextodec(char c) {
	switch (tolower(c)) {
	case '0':	return 0;
	case '1':	return 1;
	case '2':	return 2;
	case '3':	return 3;
	case '4':	return 4;
	case '5':	return 5;
	case '6':	return 6;
	case '7':	return 7;
	case '8':	return 8;
	case '9':	return 9;
	case 'a':	return 10;
	case 'b':	return 11;
	case 'c':	return 12;
	case 'd':	return 13;
	case 'e':	return 14;
	case 'f':	return 15;
	default:	return 0;
	}
}



//----------------------------------------------------------------------------
int    cdrom_process_ipl(void)
{
    FILE    *fp;
    char    Path[256];
    char    Line[32];
    char    FileName[16];
    int        FileType;
    int        Bnk;
    int        Off;
    int        i, j;

    strcpy(Path, cdpath);
    strcat(Path, IPL_TXT);
    
    fp = fopen(Path, "rb");
    
    if (fp==NULL) {
        strcpy(global_error, "Could not open IPL.TXT!");
        return 0;
    }
    while (fgets(Line, 32, fp)!=NULL)
    {
        Bnk=0;
		Off=0;

		processEvents();
    
        i=0;
        j=0;
        while((Line[i] != ',')&&(Line[i]!=0))
            FileName[j++] = CHANGECASE(Line[i++]);
        FileName[j]=0;

        j -= 3;
        if (j>0) {
            FileType = recon_filetype(&FileName[j]);
            i++;
            j=0;
            while(Line[i] != ',') {
                Bnk*=10;
				Bnk+=Line[i]-'0';
				i++;
			}

            i++;
            j=0;

            while(Line[i] != 0x0D) {
			    Off*=16;
				Off+=hextodec(Line[i++]);
			}

            Bnk &= 3;
            
            printf("Loading File: %s %02x %08x\n", FileName, Bnk, Off);

            switch( FileType ) {
            case PRG_TYPE:
                if (!cdrom_load_prg_file(FileName, Off)) {
                    fclose(fp);
                    return 0;
                }
                break;
            case FIX_TYPE:
                if (!cdrom_load_fix_file(FileName, (Off>>1))) {
                    fclose(fp);
                    return 0;
                }
                break;
            case SPR_TYPE:
                if (!cdrom_load_spr_file(FileName, (Bnk*0x100000) + Off)) {
                    fclose(fp);
                    return 0;
                }
                break;
            case Z80_TYPE:
                if (!cdrom_load_z80_file(FileName, (Off>>1))) {
                    fclose(fp);
                    return 0;
                }
                break;
            case PAT_TYPE:
                if (!cdrom_load_pat_file(FileName, Off, Bnk)) {
                    fclose(fp);
                    return 0;
                }
                break;
            case PCM_TYPE:
                if (!cdrom_load_pcm_file(FileName, (Bnk*0x80000) + (Off>>1))) {
                    fclose(fp);
                    return 0;
                }
                break;
            }
        }
    }
    
    fclose(fp);
    
    return 1;
}

//----------------------------------------------------------------------------
int    recon_filetype(char *ext)
{    
    if (strcmp(ext, PRG)==0)
        return PRG_TYPE;
    
    if (strcmp(ext, FIX)==0)
        return FIX_TYPE;
    
    if (strcmp(ext, SPR)==0)
        return SPR_TYPE;
        
    if (strcmp(ext, Z80)==0)
        return Z80_TYPE;
        
    if (strcmp(ext, PAT)==0)
        return PAT_TYPE;
    
    if (strcmp(ext, PCM)==0)
        return PCM_TYPE;
        
    return    -1;
}


//----------------------------------------------------------------------------
unsigned int motorola_peek(unsigned char *address) 
{
    unsigned int a,b,c,d;
	
	a=address[0]<<24;
	b=address[1]<<16;
	c=address[2]<<8;
	d=address[3]<<0;
	
	return (a|b|c|d);
}



//----------------------------------------------------------------------------
void    cdrom_load_files(void)
{
    char    Entry[32], FileName[13];
    char    *Ptr, *Ext;
    int        i, j, Bnk, Off, Type, Reliquat;
    
    //sound_mute();

    if (m68k_read_memory_8(m68k_get_reg(NULL,M68K_REG_A0))==0)
        return;


    cdda_stop();
    cdda_current_track = 0;

    m68k_write_memory_32(0x10F68C, 0x00000000);
    m68k_write_memory_8(0x10F6C3, 0x00);
    m68k_write_memory_8(0x10F6D9, 0x01);
    m68k_write_memory_8(0x10F6DB, 0x01);
    m68k_write_memory_32(0x10F742, 0x00000000);
    m68k_write_memory_32(0x10F746, 0x00000000);
    m68k_write_memory_8(0x10FDC2, 0x01);
    m68k_write_memory_8(0x10FDDC, 0x00);
    m68k_write_memory_8(0x10FDDD, 0x00);
    m68k_write_memory_8(0x10FE85, 0x01);
    m68k_write_memory_8(0x10FE88, 0x00);
    m68k_write_memory_8(0x10FEC4, 0x01);


    if (img_display == 1) {
        SDL_BlitSurface(loading_pict,NULL,video_buffer,NULL);
        (*blitter)();
    }

    Ptr = neogeo_prg_memory + m68k_get_reg(NULL,M68K_REG_A0);

    do {
        Reliquat = ((int)Ptr)&1;

        if (Reliquat)
            Ptr--;

        swab(Ptr, Entry, 32);
        i=Reliquat;

        while((Entry[i]!=0)&&(Entry[i]!=';')) {
            FileName[i-Reliquat] = CHANGECASE(Entry[i]);
            i++;
        }

        FileName[i-Reliquat] = 0;

        if (Entry[i]==';')    /* 01/05/99 MSLUG2 FIX */
            i += 2;

        i++;

        Bnk = Entry[i++]&3;

        if (i&1)
            i++;


        Off = motorola_peek(&Entry[i]);
        i += 4;
        Ptr += i;

        printf("Loading File: %s %02x %08x\n", FileName, Bnk, Off);

        j=0;

        while(FileName[j] != '.' && FileName[j] != '\0')
            j++;

        if(FileName[j]=='\0')
        {
            fprintf(stderr,"Internal Error loading file: %s",FileName);
            exit(1);
        }

        j++;
        Ext=&FileName[j];

        Type = recon_filetype(Ext);

        switch( Type ) {
        case PRG_TYPE:
            cdrom_load_prg_file(FileName, Off);
            break;
        case FIX_TYPE:
            cdrom_load_fix_file(FileName, Off>>1);
            break;
        case SPR_TYPE:
            cdrom_load_spr_file(FileName, (Bnk*0x100000) + Off);
            break;
        case Z80_TYPE:
            cdrom_load_z80_file(FileName, Off>>1);
            break;
        case PAT_TYPE:
            cdrom_load_pat_file(FileName, Off, Bnk);
            break;
        case PCM_TYPE:
            cdrom_load_pcm_file(FileName, (Bnk*0x80000) + (Off>>1));
            break;
        }
        
        processEvents();

    } while( Entry[i] != 0);
	
	/* update neocd time */
	neocd_time=SDL_GetTicks()+REFRESHTIME;

}

//----------------------------------------------------------------------------
void    fix_conv(unsigned char *Src, unsigned char *Ptr, int Taille,
    unsigned char *usage_ptr)
{
    int        i;
    unsigned char    usage;
    
    for(i=Taille;i>0;i-=32) {
        usage = 0;
        *Ptr++ = *(Src+16);
        usage |= *(Src+16);
        *Ptr++ = *(Src+24);
        usage |= *(Src+24);
        *Ptr++ = *(Src);
        usage |= *(Src);
        *Ptr++ = *(Src+8);
        usage |= *(Src+8);
        Src++;
        *Ptr++ = *(Src+16);
        usage |= *(Src+16);
        *Ptr++ = *(Src+24);
        usage |= *(Src+24);
        *Ptr++ = *(Src);
        usage |= *(Src);
        *Ptr++ = *(Src+8);
        usage |= *(Src+8);
        Src++;
        *Ptr++ = *(Src+16);
        usage |= *(Src+16);
        *Ptr++ = *(Src+24);
        usage |= *(Src+24);
        *Ptr++ = *(Src);
        usage |= *(Src);
        *Ptr++ = *(Src+8);
        usage |= *(Src+8);
        Src++;
        *Ptr++ = *(Src+16);
        usage |= *(Src+16);
        *Ptr++ = *(Src+24);
        usage |= *(Src+24);
        *Ptr++ = *(Src);
        usage |= *(Src);
        *Ptr++ = *(Src+8);
        usage |= *(Src+8);
        Src++;
        *Ptr++ = *(Src+16);
        usage |= *(Src+16);
        *Ptr++ = *(Src+24);
        usage |= *(Src+24);
        *Ptr++ = *(Src);
        usage |= *(Src);
        *Ptr++ = *(Src+8);
        usage |= *(Src+8);
        Src++;
        *Ptr++ = *(Src+16);
        usage |= *(Src+16);
        *Ptr++ = *(Src+24);
        usage |= *(Src+24);
        *Ptr++ = *(Src);
        usage |= *(Src);
        *Ptr++ = *(Src+8);
        usage |= *(Src+8);
        Src++;
        *Ptr++ = *(Src+16);
        usage |= *(Src+16);
        *Ptr++ = *(Src+24);
        usage |= *(Src+24);
        *Ptr++ = *(Src);
        usage |= *(Src);
        *Ptr++ = *(Src+8);
        usage |= *(Src+8);
        Src++;
        *Ptr++ = *(Src+16);
        usage |= *(Src+16);
        *Ptr++ = *(Src+24);
        usage |= *(Src+24);
        *Ptr++ = *(Src);
        usage |= *(Src);
        *Ptr++ = *(Src+8);
        usage |= *(Src+8);
        Src+=25;
        *usage_ptr++ = usage;
    }
}


//----------------------------------------------------------------------------
#define COPY_BIT(a, b) { \
    a <<= 1; \
    a |= (b & 0x01); \
    b >>= 1; }

void extract8(char *src, char *dst) 
{ 
   int i; 

   unsigned char bh = *src++;
   unsigned char bl = *src++;
   unsigned char ch = *src++;
   unsigned char cl = *src; 
   unsigned char al, ah; 

   for(i = 0; i < 4; i++)
   { 
      al = ah = 0; 

      COPY_BIT(al, ch) 
      COPY_BIT(al, cl) 
      COPY_BIT(al, bh) 
      COPY_BIT(al, bl) 

      COPY_BIT(ah, ch) 
      COPY_BIT(ah, cl) 
      COPY_BIT(ah, bh) 
      COPY_BIT(ah, bl) 

      *dst++ = ((ah << 4) | al);
   } 
} 


//----------------------------------------------------------------------------
void spr_conv(unsigned char *src, unsigned char *dst, int len, unsigned char *usage_ptr)
{
    register int    i;
    int offset;

    for(i=0;i<len;i+=4) {
        if((i&0x7f)<64)
            offset=(i&0xfff80)+((i&0x7f)<<1)+4;
        else
            offset=(i&0xfff80)+((i&0x7f)<<1)-128;

        extract8(src,dst+offset);
        src+=4;
    }
}

//----------------------------------------------------------------------------
void    neogeo_upload(void)
{
    int        Zone;
    int        Taille;
    int        Banque;
    int        Offset = 0;
    unsigned char    *Source;
    unsigned char    *Dest;
    // FILE            *fp;

    Zone = m68k_read_memory_8(0x10FEDA);

    /*fp = fopen("UPLOAD.LOG", "at");

    fprintf(fp, "%02x: %06x, %06x:%02x, %08x\n",
        Zone,
        m68k_read_memory_32(0x10FEF8),
        m68k_read_memory_32(0x10FEF4),
        m68k_read_memory_8(0x10FEDB),
        m68k_read_memory_32(0x10FEFC));

    fclose(fp);   */

    switch( Zone&0x0F )
    {
    case    0:    // PRG

        Source = neogeo_prg_memory + m68k_read_memory_32(0x10FEF8);
        Dest = neogeo_prg_memory + m68k_read_memory_32(0x10FEF4);
        Taille = m68k_read_memory_32(0x10FEFC);

        memcpy(Dest, Source, Taille);

        m68k_write_memory_32( 0x10FEF4, m68k_read_memory_32(0x10FEF4) + Taille );

        break;

    case    2:    // SPR
        Banque = m68k_read_memory_8(0x10FEDB);
        Source = neogeo_prg_memory + m68k_read_memory_32(0x10FEF8);
        Offset = m68k_read_memory_32(0x10FEF4) + (Banque<<20);
        Dest = neogeo_spr_memory + Offset;
        Taille = m68k_read_memory_32(0x10FEFC);
        
        do {
            memset(cdrom_buffer, 0, BUFFER_SIZE);
            swab(Source, cdrom_buffer, min(BUFFER_SIZE, Taille));
            spr_conv(cdrom_buffer, Dest, min(BUFFER_SIZE, Taille), 
                video_spr_usage + (Offset>>7));
            Source += min(BUFFER_SIZE, Taille);
            Dest += min(BUFFER_SIZE, Taille);
            Offset += min(BUFFER_SIZE, Taille);
            Taille -= min(BUFFER_SIZE, Taille);
        } while(Taille!=0);
        
        
        // Mise � jour des valeurs
        Offset = m68k_read_memory_32( 0x10FEF4 );
        Banque = m68k_read_memory_8( 0x10FEDB );
        Taille = m68k_read_memory_8( 0x10FEFC );
        
        Offset += Taille;
        
        while (Offset > 0x100000 )
        {
            Banque++;
            Offset -= 0x100000;
        }
        
        m68k_write_memory_32( 0x10FEF4, Offset );
        m68k_write_memory_16( 0x10FEDB, Banque );
        
        break;

    case    1:    // FIX
        Source = neogeo_prg_memory + m68k_read_memory_32(0x10FEF8);
        Offset = m68k_read_memory_32(0x10FEF4)>>1;
        Dest = neogeo_fix_memory + Offset;
        Taille = m68k_read_memory_32(0x10FEFC);

        do {
            memset(cdrom_buffer, 0, BUFFER_SIZE);
            swab(Source, cdrom_buffer, min(BUFFER_SIZE, Taille));
            fix_conv(cdrom_buffer, Dest, min(BUFFER_SIZE, Taille), 
                video_fix_usage + (Offset>>5));
            Source += min(BUFFER_SIZE, Taille);
            Dest += min(BUFFER_SIZE, Taille);
            Offset += min(BUFFER_SIZE, Taille);
            Taille -= min(BUFFER_SIZE, Taille);
        } while(Taille!=0);
        
        Offset = m68k_read_memory_32( 0x10FEF4 );
        Taille = m68k_read_memory_32( 0x10FEFC );
        
        Offset += (Taille<<1);
        
        m68k_write_memory_32( 0x10FEF4, Offset);
        
        break;

    case    3:    // Z80

        Source = neogeo_prg_memory + m68k_read_memory_32(0x10FEF8);
        Dest = subcpu_memspace + (m68k_read_memory_32(0x10FEF4)>>1);
        Taille = m68k_read_memory_32(0x10FEFC);
        
        swab( Source, Dest, Taille);        

        m68k_write_memory_32( 0x10FEF4, m68k_read_memory_32(0x10FEF4) + (Taille<<1) );

        break;        

    case    5:    // Z80 patch

        Source = neogeo_prg_memory + m68k_read_memory_32(0x10FEF8);
        cdrom_apply_patch((short*)Source, m68k_read_memory_32(0x10FEF4), m68k_read_memory_8(0x10FEDB));

        break;
    
    case    4:    // PCM
        Banque = m68k_read_memory_8(0x10FEDB);
        Source = neogeo_prg_memory + m68k_read_memory_32(0x10FEF8);
        Offset = (m68k_read_memory_32(0x10FEF4)>>1) + (Banque<<19);
        Dest = neogeo_pcm_memory + Offset;
        Taille = m68k_read_memory_32(0x10FEFC);
        
        swab( Source, Dest, Taille);        
        
        // Mise � jour des valeurs
        Offset = m68k_read_memory_32( 0x10FEF4 );
        Banque = m68k_read_memory_8( 0x10FEDB );
        Taille = m68k_read_memory_8( 0x10FEFC );
        
        Offset += (Taille<<1);
        
        while (Offset > 0x100000 )
        {
            Banque++;
            Offset -= 0x100000;
        }
        
        m68k_write_memory_32( 0x10FEF4, Offset );
        m68k_write_memory_16( 0x10FEDB, Banque );

        break;    
        
    }
}

//----------------------------------------------------------------------------
void cdrom_load_title(void)
{
    char            Path[256];
    char            jue[4] = JUE;
    char            file[12] = TITLE_X_SYS;
    FILE            *fp;
    char            *Ptr;
    int                Readed;
    int                x, y;

    strcpy(Path, cdpath);

    file[6] = jue[m68k_read_memory_8(0x10FD83)&3];

    strcat(Path, file);
    
    fp = fopen(Path, "rb");
    if (fp==NULL)
    {
        return;
    }
    
    fread(video_paletteram_pc, 1, 0x5A0, fp);
    swab((char *)video_paletteram_pc, (char *)video_paletteram_pc, 0x5A0);

    for(Readed=0;Readed<720;Readed++)
        video_paletteram_pc[Readed] = video_color_lut[video_paletteram_pc[Readed]];

    Ptr = neogeo_spr_memory;
    
    Readed = fread(cdrom_buffer, 1, BUFFER_SIZE, fp);
    spr_conv(cdrom_buffer, Ptr, Readed, video_spr_usage);
    fclose(fp);

    Readed = 0;
    for(y=0;y<80;y+=16)
    {
        for(x=0;x<144;x+=16)
        {
            video_draw_spr(Readed, Readed, 0, 0, x+16, y+16, 15, 16);
            Readed++;
        }
    }

	(*blitter)();

    memset(neogeo_spr_memory, 0, 4194304);
    memset(neogeo_fix_memory, 0, 131072);
    memset(video_spr_usage, 0, 32768);
    memset(video_fix_usage, 0, 4096);
}

#define PATCH_Z80(a, b) { \
                            subcpu_memspace[(a)] = (b)&0xFF; \
                            subcpu_memspace[(a+1)] = ((b)>>8)&0xFF; \
                        }

void cdrom_apply_patch(short *source, int offset, int bank)
{
    int master_offset;
    
    master_offset = (((bank*1048576) + offset)/256)&0xFFFF;
    
    while(*source != 0)
    {
        PATCH_Z80( source[0], ((source[1] + master_offset)>>1) );
        PATCH_Z80( source[0] + 2, (((source[2] + master_offset)>>1) - 1) );
        
        if ((source[3])&&(source[4]))
        {
            PATCH_Z80( source[0] + 5, ((source[3] + master_offset)>>1) );
            PATCH_Z80( source[0] + 7, (((source[4] + master_offset)>>1) - 1) );
        }
            
        source += 5;
    }
}
