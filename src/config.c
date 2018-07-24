
#include "config.h"
#include "nvm.h"
#include "stdlib.h"
#include "printf.h"
#include "utils.h"

unsigned int cfg_led_count = 0;
unsigned int cfg_brightness = 4;
unsigned int cfg_program = 0;

typedef struct {
    const char* name;
    void (*func)(char*);
} config_t;

void set_strip_length(char* arg);
void set_start_program(char* arg);
void set_brightness(char* arg);

config_t configs[] = {
    {"LEDS", &set_strip_length},
    {"PROG", &set_start_program},
    {"BRI", &set_brightness},

};

const char* copy_line(const char* src, char* line_buffer, size_t buffer_size)
{
    const char* end_pos = strchr(src, '\n');
    if(end_pos == NULL)
        return NULL;

    size_t len = end_pos - src;
    if(len > buffer_size - 1)
        len = buffer_size - 1;
    
    memcpy(line_buffer, src, len);
    line_buffer[len] = '\0';
 
    return end_pos + 1;
}

int is_command_name(char* cmd, config_t* cfg)
{
    //Convert to upper case
    char* str = cmd;
    while(*str) {
        *str = toupper(*str);
        str++;
    }

    //Remove white space
    char* ws = strpbrk(cmd, " \t\n\r");
    if(ws)
        *ws = '\0';

    return !strcmp(cmd, cfg->name);
}

void config_run_line(char* line)
{
    //Split the line to command and argument parts
    char* arg;
    char* cmd = strtok_r(line, ":", &arg);

    int i;
    for(i = 0; i < sizeof(configs) / sizeof(configs[0]); i++) {
        if(is_command_name(cmd, &configs[i])) {
            configs[i].func(arg);
        }
    }
}


void config_load()
{
    const char* cfg = get_file_ptr(7);

    char buffer[128];
    while(cfg = copy_line(cfg, buffer, sizeof(buffer))) {
        config_run_line(buffer);
    }

}

void set_strip_length(char* arg)
{
    int leds = atoi(arg);
    cfg_led_count = clamp(leds, 0, 299);

    printf("CFG:LEDS: %i\n", cfg_led_count);
}

void set_start_program(char* arg)
{
    int prog = atoi(arg);
    cfg_program = clamp(prog - 1, 0, 6);

    printf("CFG:PROG: %i\n", cfg_program);
}

void set_brightness(char* arg)
{
    int b = atoi(arg);
    cfg_brightness = clamp(b, 1, 4);

    printf("CFG:BRI: %i\n", cfg_brightness);
}
