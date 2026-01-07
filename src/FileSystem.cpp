#include "FileSystem.hpp"

namespace fs{
    // Filesystem (for SD)
    FRESULT fsresult;
    FATFS filesystem;
    int fileresult;
    FIL current_file;

    bool sd_available = false;
    bool sd_busy = false;


    bool init_sd(){
        sd_available = true; 

        // Check for & init filesystem
        if(!sd_init_driver()){
            sd_available = false;
            return false;
        }

        // Mount filesystem
        fsresult = f_mount(&filesystem, "0:", 1);
        if(fsresult != FR_OK){
            sd_available = false;
            return false;
        }
        return true;
    }
    void unmount_sd(){
        f_unmount("0:");
    }

    // Open file in selected mode
    SD_STATUS open_file(SD_MODE mode, char* path){
        if(!sd_busy){   // Only open file when SD is not busy
            // Attempt to open file in selected mode
            switch(mode){
                case SD_MODE::SD_WRITE: fsresult = f_open(&current_file, path, FA_WRITE | FA_OPEN_ALWAYS); break;
                case SD_MODE::SD_WRITE_APPEND: fsresult = f_open(&current_file, path, FA_WRITE | FA_OPEN_APPEND | FA_OPEN_ALWAYS); break;
                case SD_MODE::SD_READ: fsresult = f_open(&current_file, path, FA_READ); break;
                default: return SD_STATUS::SD_ERR;
            }
            // Return SD_OK if succeeded
            if(fsresult == FR_OK){
                sd_busy = true;
                return SD_STATUS::SD_OK;
            }
        }

        return SD_STATUS::SD_ERR;  // If file already open (busy) or a different error occured, return SD_ERR
    }

    // Close currently open file
    SD_STATUS close_file(){
        if(sd_busy){
            fsresult = f_close(&current_file); // Attempt to close current file

            // Return SD_OK if succeeded
            if(fsresult == FR_OK){
                sd_busy = false;
                return SD_STATUS::SD_OK;
            }
        }

        return SD_STATUS::SD_ERR; // If no file open (!busy) or a different error occured, return SD_ERR
        
    }

    // Write to currently open file
    SD_STATUS write_file(char* data){
        if(sd_busy){
            fileresult = f_printf(&current_file, data);
            
            // Return SD_OK if succeeded
            if(fileresult >= 0){
                return SD_STATUS::SD_OK;
            }
        }

        return SD_STATUS::SD_ERR; // If no file open (!busy) or a different error occured, return SD_ERR
    }
}