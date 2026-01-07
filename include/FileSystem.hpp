#pragma once

#include "pico/stdlib.h"
#include "sd_card.h"
#include "ff.h"

namespace fs{
    extern FRESULT fsresult;
    extern FATFS filesystem;
    extern int fileresult;
    extern FIL current_file;

    enum class SD_MODE : int { SD_WRITE, SD_WRITE_APPEND, SD_READ }; // SD File modes
    enum class SD_STATUS : int { SD_OK, SD_ERR }; // SD File status

    extern bool sd_available;
    extern bool sd_busy;

    bool init_sd();
    void unmount_sd();
    SD_STATUS open_file(SD_MODE mode, char* path);
    SD_STATUS close_file();
    SD_STATUS write_file(char* data);
}