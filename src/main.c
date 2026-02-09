/*******************************************************************
 *
 * main.c - LVGL simulator for GNU/Linux
 *
 * Based on the original file from the repository
 *
 * @note eventually this file won't contain a main function and will
 * become a library supporting all major operating systems
 *
 * To see how each driver is initialized check the
 * 'src/lib/display_backends' directory
 *
 * - Clean up
 * - Support for multiple backends at once
 *   2025 EDGEMTech Ltd.
 *
 * Author: EDGEMTech Ltd, Erik Tagirov (erik.tagirov@edgemtech.ch)
 *
 ******************************************************************/
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"

#include "src/lib/driver_backends.h"
#include "src/lib/simulator_util.h"
#include "src/lib/simulator_settings.h"

/* Internal functions */
static void configure_simulator(int argc, char ** argv);
static void print_lvgl_version(void);
static void print_usage(void);

/* contains the name of the selected backend if user
 * has specified one on the command line */
static char * selected_backend;

#if USE_STANDARD
/* Global simulator settings, defined in lv_linux_backend.c */
extern simulator_settings_t settings;


/**
 * @brief Print LVGL version
 */
static void print_lvgl_version(void)
{
    fprintf(stdout, "%d.%d.%d-%s\n",
            LVGL_VERSION_MAJOR,
            LVGL_VERSION_MINOR,
            LVGL_VERSION_PATCH,
            LVGL_VERSION_INFO);
}

/**
 * @brief Print usage information
 */
static void print_usage(void)
{
    fprintf(stdout, "\nlvglsim [-V] [-B] [-f] [-m] [-b backend_name] [-W window_width] [-H window_height]\n\n");
    fprintf(stdout, "-V print LVGL version\n");
    fprintf(stdout, "-B list supported backends\n");
    fprintf(stdout, "-f fullscreen\n");
    fprintf(stdout, "-m maximize\n");
}

/**
 * @brief Configure simulator
 * @description process arguments received by the program to select
 * appropriate options
 * @param argc the count of arguments in argv
 * @param argv The arguments
 */
static void configure_simulator(int argc, char ** argv)
{
    int opt = 0;

    selected_backend = NULL;
    driver_backends_register();

    const char * env_w = getenv("LV_SIM_WINDOW_WIDTH");
    const char * env_h = getenv("LV_SIM_WINDOW_HEIGHT");
    /* Default values */
    settings.window_width = atoi(env_w ? env_w : "800");
    settings.window_height = atoi(env_h ? env_h : "480");

    /* Parse the command-line options. */
    while((opt = getopt(argc, argv, "b:fmW:H:BVh")) != -1) {
        switch(opt) {
            case 'h':
                print_usage();
                exit(EXIT_SUCCESS);
                break;
            case 'V':
                print_lvgl_version();
                exit(EXIT_SUCCESS);
                break;
            case 'B':
                driver_backends_print_supported();
                exit(EXIT_SUCCESS);
                break;
            case 'b':
                if(driver_backends_is_supported(optarg) == 0) {
                    die("error no such backend: %s\n", optarg);
                }
                selected_backend = strdup(optarg);
                break;
            case 'f':
                settings.fullscreen = true;
                break;
            case 'm':
                settings.maximize = true;
                break;
            case 'W':
                settings.window_width = atoi(optarg);
                break;
            case 'H':
                settings.window_height = atoi(optarg);
                break;
            case ':':
                print_usage();
                die("Option -%c requires an argument.\n", optopt);
                break;
            case '?':
                print_usage();
                die("Unknown option -%c.\n", optopt);
        }
    }
}

/**
 * @brief entry point
 * @description start a demo
 * @param argc the count of arguments in argv
 * @param argv The arguments
 */
int main(int argc, char ** argv)
{

    configure_simulator(argc, argv);

    /* Initialize LVGL. */
    lv_init();

    /* Initialize the configured backend */
    if(driver_backends_init_backend(selected_backend) == -1) {
        die("Failed to initialize display backend");
    }

    /* Enable for EVDEV support */
#if LV_USE_EVDEV
    if(driver_backends_init_backend("EVDEV") == -1) {
        die("Failed to initialize evdev");
    }
#endif

    /*Create a Demo*/
    lv_demo_widgets();
    lv_demo_widgets_start_slideshow();

    /* Enter the run loop of the selected backend */
    driver_backends_run_loop();

    return 0;
}

#else

void debug_print_lvgl_buffer(void)
{
    lv_display_t * disp = lv_display_get_default();
    lv_draw_buf_t * buf = lv_display_get_buf_active(disp);
    uint8_t * data = buf->data;
    uint32_t w = lv_display_get_horizontal_resolution(disp);
    uint32_t h = lv_display_get_vertical_resolution(disp);

    printf("=== LVGL DRAW BUFFER (%dx%d) ===\n", w, h);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {

            int byte_index = y * 16 + (x / 8);
            int bit_index  = 7 - (x % 8);   // bit7 = 左边像素

            int pixel = (data[byte_index] >> bit_index) & 0x01;

            putchar(pixel ? '*' : '-');
        }
        putchar('\n');
    }
}

static void draw_label(void)
{
    // 1. 创建新屏幕对象
    lv_obj_t * screen = lv_obj_create(NULL);
    
    // 2. 设置屏幕为全屏（移除默认边距）
    lv_obj_set_size(screen, 128, 64);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    
    // 3. 设置单色屏专用样式
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    
    // 4. 创建标签
    lv_obj_t * label = lv_label_create(screen);
    lv_label_set_text(label, LV_SYMBOL_OK);
    
    // 5. 设置标签样式（高对比度）
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    
    // 6. 居中标签
    lv_obj_center(label);
    
    // 7. 加载屏幕
    lv_scr_load(screen); 

}
static void draw_char_example(void)
{
    /* 设置屏幕背景为黑色 */
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), LV_PART_MAIN);

    /* 创建一个 label 对象 */
    lv_obj_t * label = lv_label_create(lv_screen_active());

    /* 设置要显示的文本 */
    lv_label_set_text(label, "hello world");

    /* 设置字体颜色为白色（单色屏就是亮点） */
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);

    /* 设置字体大小：选择 LVGL 内置字体，比如 20 像素 */
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, LV_PART_MAIN);

    /* 居中显示 */
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}


int main(int argc, char **argv){

    driver_backends_register();

    selected_backend = "FBDEV";

    lv_init();

    //registerbackends and then selected_backend
    /* Initialize the configured backend */
    if(driver_backends_init_backend(selected_backend) == -1) {
        die("Failed to initialize display backend");
    }

    /* Show a char example  */
    draw_char_example();

    //draw_label();

    /* Enter the run loop of the selected backend */
    driver_backends_run_loop();

    while (1)
    {
        /* code */
    }
    
    return 0;
}



#endif