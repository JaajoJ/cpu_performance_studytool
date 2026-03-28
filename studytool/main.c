#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "st_lib.h"
#include "st_idle_freq.h"
#include "st_output.h"

// Main function: entry point for execution

void help() {
    printf(
        "Usage: program [OPTIONS]\n"
        "\n"
        "Options:\n"
        "  -h            Show this help message and exit\n"
        "  -v            Show program version and exit\n"
        "  -c <core>     Select CPU core (default: all cores)\n"
        "  -m            Enable modify mode\n"
        "  -a            Enable apply mode\n"
        "  -o <mode>     Output format\n"
        "                  h  human-readable (default)\n"
        "                  j  JSON\n"
        "Examples:\n"
        "  idlefreq_stat -i\n"
        "      Show idle information\n"
        "  idlefreq_stat -f -c 2\n"
        "      Show frequency information for core 2\n"
        "  program -i -o json\n"
        "      Show frequency information in JSON format\n"
        "\n"
    );
}

void version() {
    printf("Version: %s\n", RELEASE_NAME);
}

int main(int argc, char *argv[]) {

    // Writing print statement to print hello world
    
    int opt;

    // Defaults
    int core = -1;
    int modify = false;
    int apply = false;
    int ret = 0;
    char output_mode = 'h'; // j = json, h = human readable
    
    while ( (opt = getopt(argc, argv, "hvo:mac:")) != -1 )
    {
        switch (opt) {

            case 'h':
                help();
                return 0;
                break;

            case 'v':
                version();
                return 0;
                break;

            case 'o':
                output_mode = optarg[0];
                break;
            
            case 'c':
                core = atoi(optarg);
                break;

            case 'm':
                modify = true;
                break;
            case 'a':
                apply = true;
                break;

            default:
                fprintf(stderr, "Invalid argument...\n");
                help();
                return -1;


        }
    }

    if (modify)
    {
        // If file does not exists. Create one...
        if (access(ST_CONFIG_DEFAULT_PATH, F_OK | R_OK) != 0)
        {
            ret = st_set_default_config(ST_CONFIG_DEFAULT_PATH);
            if( ret )
            {
                fprintf(stderr, "Unable to create config file: ");
                fprintf(stderr, ST_CONFIG_DEFAULT_PATH);
                fprintf(stderr, "\n");
                return 1;
            }
        }
        st_modify(ST_CONFIG_DEFAULT_PATH);
        return 0;
    }

    if (apply)
    {
        STConfig config;
        
        ret = st_get_config(&config, ST_CONFIG_DEFAULT_PATH);
        if (ret)
        {
            fprintf(stderr, "Please create the configs using -m argument\n");
            return 1;
        }
        return st_apply(&config);
    }

    // ALLOWED OUTPUT FORMATS
    const char * supported_outputs = ST_SUPPORTED_OUTPUT;
    char * chr_found = strchr(supported_outputs, output_mode); // if suggested output is not found in supported modes MACRO
    if ( ! chr_found )
    {
        fprintf(stderr, "%s %c\n", "Non supported output mode:", output_mode);
        return -1;
    }

    // CURRENT MODE: idle, frequency
    
    return st_collect(core, output_mode);
    
}