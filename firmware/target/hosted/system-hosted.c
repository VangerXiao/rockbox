/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2017 Marcin Bukat
 * Copyright (C) 2016 Amaury Pouly
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ucontext.h>
#include <sys/reboot.h>
#include <backtrace.h>

#include "system.h"
#include "mv.h"
#include "font.h"
#include "power.h"
#include "button.h"
#include "button-devinput.h"
#include "backlight-target.h"
#include "lcd.h"
#include "filesystem-hosted.h"
#include "logf.h"

/* to make thread-internal.h happy */
uintptr_t *stackbegin;
uintptr_t *stackend;

/* forward-declare */
bool os_file_exists(const char *ospath);

static void sig_handler(int sig, siginfo_t *siginfo, void *context)
{
    /* safe guard variable - we call backtrace() only on first
     * UIE call. This prevent endless loop if backtrace() touches
     * memory regions which cause abort
     */
    static bool triggered = false;

    lcd_set_backdrop(NULL);
    lcd_set_drawinfo(DRMODE_SOLID, LCD_BLACK, LCD_WHITE);
    unsigned line = 0;

    lcd_setfont(FONT_SYSFIXED);
    lcd_set_viewport(NULL);
    lcd_clear_display();

    /* get context info */
    ucontext_t *uc = (ucontext_t *)context;
    unsigned long pc = uc->uc_mcontext.pc;
    unsigned long sp = uc->uc_mcontext.gregs[29];

    lcd_putsf(0, line++, "%s at %08lx", strsignal(sig), pc);

    if(sig == SIGILL || sig == SIGFPE || sig == SIGSEGV || sig == SIGBUS || sig == SIGTRAP) {
        lcd_putsf(0, line++, "address %p", siginfo->si_addr);
    }

    if(!triggered)
    {
        triggered = true;
        rb_backtrace(pc, sp, &line);
    }

#ifdef ROCKBOX_HAS_LOGF
    lcd_putsf(0, line++, "logf:");
    logf_panic_dump(&line);
#endif

    lcd_update();

    system_exception_wait(); /* If this returns, try to reboot */
    system_reboot();
    while (1) {
        // Make sure we're not throttling the cpu
        usleep(1000);
    }
}

void power_off(void)
{
    backlight_hw_off();
    button_close_device();
    sync();
    /* The hosted port normally leaves the shutdown to init. On the HiBy R1
     * multi-boot image Rockbox is started from an init script, so init is still
     * busy in rcS and never acts on the signal /sbin/poweroff sends: the screen
     * goes dark and the device stays powered until the key is held for ten
     * seconds. Ask the kernel directly instead -- the same call the stock
     * firmware's own shutdown ends with -- and keep the old way as a fallback. */
    if (reboot(RB_POWER_OFF) == 0) {
        while (1) {
            usleep(1000);
        }
    }
    system("/sbin/poweroff");
    while (1) {
        // Make sure we're not throttling the cpu
        usleep(1000);
    }
}

void system_init(void)
{
    int *s;
    /* fake stack, to make thread-internal.h happy */
    stackbegin = stackend = (uintptr_t*)&s;
   /* catch some signals for easier debugging */
    struct sigaction sa;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = &sig_handler;
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGPIPE, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

void system_reboot(void)
{
    backlight_hw_off();
    system("/sbin/reboot");
    while (1) {
        // Make sure we're not throttling the cpu
        usleep(1000);
    }
}

void system_exception_wait(void)
{
    backlight_hw_on();
    backlight_hw_brightness(DEFAULT_BRIGHTNESS_SETTING);
    /* wait until button press and release */
#ifdef HAVE_BUTTON_DATA
    int bdata;
#define BDATA &bdata
#else
#define BDATA
#endif
    while(button_read_device(BDATA) != 0) {}
    while(button_read_device(BDATA) == 0) {}
    while(button_read_device(BDATA) != 0) {}
    while(button_read_device(BDATA) == 0) {}
}

bool hostfs_removable(IF_MD_NONVOID(int drive))
{
#ifdef HAVE_MULTIDRIVE
    if (drive > 0)
        return true;
    else
#endif
#ifdef HAVE_HOTSWAP_STORAGE_AS_MAIN
        return true;
#else
        return false; /* internal: always present */
#endif
}

bool hostfs_present(IF_MD_NONVOID(int drive))
{
#ifdef HAVE_MULTIDRIVE
    if (drive > 0)
#if defined(MULTIDRIVE_DEV)
        return os_file_exists(MULTIDRIVE_DEV);
#else
        return true; // FIXME?
#endif
    else
#endif
#ifdef HAVE_HOTSWAP_STORAGE_AS_MAIN
        return os_file_exists(ROOTDRIVE_DEV);
#else
        return true; /* internal: always present */
#endif
}

#ifdef HAVE_MULTIDRIVE
int volume_drive(int drive)
{
    return drive;
}
#endif /* HAVE_MULTIDRIVE */

#ifdef CONFIG_STORAGE_MULTI
int hostfs_driver_type(int drive)
{
#if (CONFIG_STORAGE & STORAGE_USB)
    return drive > 0 ? STORAGE_USB_NUM : STORAGE_HOSTFS_NUM;
#else
    return drive > 0 ? STORAGE_SD_NUM : STORAGE_HOSTFS_NUM;
#endif
}
#endif /* CONFIG_STORAGE_MULTI */

int hostfs_init(void)
{
    return 0;
}

int hostfs_flush(void)
{
    sync();
    return 0;
}

#ifdef HAVE_HOTSWAP
bool volume_removable(int volume)
{
    /* don't support more than one partition yet, so volume == drive */
    return hostfs_removable(volume);
}

bool volume_present(int volume)
{
    /* don't support more than one partition yet, so volume == drive */
    return hostfs_present(volume);
}
#endif

int volume_partition(int volume)
{
    (void)volume;
    /* Hosted only implement a single parition per "drive" */
    return 0;
}
