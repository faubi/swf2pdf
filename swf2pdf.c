#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

#include <swfdec/swfdec.h>
#include <cairo.h>
#include <cairo-pdf.h>

int skip = 0;
const char* output_file = "output.pdf";
char** input_files = NULL;
int num_input_files = 0;

void render_page(const char* filename, cairo_surface_t* surface, cairo_t* context) {
    char url_string[PATH_MAX + 7];
    strncpy(url_string, "file://", 7);
    realpath(filename, &url_string[7]);
    SwfdecURL* url = swfdec_url_new(url_string);
    if (url == NULL) {return;}
    
    SwfdecPlayer* player = swfdec_player_new(NULL);
    swfdec_player_set_url(player, url);
    swfdec_player_advance(player, 0); // Forces player to initialize
    
    guint width, height;
    swfdec_player_get_default_size(player, &width, &height);
    cairo_pdf_surface_set_size(surface, width, height); // Assume 72 dpi
    swfdec_player_render(player, context);
    cairo_surface_show_page(surface);
    
    g_object_unref(player);
    swfdec_url_free(url);
}
            
void from_args(cairo_surface_t* surface, cairo_t* context) {
    for (int file = 0; file < num_input_files; file++) {
        render_page(input_files[file], surface, context);
    }
}

void from_stdin(cairo_surface_t* surface, cairo_t* context) {
    char* line = NULL;
    size_t len = 0;
    ssize_t read = 0;
    while ((read = getline(&line, &len, stdin)) != -1) {
        line[read-1] = 0;
        render_page(line, surface, context);
    }
    free(line);
}

void usage() {
    printf("Usage: swf2pdf [-o OUTPUT_FILE] [swf_file...]\n");
    exit (EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
    char* arg;
    for (int i = 1; i < argc; i++) {
        arg = argv[i];
        if (strcmp(arg, "-o") == 0 || strcmp(arg, "--output") == 0) {
            if (++i >= argc) {
                usage();
            }
            output_file = argv[i];
//         }
//         else if (strcmp(arg, "-s") == 0 || strcmp(arg, "--skip") == 0) {
//             skip = 1;
        } else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            usage();
        } else {
            num_input_files++;
            input_files = realloc(input_files, num_input_files * sizeof(*input_files));
            input_files[num_input_files - 1] = arg;
        }
    }
    cairo_surface_t* surface = cairo_pdf_surface_create(output_file, 0, 0);
    cairo_t* context = cairo_create(surface);
    if (num_input_files > 0) {
        from_args(surface, context);
    } else {
        from_stdin(surface, context);
    }
    cairo_destroy(context);
    cairo_surface_destroy(surface);
    return 0;
}
