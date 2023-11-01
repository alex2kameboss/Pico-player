#include <stdio.h>
#include "pico/stdlib.h"
#include <fatfs/ff.h>

#include <string>
#include <vector>

std::vector<std::string> list_files (const char *path) {
    FRESULT res;
    DIR dir;
    FILINFO fno;
    int nfile;

    std::vector<std::string> ret;

    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        nfile = 0;
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Error or end of dir */
            if (!(fno.fattrib & AM_DIR)) {
                nfile++;
            }
        }
        f_closedir(&dir);
    
        ret.reserve(nfile);
        
        res = f_opendir(&dir, path);                       /* Open the directory */
        nfile = 0;
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Error or end of dir */
            if (!(fno.fattrib & AM_DIR)) {
                ret.push_back(fno.fname);
            }
        }
        f_closedir(&dir);
    }

    return ret;
}

std::vector<std::string> list_dirs (const char *path) {
    FRESULT res;
    DIR dir;
    FILINFO fno;
    int nfile;

    std::vector<std::string> ret;

    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        nfile = 0;
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Error or end of dir */
            if (fno.fattrib & AM_DIR) {
                nfile++;
            }
        }
        f_closedir(&dir);
    
        ret.reserve(nfile);
        
        res = f_opendir(&dir, path);                       /* Open the directory */
        nfile = 0;
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Error or end of dir */
            if (fno.fattrib & AM_DIR) {
                ret.push_back(fno.fname);
            }
        }
        f_closedir(&dir);
    }

    return ret;
}

FRESULT list_dir (const char *path)
{
    FRESULT res;
    DIR dir;
    FILINFO fno;
    int nfile, ndir;


    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        nfile = ndir = 0;
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Error or end of dir */
            if (fno.fattrib & AM_DIR) {            /* Directory */
                printf("   <DIR>   %s\n", fno.fname);
                ndir++;
            } else {                               /* File */
                printf("%10u %s\n", fno.fsize, fno.fname);
                nfile++;
            }
        }
        f_closedir(&dir);
        printf("%d dirs, %d files.\n", ndir, nfile);
    } else {
        printf("Failed to open \"%s\". (%u)\n", path, res);
    }
    return res;
}

int main() {
    // start uart
    stdio_init_all();
    
    sleep_ms(5000);

    FATFS fs;
    FIL fil;
    FRESULT fr;
    UINT br;
    UINT bw;
    char line[100]; /* Line buffer */

    fr = f_mount(&fs, "", 1);

    if (fr != FR_OK) {
        printf("mount error %d\n", fr);
    }
    printf("mount ok\n");

    switch (fs.fs_type) {
        case FS_FAT12:
            printf("Type is FAT12\n");
            break;
        case FS_FAT16:
            printf("Type is FAT16\n");
            break;
        case FS_FAT32:
            printf("Type is FAT32\n");
            break;
        case FS_EXFAT:
            printf("Type is EXFAT\n");
            break;
        default:
            printf("Type is unknown\n");
            break;
    }
    printf("Card size: %7.2f GB (GB = 1E9 bytes)\n\n", fs.csize * fs.n_fatent * 512E-9);

    //list_dir("/");
    auto files = list_files("/");
    printf("Files:\n");
    for (const auto& f : files) {
        printf("\t%s\n", f.c_str());
    }

    auto dirs = list_dirs("/");
    printf("Dirs:\n");
    for (const auto& d : dirs) {
        printf("\t%s\n", d.c_str());
    }

    if (files.size() > 0) {
        printf("Content of the %s\n", files[0].c_str());
        /* Open a text file */
        fr = f_open(&fil, files[0].c_str(), FA_READ);
        if (fr) return (int)fr;

        /* Read every line and display it */
        while (f_gets(line, sizeof line, &fil)) {
            printf(line);
        }

        /* Close the file */
        f_close(&fil);
    }

    while(true) {
        printf("SD Test!\n");
        sleep_ms(1000);
    }

    return 0;
}