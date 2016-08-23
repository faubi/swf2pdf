#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>

#include <swfdec/swfdec.h>
#include <cairo.h>
#include <cairo-pdf.h>

int verbose = 0;
const char* output_file = "output.pdf";
char** input_files = NULL;
int num_input_files = 0;
int read_from_stdin = 0;
const char* program_name = "swf2pdf";

typedef enum {fail_on_error, skip_on_error, blank_on_error} error_mode;
error_mode on_error = blank_on_error;

void log_message(const char* format, ...) {
    if (!verbose) {
        return;
    }
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void handle_error(cairo_surface_t* surface, cairo_t* context) {
    switch (on_error) {
        case fail_on_error:
            log_message("Exited due to error\n");
            exit(EXIT_FAILURE);
            break;
        case blank_on_error:
            log_message("Rendering blank page due to error\n");
            cairo_surface_show_page(surface);
            break;
        case skip_on_error:
            log_message("Skipping page due to error\n");
            break; // Do nothing
    }
}

/* Render the given swf file to the next page of the surface */
void render_page(const char* filename, cairo_surface_t* surface, cairo_t* context) {
    log_message("Beginning render for %s\n", filename);
    char url_string[PATH_MAX + 7];
    strncpy(url_string, "file://", 7);
    realpath(filename, &url_string[7]);
    SwfdecURL* url = swfdec_url_new(url_string);
    if (url == NULL) {
        log_message("Error: Invalid url %s\n", url_string);
        handle_error(surface, context);
        return;
    }
    
    SwfdecPlayer* player = swfdec_player_new(NULL);
    swfdec_player_set_url(player, url);
    swfdec_player_advance(player, 0); // Forces player to initialize
    if (!swfdec_player_is_initialized(player)) {
        log_message("Error: Failed to load %s\n", filename);
        handle_error(surface, context);
        return;
    }
    
    guint width, height;
    swfdec_player_get_default_size(player, &width, &height);
    cairo_pdf_surface_set_size(surface, width, height); // Assume 72 dpi
    swfdec_player_render(player, context);
    cairo_surface_show_page(surface);
    log_message("Successfully rendered %s\n", filename);
    
    g_object_unref(player);
    swfdec_url_free(url);
}
            
int from_args(cairo_surface_t* surface, cairo_t* context) {
    for (int file = 0; file < num_input_files; file++) {
        render_page(input_files[file], surface, context);
    }
    return num_input_files;
}

int from_stdin(cairo_surface_t* surface, cairo_t* context) {
    char* line = NULL;
    size_t len = 0;
    ssize_t read = 0;
    int lines_read = 0;
    while ((read = getline(&line, &len, stdin)) != -1) {
        line[read-1] = 0;
        render_page(line, surface, context);
        lines_read++;
    }
    free(line);
    return lines_read;
}

void print_usage() {
    printf("Usage: %s [-s] [-v] [-o OUTPUTFILE] [-e ERROR_MODE] [SWF_FILE]...\n", program_name);
}

void usage(const char* message) {
    printf("Error: %s", message);
    print_usage();
    printf("Try '%s --help' for more information\n", program_name);
    exit (EXIT_FAILURE);
}

void help() {
    print_usage();
    printf("%s - Convert swf files to pdf\n\n", program_name);
    printf("Options:\n");
    printf(" -o, --output FILE       Set the output filename\n");
    printf(" -e, --error-mode MODE   Specify how to handle errors when loading swf files.\n");
    printf("                           MODE is 'fail' to cancel the entire conversion,\n");
    printf("                           'skip' to continue and skip the current page, or,\n");
    printf("                           'blank' to continue with the current page blank\n");
    printf(" -v, --verbose           Write information about status to output\n");
    printf(" -s, --stdin             Read file to load from stdin in addition to arguments\n");
    printf(" -h, --help              Display this information and exit\n");
    exit (EXIT_SUCCESS);
}

int main(int argc, char* argv[]) {
    char* arg;
    program_name = argv[0];
    for (int i = 1; i < argc; i++) {
        arg = argv[i];
        if (strcmp(arg, "-o") == 0 || strcmp(arg, "--output") == 0) {
            if (++i >= argc) {usage("Error: Missing argument for -o/--output");}
            output_file = argv[i];
        } else if (strcmp(arg, "-e") == 0 || strcmp(arg, "--error-mode") == 0) {
            if (++i >= argc) {usage("Error: Missing argument for -e/--error-mode");}
            if (argv[i][0] == 'b' || argv[i][0] == 'B') {
                on_error = blank_on_error;
            } else if (argv[i][0] == 'f' || argv[i][0] == 'F') {
                on_error = fail_on_error;
            } else if (argv[i][0] == 's' || argv[i][0] == 'S') {
                on_error = skip_on_error;
            }
        } else if (strcmp(arg, "-s") == 0 || strcmp(arg, "--stdin") == 0) {
            read_from_stdin = 1;
        } else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            help();
        } else if (strcmp(arg, "-v") == 0 || strcmp(arg, "--verbose") == 0) {
            verbose = 1;
        } else {
            num_input_files++;
            input_files = realloc(input_files, num_input_files * sizeof(*input_files));
            input_files[num_input_files - 1] = arg;
        }
    }
    if (!(num_input_files > 0 || read_from_stdin)) {
        usage("No input files supplied\n");
    }
    fclose(stderr); // Keeps swfdec from writing error messages to the terminal
    cairo_surface_t* surface = cairo_pdf_surface_create(output_file, 0, 0);
    cairo_t* context = cairo_create(surface);
    log_message("Created cairo surface and context\n");
    int pages = 0;
    pages += from_args(surface, context);
    if (read_from_stdin) {
        pages += from_stdin(surface, context);
    }
    log_message("Finished converting %d swf files\n", pages);
    cairo_destroy(context);
    cairo_surface_destroy(surface);
    return 0;
}
