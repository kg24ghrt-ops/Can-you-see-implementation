/* examples/render_samples.c
 *
 * Generates sample PPM images demonstrating paper materials and layouts.
 * PPM (P6) is used so no image library is needed. Convert with any image
 * tool, e.g.  magick page.ppm page.png   or   ffmpeg -i page.ppm page.png
 *
 * Build: compile this file with the C sources in src/paper/ and link the math
 * library. Then run ./render_samples; it writes PPMs to build/samples/.
 */

#include "paper/paper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static void write_ppm(const char *path, const paper_pixel_buffer_t *rgba) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "cannot open %s\n", path);
        return;
    }
    fprintf(f, "P6\n%d %d\n255\n", rgba->width, rgba->height);
    for (int y = 0; y < rgba->height; y++) {
        const unsigned char *row = rgba->data + (size_t)y * rgba->stride;
        for (int x = 0; x < rgba->width; x++) {
            unsigned char px[3] = {row[(size_t)x * 4 + 0],
                                   row[(size_t)x * 4 + 1],
                                   row[(size_t)x * 4 + 2]};
            fwrite(px, 1, 3, f);
        }
    }
    fclose(f);
}

static paper_engine_t *make_engine(paper_material_id_t mat,
                                   paper_layout_id_t lay, uint32_t seed) {
    paper_engine_t *e = NULL;
    paper_engine_create(&e);
    paper_engine_set_dpi(e, 150.0);
    paper_engine_set_seed(e, seed);
    paper_engine_set_material(e, mat, NULL);
    paper_engine_set_layout(e, lay, NULL);
    return e;
}

int main(void) {
    const char *outdir = "build/samples";
    mkdir(outdir, 0755);

    struct {
        const char *name;
        paper_material_id_t mat;
    } materials[] = {
        {"natural_white", PAPER_MATERIAL_NATURAL_WHITE},
        {"warm_white", PAPER_MATERIAL_WARM_WHITE},
        {"cream", PAPER_MATERIAL_CREAM},
        {"ivory", PAPER_MATERIAL_IVORY},
        {"aged", PAPER_MATERIAL_AGED},
        {"recycled", PAPER_MATERIAL_RECYCLED},
    };

    for (size_t i = 0; i < sizeof(materials) / sizeof(materials[0]); i++) {
        paper_engine_t *e = make_engine(materials[i].mat, PAPER_LAYOUT_BLANK, 7u);
        paper_pixel_buffer_t buf = {0};
        buf.format = PAPER_PIXEL_RGBA8888;
        paper_engine_render(e, &buf);
        char path[260];
        snprintf(path, sizeof(path), "%s/blank_%s.ppm", outdir, materials[i].name);
        write_ppm(path, &buf);
        paper_buffer_release(&buf);
        paper_engine_destroy(e);
    }

    struct {
        const char *name;
        paper_layout_id_t lay;
    } layouts[] = {
        {"blank", PAPER_LAYOUT_BLANK},
        {"ruled", PAPER_LAYOUT_RULED},
        {"college", PAPER_LAYOUT_COLLEGE_RULED},
        {"wide", PAPER_LAYOUT_WIDE_RULED},
        {"graph", PAPER_LAYOUT_GRAPH},
        {"dotgrid", PAPER_LAYOUT_DOT_GRID},
    };

    for (size_t i = 0; i < sizeof(layouts) / sizeof(layouts[0]); i++) {
        paper_engine_t *e = make_engine(PAPER_MATERIAL_NATURAL_WHITE, layouts[i].lay, 7u);
        paper_pixel_buffer_t buf = {0};
        buf.format = PAPER_PIXEL_RGBA8888;
        paper_engine_render(e, &buf);
        char path[260];
        snprintf(path, sizeof(path), "%s/layout_%s.ppm", outdir, layouts[i].name);
        write_ppm(path, &buf);
        paper_buffer_release(&buf);
        paper_engine_destroy(e);
    }

    /* Stroke sample: ballpoint on ruled paper. */
    {
        paper_engine_t *e = make_engine(PAPER_MATERIAL_NATURAL_WHITE,
                                        PAPER_LAYOUT_COLLEGE_RULED, 11u);
        paper_stroke_point_t pts[] = {
            {50.0, 200.0, 0.5, 1.0, 0.0, 0.0, 0.0},
            {100.0, 180.0, 0.7, 1.0, 0.0, 0.0, 0.0},
            {160.0, 210.0, 0.5, 0.9, 0.0, 0.0, 0.0},
            {220.0, 190.0, 0.6, 1.0, 0.0, 0.0, 0.0},
        };
        paper_stroke_t s = {pts, 4, PAPER_INK_BALLPOINT, 0.05, 0.05, 0.05, 0.9};
        paper_engine_add_stroke(e, &s);
        paper_pixel_buffer_t buf = {0};
        buf.format = PAPER_PIXEL_RGBA8888;
        paper_engine_render(e, &buf);
        write_ppm("build/samples/stroke_ballpoint.ppm", &buf);
        paper_buffer_release(&buf);
        paper_engine_destroy(e);
    }

    printf("sample PPMs written to %s/\n", outdir);
    return 0;
}
