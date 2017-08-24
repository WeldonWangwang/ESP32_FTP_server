#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "dirent.h"
#include "ff.h"
#include "sd.h"

#define TAG  "ftp_server"



DIR * opennextdir(char *SDdirname)
{
    char SDNAME[10] = "/sdcard/";
    char *totaldir = SDNAME;
    while (*totaldir != '\0')
        totaldir++;
    while (*SDdirname != '\0')
    {
        *totaldir = *SDdirname;
        totaldir++;
        SDdirname++;
    }
    *totaldir = '\0';
    totaldir = SDNAME;
    DIR * r = opendir(totaldir);
    return r;
}

int vfs_stat(const char* filename, vfs_stat_t* st)
{
    FILINFO f;
#if _USE_LFN
    f.lfname = NULL;
#endif
    if (FR_OK != f_stat(filename, &f)) {
        return 1;
    }
    st->st_size = f.fsize;
    st->st_mode = f.fattrib;
    st->st_mtime.date = f.fdate;
    st->st_mtime.time = f.ftime;
    return 0;
}

FILE* openfile_read(char *SDfilename)
{
    char SDNAME[20] = "/sdcard/";
    char *totalfile = SDNAME;
    while (*totalfile != '\0')
        totalfile++;
    while (*SDfilename != '\0')
    {
        *totalfile = *SDfilename;
        totalfile++;
        SDfilename++;
    }
    *totalfile = '\0';
    totalfile = SDNAME;

    printf("file = %s\n", totalfile);
    FILE* f = fopen(totalfile, "r");
    return f;
}

FILE* openfile_write(char *q)
{

    char p[20] = "/sdcard/";
    char *p1 = p;
    while (*p1 != '\0')
        p1++;
    while (*q != '\0')
    {
        *p1 = *q;
        p1++;
        q++;
    }

    *p1 = '\0';
    p1 = p;

    printf("file = %s\n", p1);
    FILE* f = fopen(p1, "w");
    return f;
}

vfs_file_t* vfs_open(const char* filename) 
{
    vfs_file_t *f = malloc(sizeof(vfs_file_t));
    BYTE flags = 0;
    // while (*mode != '\0') {
    //     if (*mode == 'r') flags |= FA_READ;
    //     if (*mode == 'w') flags |= FA_WRITE | FA_CREATE_ALWAYS;
    //     mode++;
    // }
    FRESULT r = f_open(f, filename, FA_READ);
    if (FR_OK != r) {
        free(f);
        return NULL;
    }
    return f;
}

int vfs_read (char* buffer, int len, FIL* file) 
{
    unsigned int bytesread;
    FRESULT r = f_read(file, buffer, len, &bytesread);
    if (r != FR_OK) return 0;
    return bytesread;
}

void vfs_close(vfs_t* vfs) 
{

        /* Close a file */
        f_close(vfs);
        free(vfs);
    
}

void initilalise_SD(void)
{
    ESP_LOGI(TAG, "Initializing SD card");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5
    };

    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%d). Make sure SD card lines have pull-up resistors in place.", ret);
        }
        return;
    }
    sdmmc_card_print_info(stdout, card);
}