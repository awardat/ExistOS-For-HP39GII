#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"


#include "SysConf.h"

#if FS_TYPE == FS_FATFS
    #include "ff.h"
#else
    #endif


//#include "lvgl.h"

#include "debug.h"

#include "SystemFs.h"
#include "SystemUI.h"


#if FS_TYPE == FS_FATFS
    FATFS *fs;
#else

static char read_buf[2048];
static char write_buf[2048];

int EVM_Flash_Read(const struct lfs_config *c, lfs_block_t block,
            lfs_off_t off, void *buffer, lfs_size_t size)
{
    printf("lfs_read:b:%d,off:%d,size:%d\n",block, off, size);

    //ll_flash_page_read(block, 1, read_buf);
    //memcpy(buffer, &read_buf[off], size);

    ll_flash_page_read(block, 1, buffer);
    return 0;
}

int EVM_Flash_Prog(const struct lfs_config *c, lfs_block_t block,
            lfs_off_t off, const void *buffer, lfs_size_t size)
{
    printf("lfs_prog:b:%d,off:%d,size:%d\n",block, off, size);
    /*
    ll_flash_page_read(block, 1, write_buf);
    memcpy(&write_buf[off], buffer, size);
    ll_flash_page_write(block, 1, write_buf);*/

    ll_flash_page_write(block, 1, (char *)buffer);
    return 0;
}
    
int EVM_Flash_Erase(const struct lfs_config *c, lfs_block_t block)
{
    printf("lfs_erase:b:%d\n",block);
    ll_flash_page_trim(block);
    return 0;
}

int EVM_Flash_Sync(const struct lfs_config *c)
{
    ll_flash_sync();
    return 0;
}


struct lfs_config lfs_cfg = {
    // block device operations
    .read  = EVM_Flash_Read,
    .prog  = EVM_Flash_Prog,
    .erase = EVM_Flash_Erase,
    .sync  = EVM_Flash_Sync,

    // block device configuration
    .read_size = 2048,
    .prog_size = 2048,
    //.block_size = 2048,
    //.block_count = 128,
    .cache_size = 2048,
    .lookahead_size = 64,
    .block_cycles = -1,
};

#endif

 
void *GetFsObj()
{
    #if FS_TYPE == FS_FATFS
        return NULL; // FATFS 分支未挂载，返回空对象
    #else
        return (void *)&lfs;
    #endif
}

void SystemFSInit() {
#if FS_TYPE == FS_FATFS

    fs = pvPortMalloc(sizeof(FATFS));
    if (!fs) {
        printf("SystemFSInit: no mem for FATFS\n");
        return;
    }


/*
    fres = f_mount(fs, FS_FLASH_PATH, 1);

    printf("f_mount res:%d\n", fres);
    if (fres != FR_OK) {
        sprintf(textbuf, "Mount Fatfs Failed:-%d, would you like to format the flash?", fres);
        sel = SystemUIMsgBox(NULL, textbuf, "Mount " FS_FLASH_PATH " Failed", SYSTEMUI_MSGBOX_BUTTON_CANCAL | SYSTEMUI_MSGBOX_BUTTON_OK);
        if (sel == 0) {

            lv_obj_t *spinner = lv_spinner_create(lv_scr_act(), 1000, 60);
            lv_obj_set_size(spinner, 50, 50);
            lv_obj_center(spinner);

            BYTE *work = pvPortMalloc(FF_MAX_SS);
            fres = f_mkfs(FS_FLASH_PATH, 0, work, FF_MAX_SS);
            printf("mkfs:%d\n", fres);

            lv_obj_del(spinner);

            vPortFree(work);

            if (fres == FR_OK) {
                SystemUIMsgBox(NULL,"Format " FS_FLASH_PATH " Succeeded.", "Format", 0);
            } else {
                SystemUIMsgBox(NULL,"Format " FS_FLASH_PATH " Failed.", "Format", 0);
            }
        }
        goto mount_flash;
    }

    */


#endif


}


void SystemFSDeInit() 
{



}
