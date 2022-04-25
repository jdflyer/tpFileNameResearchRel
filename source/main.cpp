/**
 * @file main.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date YYYY-MM-DD
 *
 * @copyright Copyright (c) YYYY
 *
 */
#include <display/console.h>     // Contains a very neat helper class to print to the console
#include <main.h>
#include <patch.h>     // Contains code for hooking into a function
#include <tp/f_ap_game.h>
#include <tp/m_do_controller_pad.h>
#include "dName_c.h"
#include "dFile_select_c.h"


extern "C" void ct_dName_c(dName_c* this_);
extern "C" void dt_dName_c(dName_c* this_,uint16_t dtorSpecifier);
extern "C" void dFileSelect_c_screenSet(dFile_select_c* this_);

void (*ct_dName_c_trampoline)(dName_c* this_);
void (*dt_dName_c_trampoline)(dName_c* this_,uint16_t dtorSpecifier);
void (*dFileSelect_c_screenSet_trampoline)(dFile_select_c* this_);

dName_c* nameClass = NULL;
dFile_select_c* fileSelect = NULL;

namespace mod
{
    Mod* gMod = nullptr;

    void main()
    {
        Mod* mod = new Mod();
        mod->init();
    }

    // Create our console instance (it will automatically display some of the definitions from our Makefile like version,
    // variant, project name etc.
    // this console can be used in a similar way to cout to make printing a little easier; it also supports \n for new lines
    // (\r is implicit; UNIX-Like) and \r for just resetting the column and has overloaded constructors for all of the
    // primary cinttypes
    Mod::Mod(): c( 0 ) { i = 0; }

    void ct_dName_c_hook(dName_c* this_) {
        nameClass = this_;
        ct_dName_c_trampoline(this_);
    }

    void dt_dName_c_hook(dName_c* this_, uint16_t spec) {
        //nameClass = NULL;
        dt_dName_c_trampoline(this_,spec);
    }

    void dFileSelect_c_screenSet_hook(dFile_select_c* this_) {
        dFileSelect_c_screenSet_trampoline(this_);
        fileSelect = this_;
        return;
    }

    void enableCrashScreen() {
        uint32_t* enableCrashScreen = reinterpret_cast<uint32_t*>( 0x8000B8A4 );

        // Get the addresses to overwrite
        #ifdef TP_US
                enableCrashScreen = reinterpret_cast<uint32_t*>( 0x8000B8A4 );
        #elif defined TP_EU
                enableCrashScreen = reinterpret_cast<uint32_t*>( 0x8000B878 );
        #elif defined TP_JP
                enableCrashScreen = reinterpret_cast<uint32_t*>( 0x8000B8A4 );
        #endif

        // Perform the overwrites
        /* If the address is loaded into the cache before the overwrite is made,
        then the cache will need to be cleared after the overwrite */
        // Enable the crash screen
        *enableCrashScreen = 0x48000014;     // b 0x14

    }

    void Mod::init()
    {
        /**
         * Old way of printing to the console
         * Kept for reference as its still being used by the new console class.
         *
         * libtp::display::setConsole(true, 25);
         * libtp::display::print(1, "Hello World!");
         */
        enableCrashScreen();

        libtp::display::setConsole(true,25);
        //c << "Hello world!\n\n";

        gMod = this;
        // Hook the function that runs each frame
        return_fapGm_Execute = libtp::patch::hookFunction( libtp::tp::f_ap_game::fapGm_Execute, []() { return gMod->procNewFrame(); } );
        ct_dName_c_trampoline = libtp::patch::hookFunction(ct_dName_c,ct_dName_c_hook);
        dt_dName_c_trampoline = libtp::patch::hookFunction(dt_dName_c,dt_dName_c_hook);
        dFileSelect_c_screenSet_trampoline = libtp::patch::hookFunction(dFileSelect_c_screenSet,dFileSelect_c_screenSet_hook);
    }

    

    void Mod::procNewFrame()
    {
        static int32_t page = 0;
        static bool menuOn = true;
        static bool LlastPressed = false;
        static bool RlastPressed = false;
        static bool ZlastPressed = false;
        if (menuOn) {
            libtp::display::clearConsole(0,25);
            c.setLine(0);
            if (libtp::tp::m_do_controller_pad::cpadInfo.buttonInput==libtp::tp::m_do_controller_pad::PadInputs::Button_R) {
                if (RlastPressed==false){
                    page++;
                }

                RlastPressed = true;
            }else{
                RlastPressed = false;
            }
            if (libtp::tp::m_do_controller_pad::cpadInfo.buttonInput==libtp::tp::m_do_controller_pad::PadInputs::Button_L) {
                if (LlastPressed==false) {
                    page--;
                }

                LlastPressed = true;
            }else{
                LlastPressed = false;
            }
        }
        if (libtp::tp::m_do_controller_pad::cpadInfo.buttonInput==libtp::tp::m_do_controller_pad::PadInputs::Button_Z) {
            if (ZlastPressed==false) {
                if (menuOn) {
                    libtp::display::setConsole(false,25);
                    menuOn = false;
                }else{
                    libtp::display::setConsole(true,25);
                    menuOn = true;
                }
            }

            ZlastPressed = true;
        }else{
            ZlastPressed = false;
        }

        if (nameClass != NULL) {
            if (page == 0) {
                c << "\nCursor Pos: " << nameClass->mCursorPosition << '\n';
                c << "Last Pos: " << nameClass->mLastCursorPosition << '\n';
                c << "L/R For Full, Z to hide\n";
                for (int i = 0; i < 256; i++) {
                    if (nameClass->mCursorPosition==i) {
                        c << '>';
                    }else{
                        c << ' ';
                    }
                    c << *(uint8_t*)((&nameClass->field_0x2d3)+(i*8));
                }
            }else if(page == -1) {
                if (fileSelect != NULL) {
                    c << "\ndName_c addr = " << fileSelect->name;
                    c << "\nNext addr = " << fileSelect->name+sizeof(dName_c);
                    c << "\nFile Warning addr = " << fileSelect->file_warning;
                    c << "\ntt_block8x8.bti addr = " << fileSelect->field_0x2378;
                }
            }else{
                c << "\nPage " << page << ":\n";
                c << "Cursor Pos: " << nameClass->mCursorPosition << '\n';
                c << "Cursor Address: " << (uint8_t*)((&nameClass->field_0x2cc)+(nameClass->mCursorPosition*8)) << '\n';
                for (int i = 0; i < 256; i++) {
                    if (nameClass->mCursorPosition*8==(i)+((page*256)-256)) {
                        c << '>';
                    }
                    else{
                        c << ' ';
                    }
                    c << *(uint8_t*)((&nameClass->field_0x2cc)+(i)+((page*256)-256));
                }
            }
            
        }

        return return_fapGm_Execute();
    }
}     // namespace mod