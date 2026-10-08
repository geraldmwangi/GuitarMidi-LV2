/*
 * GuitarMidi-LV2 - X11/Cairo UI
 *
 * A self-contained modern dark UI for the GuitarMidi plugin.
 * Uses raw Xlib + Cairo, implements the LV2UI X11 interface with
 * the ui:idleInterface extension (no toolkit event loop needed).
 *
 * License: ISC (same as plugin)
 */

#include <lv2/core/lv2.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/util.h>
#include <lv2/urid/urid.h>
#include <lv2/ui/ui.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <array>
#include <string>
#include <vector>

#define GUITARMIDI_URI "http://github.com/geraldmwangi/GuitarMidi-LV2"
#define GUITARMIDI_UI_URI GUITARMIDI_URI "#ui"

// ---------------------------------------------------------------------------
// Layout constants
// ---------------------------------------------------------------------------
static const int UI_W = 1000;
static const int UI_H = 280;
static const int DEBUG_W = 800;
static const int DEBUG_H = 560;
static const size_t NOTE_COUNT = 37;
static const size_t PLOT_COUNT = 4;
static const size_t HISTORY_SIZE = 960;
static const size_t DEFAULT_VISIBLE_COLUMNS = 240;
static const size_t MIN_VISIBLE_COLUMNS = 30;
static const double DEBUG_UPDATES_PER_SECOND = 20.0;

// ---------------------------------------------------------------------------
// Color helpers
// ---------------------------------------------------------------------------
struct Color {
    double r, g, b;
};

static inline void set_color(cairo_t* cr, Color c, double a = 1.0)
{
    cairo_set_source_rgba(cr, c.r, c.g, c.b, a);
}

// Palette (modern dark)
static const Color COL_BG_TOP     = {0.086, 0.094, 0.118}; // #161e1e
static const Color COL_BG_BOT     = {0.055, 0.059, 0.075};
static const Color COL_PANEL      = {0.118, 0.129, 0.161};
static const Color COL_PANEL_EDGE = {0.20, 0.22, 0.27};
static const Color COL_TEXT       = {0.90, 0.91, 0.93};
static const Color COL_TEXT_DIM   = {0.55, 0.58, 0.64};
static const Color COL_ACCENT     = {0.00, 0.78, 0.60};  // teal/green
static const Color COL_ACCENT2    = {1.00, 0.62, 0.20};  // orange (offset group)
static const Color COL_ACCENT3    = {0.35, 0.62, 1.00};  // blue (input group)
static const Color COL_TRACK      = {0.22, 0.24, 0.29};
static const Color COL_KNOB_FACE  = {0.16, 0.175, 0.215};
static const Color COL_KNOB_EDGE  = {0.30, 0.33, 0.39};

// ---------------------------------------------------------------------------
// Knob widget model
// ---------------------------------------------------------------------------
struct Knob {
    uint32_t    port;       // LV2 port index
    const char* label;
    const char* unit;       // may be ""

    const char* tooltip; //the description of the parameter
    float       min, max, def;
    int         decimals;   // display precision
    Color       accent;
    // geometry (computed in layout)
    double cx, cy, radius;
    // state
    float value;
};

struct DebugPlot {
    Window window = 0;
    cairo_surface_t* surface = nullptr;
    cairo_surface_t* backbuffer = nullptr;
    int width = DEBUG_W;
    int height = DEBUG_H;
    bool visible = false;
    bool dirty = false;
    size_t next_column = 0;
    size_t columns_filled = 0;
    size_t visible_columns = DEFAULT_VISIBLE_COLUMNS;
    std::array<std::array<float, NOTE_COUNT>, HISTORY_SIZE> history = {};
};

struct GuitarMidiUI {
    // LV2 plumbing
    LV2UI_Write_Function write;
    LV2UI_Controller     controller;
    uint32_t debug_port;
    LV2_URID event_transfer;
    LV2_URID vector_type;
    LV2_URID float_type;
    Atom wm_delete;

    // X11
    Display* dpy;
    Window   parent;
    Window   win;
    int      screen;
    bool     mapped;

    // Cairo
    cairo_surface_t* surface;

    // Widgets
    std::vector<Knob> knobs;
    DebugPlot plots[PLOT_COUNT];

    // Interaction state
    int    drag_knob;     // index into knobs, -1 = none
    double drag_start_y;
    float  drag_start_val;
    bool   fine_drag;
    int    hover_knob;

    bool dirty;
};

// ---------------------------------------------------------------------------
// Knob definitions (ports 2..9)
// ---------------------------------------------------------------------------
static void init_knobs(GuitarMidiUI* ui)
{
    ui->knobs = {
        // port, label, unit,tooltip, min, max, default, decimals, accent
        {2, "Input Gain",   "dB","Add gain to the input signal prior to processing", -20.0f, 40.0f,  28.0f,  1, COL_ACCENT3, 0, 0, 0, 28.0f},
        {3, "Expressivity", "dB", "The range of the attack velocity . For low values any attack strength of the string produces max velocity, passed to the synth", -3.0f, 20.0f,  3.0f,  1, COL_ACCENT3, 0, 0, 0, 3.0f},
        // onset knobs
        {4, "Smoothing",    "",   "Timewise smoothing of the note begin to avoid jitter. High values produce smoother predictions at the expense of latency",   0.0f,  0.9f,  0.3f,  2, COL_ACCENT,  0, 0, 0, 0.3f},
        {6, "Confidence",   "",   "The confidence level of the note beginning. Use to filter out false notes. High values lead to more conservative note detection",   0.0f,  0.99f, 0.95f, 2, COL_ACCENT,  0, 0, 0, 0.95f},
        {8, "Energy",       "dB", "The energy threshold for note detection. Lower values make the detector more sensitive to quieter notes", -20.0f,  3.0f,  -18.0f,  1, COL_ACCENT,  0, 0, 0, -18.0f},

        // offset knobs
        {5, "Smoothing",    "",   "Timewise smoothing of the note ends to avoid jitter. Lower values lead to notes being released more quickly",   0.0f,  0.9f,  0.3f,  2, COL_ACCENT2, 0, 0, 0, 0.3f},
        {7, "Confidence",   "",   "The confidence level of the note ending. Lower values make the notes linger on",   0.0f,  0.99f, 0.3f, 2, COL_ACCENT2, 0, 0, 0, 0.3f},
        {9, "Energy",       "dB", "The energy threshold for note detection. Lower values make the notes linger on, especially when chords change fast", -20.0f,  3.0f, -18.0f, 1, COL_ACCENT2, 0, 0, 0, -18.0f},
    };
    for (auto& k : ui->knobs)
        k.value = k.def;
}



// Panel geometry ------------------------------------------------------------
struct Panel {
    const char* title;
    Color       accent;
    double x, y, w, h;
    int first_knob, num_knobs;
};

static const double HEADER_H = 64;

static void get_panels(Panel panels[3])
{
    const double margin = 16;
    const double top = HEADER_H + 12;
    const double h = UI_H - top - margin;

    // INPUT panel: 2 knobs; ONSET: 3 knobs; OFFSET: 3 knobs
    const double gap = 12;
    const double total_w = UI_W - 2 * margin - 2 * gap;
    const double w_small = total_w * 2.0 / 8.0;
    const double w_big   = total_w * 3.0 / 8.0;

    panels[0] = {"INPUT",       COL_ACCENT3, margin, top, w_small, h, 0, 2};
    panels[1] = {"NOTE ONSET",  COL_ACCENT,  margin + w_small + gap, top, w_big, h, 2, 3};
    panels[2] = {"NOTE OFFSET", COL_ACCENT2, margin + w_small + gap + w_big + gap, top, w_big, h, 5, 3};
}

static void layout_knobs(GuitarMidiUI* ui)
{
    Panel panels[3];
    get_panels(panels);

    for (int p = 0; p < 3; ++p) {
        const Panel& pn = panels[p];
        const double cell_w = pn.w / pn.num_knobs;
        for (int i = 0; i < pn.num_knobs; ++i) {
            Knob& k = ui->knobs[pn.first_knob + i];
            k.cx = pn.x + cell_w * (i + 0.5);
            k.cy = pn.y + 46 + 58; // title area + knob center
            k.radius = 34;
        }
    }
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------
static void rounded_rect(cairo_t* cr, double x, double y, double w, double h, double r)
{
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r, r, -M_PI_2, 0);
    cairo_arc(cr, x + w - r, y + h - r, r, 0, M_PI_2);
    cairo_arc(cr, x + r, y + h - r, r, M_PI_2, M_PI);
    cairo_arc(cr, x + r, y + r, r, M_PI, 1.5 * M_PI);
    cairo_close_path(cr);
}


// Draw the tooptip for a knob
static void draw_tooltip(cairo_t* cr, const Knob& k){
    cairo_select_font_face(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);

    cairo_set_font_size(cr,11);
    cairo_text_extents_t ext;
    cairo_text_extents(cr,k.tooltip,&ext);

    const double padding_x=10;
    const double padding_y=7;

    const double width=ext.width+padding_x*2;
    const double height=24;

    double x=k.cx-width/2;
    double y=k.cy-k.radius-height-20;

    if (x < 6)
        x = 6;
    if (x + width > UI_W - 6)
        x = UI_W - width - 6;
    if (y < 6)
        y = 6;

    set_color(cr,COL_PANEL_EDGE,0.98);
    rounded_rect(cr,x,y,width,height,5);
    cairo_fill(cr);
    
    //show the text
    set_color(cr,COL_TEXT);
    cairo_move_to(cr,x+padding_x,y+padding_y+height/2);
    cairo_show_text(cr,k.tooltip);

}
static void draw_text(cairo_t* cr, double x, double y, const char* text,
                      double size, Color col, bool bold, bool center)
{
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL,
                           bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, size);
    set_color(cr, col);
    if (center) {
        cairo_text_extents_t ext;
        cairo_text_extents(cr, text, &ext);
        x -= ext.width / 2 + ext.x_bearing;
    }
    cairo_move_to(cr, x, y);
    cairo_show_text(cr, text);
}

static void format_value(const Knob& k, char* buf, size_t n)
{
    if (k.unit[0])
        snprintf(buf, n, "%.*f %s", k.decimals, k.value, k.unit);
    else
        snprintf(buf, n, "%.*f", k.decimals, k.value);
}

static void draw_knob(cairo_t* cr, const Knob& k, bool hovered)
{
    cairo_new_path(cr); // avoid connecting lines from previous text/current point

    const double a_start = 0.75 * M_PI;  // 135 deg
    const double a_range = 1.5 * M_PI;   // 270 deg sweep
    const double norm = (k.value - k.min) / (k.max - k.min);
    const double a_val = a_start + norm * a_range;

    // Track arc
    cairo_set_line_width(cr, 5.0);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    set_color(cr, COL_TRACK);
    cairo_arc(cr, k.cx, k.cy, k.radius, a_start, a_start + a_range);
    cairo_stroke(cr);

    // Value arc (glow underlay + main)
    set_color(cr, k.accent, 0.25);
    cairo_set_line_width(cr, 9.0);
    cairo_arc(cr, k.cx, k.cy, k.radius, a_start, a_val);
    cairo_stroke(cr);

    set_color(cr, k.accent);
    cairo_set_line_width(cr, 5.0);
    cairo_arc(cr, k.cx, k.cy, k.radius, a_start, a_val);
    cairo_stroke(cr);

    // Knob face (radial-ish shading)
    const double fr = k.radius - 9;
    cairo_pattern_t* grad = cairo_pattern_create_linear(
        k.cx, k.cy - fr, k.cx, k.cy + fr);
    cairo_pattern_add_color_stop_rgb(grad, 0,
        COL_KNOB_FACE.r + 0.05, COL_KNOB_FACE.g + 0.05, COL_KNOB_FACE.b + 0.05);
    cairo_pattern_add_color_stop_rgb(grad, 1,
        COL_KNOB_FACE.r - 0.02, COL_KNOB_FACE.g - 0.02, COL_KNOB_FACE.b - 0.02);
    cairo_set_source(cr, grad);
    cairo_arc(cr, k.cx, k.cy, fr, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_pattern_destroy(grad);

    set_color(cr, hovered ? k.accent : COL_KNOB_EDGE, hovered ? 0.8 : 1.0);
    cairo_set_line_width(cr, 1.2);
    cairo_arc(cr, k.cx, k.cy, fr, 0, 2 * M_PI);
    cairo_stroke(cr);

    // Pointer
    const double px1 = k.cx + cos(a_val) * (fr * 0.35);
    const double py1 = k.cy + sin(a_val) * (fr * 0.35);
    const double px2 = k.cx + cos(a_val) * (fr * 0.85);
    const double py2 = k.cy + sin(a_val) * (fr * 0.85);
    set_color(cr, k.accent);
    cairo_set_line_width(cr, 3.0);
    cairo_move_to(cr, px1, py1);
    cairo_line_to(cr, px2, py2);
    cairo_stroke(cr);

    // Label above, value below
    draw_text(cr, k.cx, k.cy - k.radius - 12, k.label, 12, COL_TEXT, false, true);

    char buf[48];
    format_value(k, buf, sizeof(buf));
    draw_text(cr, k.cx, k.cy + k.radius + 20, buf, 12,
              hovered ? COL_TEXT : COL_TEXT_DIM, hovered, true);
}

static void draw_ui(GuitarMidiUI* ui)
{
    cairo_t* cr = cairo_create(ui->surface);

    // Background gradient
    cairo_pattern_t* bg = cairo_pattern_create_linear(0, 0, 0, UI_H);
    cairo_pattern_add_color_stop_rgb(bg, 0, COL_BG_TOP.r, COL_BG_TOP.g, COL_BG_TOP.b);
    cairo_pattern_add_color_stop_rgb(bg, 1, COL_BG_BOT.r, COL_BG_BOT.g, COL_BG_BOT.b);
    cairo_set_source(cr, bg);
    cairo_paint(cr);
    cairo_pattern_destroy(bg);

    // Header ---------------------------------------------------------------
    // subtle accent strip
    set_color(cr, COL_ACCENT, 0.9);
    cairo_rectangle(cr, 0, 0, UI_W, 3);
    cairo_fill(cr);

    draw_text(cr, 20, 38, "GUITAR", 24, COL_TEXT, true, false);
    cairo_text_extents_t ext;
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 24);
    cairo_text_extents(cr, "GUITAR", &ext);
    draw_text(cr, 20 + ext.width + 8, 38, "MIDI", 24, COL_ACCENT, true, false);

    draw_text(cr, 20, 55, "Neural guitar-to-MIDI converter", 11, COL_TEXT_DIM, false, false);

    const double button_y = 15;
    const double button_w = 112;
    const double button_h = 30;
    const double button_gap = 6;
    const double button_start = UI_W - 16 - PLOT_COUNT * button_w - (PLOT_COUNT - 1) * button_gap;
    const Color button_colors[PLOT_COUNT] = {COL_ACCENT, COL_ACCENT2, COL_ACCENT, COL_ACCENT2};
    const char* button_labels[PLOT_COUNT] = {
        "CONF SMOOTH", "ENERGY SMOOTH", "CONF RAW", "ENERGY RAW"
    };
    for (size_t i = 0; i < PLOT_COUNT; ++i) {
        const double x = button_start + i * (button_w + button_gap);
        set_color(cr, COL_PANEL);
        rounded_rect(cr, x, button_y, button_w, button_h, 4);
        cairo_fill(cr);
        set_color(cr, button_colors[i], 0.75);
        cairo_set_line_width(cr, 1.0);
        rounded_rect(cr, x + 0.5, button_y + 0.5,
                     button_w - 1, button_h - 1, 4);
        cairo_stroke(cr);
        draw_text(cr, x + button_w / 2, button_y + 19,
                  button_labels[i], 10, COL_TEXT, true, true);
    }

    // Panels -----------------------------------------------------------------
    Panel panels[3];
    get_panels(panels);

    for (int p = 0; p < 3; ++p) {
        const Panel& pn = panels[p];

        set_color(cr, COL_PANEL);
        rounded_rect(cr, pn.x, pn.y, pn.w, pn.h, 10);
        cairo_fill(cr);

        set_color(cr, COL_PANEL_EDGE, 0.6);
        cairo_set_line_width(cr, 1.0);
        rounded_rect(cr, pn.x + 0.5, pn.y + 0.5, pn.w - 1, pn.h - 1, 10);
        cairo_stroke(cr);

        // Panel title with accent tick
        set_color(cr, pn.accent);
        rounded_rect(cr, pn.x + 14, pn.y + 14, 4, 14, 2);
        cairo_fill(cr);
        draw_text(cr, pn.x + 26, pn.y + 26, pn.title, 13, COL_TEXT, true, false);
    }

    // Knobs
    for (size_t i = 0; i < ui->knobs.size(); ++i)
        draw_knob(cr, ui->knobs[i], (int)i == ui->hover_knob);

    if(ui->hover_knob>=0)
        draw_tooltip(cr,ui->knobs[ui->hover_knob]);
    // Footer hint (centered)
    draw_text(cr, UI_W / 2.0, UI_H - 6,
              "drag: adjust    shift+drag: fine    double-click: reset    scroll: step", 10,
              COL_TEXT_DIM, false, true);

    cairo_destroy(cr);
    cairo_surface_flush(ui->surface);
}

static void open_plot_window(GuitarMidiUI* ui, int plot_index)
{
    DebugPlot& plot = ui->plots[plot_index];
    if (!plot.window) {
        plot.window = XCreateSimpleWindow(ui->dpy, RootWindow(ui->dpy, ui->screen),
            80 + plot_index * 50, 80 + plot_index * 50, DEBUG_W, DEBUG_H,
            0, BlackPixel(ui->dpy, ui->screen), BlackPixel(ui->dpy, ui->screen));
        XSelectInput(ui->dpy, plot.window,
                 ExposureMask | StructureNotifyMask | ButtonPressMask);
        static const char* window_titles[PLOT_COUNT] = {
            "GuitarMidi Smoothed Note Confidences",
            "GuitarMidi Smoothed Note Energies",
            "GuitarMidi Momentary Note Confidences",
            "GuitarMidi Momentary Note Energies"
        };
        XStoreName(ui->dpy, plot.window, window_titles[plot_index]);
        XSetWMProtocols(ui->dpy, plot.window, &ui->wm_delete, 1);
        XSizeHints hints = {};
        hints.flags = PMinSize;
        hints.min_width = 500;
        hints.min_height = 320;
        XSetWMNormalHints(ui->dpy, plot.window, &hints);
        plot.surface = cairo_xlib_surface_create(ui->dpy, plot.window,
            DefaultVisual(ui->dpy, ui->screen), plot.width, plot.height);
        plot.backbuffer = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
            plot.width, plot.height);
    }
    plot.visible = true;
    plot.dirty = true;
    XMapRaised(ui->dpy, plot.window);
    XFlush(ui->dpy);
}

static void draw_debug_plot(GuitarMidiUI* ui, int plot_index)
{
    DebugPlot& plot = ui->plots[plot_index];
    if (!plot.surface || !plot.backbuffer || !plot.visible)
        return;

    cairo_t* cr = cairo_create(plot.backbuffer);
    cairo_set_source_rgb(cr, COL_BG_BOT.r, COL_BG_BOT.g, COL_BG_BOT.b);
    cairo_paint(cr);

    static const char* titles[PLOT_COUNT] = {
        "SMOOTHED CONFIDENCE", "SMOOTHED ENERGY",
        "MOMENTARY CONFIDENCE", "MOMENTARY ENERGY"
    };
    const bool is_confidence = plot_index == 0 || plot_index == 2;
    const char* title = titles[plot_index];
    const Color accent = is_confidence ? COL_ACCENT : COL_ACCENT2;
    draw_text(cr, 20, 30, title, 16, COL_TEXT, true, false);
    char duration[32];
    snprintf(duration, sizeof(duration), "%.1f s", plot.visible_columns / DEBUG_UPDATES_PER_SECOND);
    draw_text(cr, 190, 30, duration, 11, COL_TEXT_DIM, false, false);
    draw_text(cr, plot.width - 55, 29, "older", 10, COL_TEXT_DIM, false, false);
    draw_text(cr, plot.width - 55, 43, "newer", 10, COL_TEXT_DIM, false, false);

    const double label_w = 58;
    const double chart_x = label_w;
    const double chart_y = 56;
    const double chart_w = plot.width - label_w - 18;
    const double chart_h = plot.height - chart_y - 18;
    const double cell_w = chart_w / plot.visible_columns;
    const double cell_h = chart_h / NOTE_COUNT;
    const char* pitch_names[] = {"C", "C#", "D", "D#", "E", "F",
                                 "F#", "G", "G#", "A", "A#", "B"};
    const size_t visible_count = std::min(plot.columns_filled, plot.visible_columns);
    const size_t first_column = (plot.next_column + HISTORY_SIZE - visible_count) % HISTORY_SIZE;

    for (size_t note = 0; note < NOTE_COUNT; ++note) {
        const int midi_note = 40 + static_cast<int>(note);
        char label[16];
        snprintf(label, sizeof(label), "%s%d", pitch_names[midi_note % 12], midi_note / 12 - 1);
        const double y = chart_y + (NOTE_COUNT - 1 - note) * cell_h;
        draw_text(cr, label_w - 7, y + cell_h * 0.78, label, 8, COL_TEXT_DIM, false, true);

        for (size_t column = 0; column < visible_count; ++column) {
            float value = plot.history[(first_column + column) % HISTORY_SIZE][note];
            value = std::max(0.0f, value);
            if (is_confidence)
                value = std::min(1.0f, value);
            else
                value = static_cast<float>(std::min(1.0, std::log1p(value) / std::log(101.0)));

            const double x = chart_x + (plot.visible_columns - visible_count + column) * cell_w;
            set_color(cr, COL_TRACK);
            cairo_rectangle(cr, x, y, cell_w + 0.3, cell_h - 0.7);
            cairo_fill(cr);
            set_color(cr, accent, 0.12 + value * 0.88);
            cairo_rectangle(cr, x, y, cell_w + 0.3, cell_h - 0.7);
            cairo_fill(cr);
        }
    }

    cairo_destroy(cr);
    cairo_surface_flush(plot.backbuffer);
    cairo_t* window_cr = cairo_create(plot.surface);
    cairo_set_source_surface(window_cr, plot.backbuffer, 0, 0);
    cairo_paint(window_cr);
    cairo_destroy(window_cr);
    cairo_surface_flush(plot.surface);
}

static void handle_plot_event(GuitarMidiUI* ui, int plot_index, XEvent* ev)
{
    DebugPlot& plot = ui->plots[plot_index];
    if (ev->type == ConfigureNotify &&
        (plot.width != ev->xconfigure.width || plot.height != ev->xconfigure.height)) {
        plot.width = ev->xconfigure.width;
        plot.height = ev->xconfigure.height;
        cairo_xlib_surface_set_size(plot.surface, plot.width, plot.height);
        cairo_surface_destroy(plot.backbuffer);
        plot.backbuffer = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
            plot.width, plot.height);
        plot.dirty = true;
    } else if (ev->type == Expose && ev->xexpose.count == 0) {
        plot.dirty = true;
    } else if (ev->type == ButtonPress &&
               (ev->xbutton.button == Button4 || ev->xbutton.button == Button5)) {
        const size_t new_zoom = ev->xbutton.button == Button4
            ? std::max(MIN_VISIBLE_COLUMNS, plot.visible_columns / 2)
            : std::min(HISTORY_SIZE, plot.visible_columns * 2);
        if (new_zoom != plot.visible_columns) {
            plot.visible_columns = new_zoom;
            plot.dirty = true;
        }
    } else if (ev->type == ClientMessage &&
               static_cast<Atom>(ev->xclient.data.l[0]) == ui->wm_delete) {
        plot.visible = false;
        XUnmapWindow(ui->dpy, plot.window);
    }
}

// ---------------------------------------------------------------------------
// Interaction helpers
// ---------------------------------------------------------------------------
static int knob_at(GuitarMidiUI* ui, double x, double y)
{
    for (size_t i = 0; i < ui->knobs.size(); ++i) {
        const Knob& k = ui->knobs[i];
        const double dx = x - k.cx;
        const double dy = y - k.cy;
        if (sqrt(dx * dx + dy * dy) <= k.radius + 6)
            return (int)i;
    }
    return -1;
}

static void set_knob_value(GuitarMidiUI* ui, int idx, float v, bool notify)
{
    Knob& k = ui->knobs[idx];
    if (v < k.min) v = k.min;
    if (v > k.max) v = k.max;
    if (v == k.value)
        return;
    k.value = v;
    ui->dirty = true;
    if (notify && ui->write)
        ui->write(ui->controller, k.port, sizeof(float), 0, &k.value);
}

// ---------------------------------------------------------------------------
// X11 event handling
// ---------------------------------------------------------------------------
static void handle_event(GuitarMidiUI* ui, XEvent* ev)
{
    switch (ev->type) {
    case Expose:
        if (ev->xexpose.count == 0)
            ui->dirty = true;
        break;

    case ButtonPress: {
        if (ev->xbutton.button == Button1 && ev->xbutton.y >= 15 && ev->xbutton.y <= 45) {
            const double button_w = 112;
            const double button_gap = 6;
            const double button_start = UI_W - 16 - PLOT_COUNT * button_w - (PLOT_COUNT - 1) * button_gap;
            for (size_t i = 0; i < PLOT_COUNT; ++i) {
                const double x = button_start + i * (button_w + button_gap);
                if (ev->xbutton.x >= x && ev->xbutton.x < x + button_w) {
                    open_plot_window(ui, static_cast<int>(i));
                    break;
                }
            }
            if (ev->xbutton.x >= button_start && ev->xbutton.x < UI_W - 16)
                break;
        }
        const int idx = knob_at(ui, ev->xbutton.x, ev->xbutton.y);
        if (idx < 0)
            break;
        if (ev->xbutton.button == Button1) {
            // Double-click detection via time
            static Time last_click = 0;
            static int  last_idx = -1;
            if (last_idx == idx && ev->xbutton.time - last_click < 350) {
                set_knob_value(ui, idx, ui->knobs[idx].def, true);
                last_click = 0;
                last_idx = -1;
                break;
            }
            last_click = ev->xbutton.time;
            last_idx = idx;

            ui->drag_knob = idx;
            ui->drag_start_y = ev->xbutton.y;
            ui->drag_start_val = ui->knobs[idx].value;
            ui->fine_drag = (ev->xbutton.state & ShiftMask) != 0;
        } else if (ev->xbutton.button == Button4 || ev->xbutton.button == Button5) {
            // Scroll wheel
            const Knob& k = ui->knobs[idx];
            const float range = k.max - k.min;
            float step = range / ((ev->xbutton.state & ShiftMask) ? 200.0f : 40.0f);
            if (ev->xbutton.button == Button5)
                step = -step;
            set_knob_value(ui, idx, k.value + step, true);
        }
        break;
    }

    case ButtonRelease:
        if (ev->xbutton.button == Button1)
            ui->drag_knob = -1;
        break;

    case MotionNotify: {
        // Compress motion events
        while (XCheckTypedWindowEvent(ui->dpy, ui->win, MotionNotify, ev))
            ;
        if (ui->drag_knob >= 0) {
            const Knob& k = ui->knobs[ui->drag_knob];
            const double sensitivity = (ev->xmotion.state & ShiftMask) ? 1000.0 : 200.0;
            const double dy = ui->drag_start_y - ev->xmotion.y;
            const float range = k.max - k.min;
            set_knob_value(ui, ui->drag_knob,
                           ui->drag_start_val + (float)(dy / sensitivity) * range,
                           true);
        } else {
            const int h = knob_at(ui, ev->xmotion.x, ev->xmotion.y);
            if (h != ui->hover_knob) {
                ui->hover_knob = h;
                ui->dirty = true;
            }
        }
        break;
    }

    case LeaveNotify:
        if (ui->hover_knob != -1 && ui->drag_knob < 0) {
            ui->hover_knob = -1;
            ui->dirty = true;
        }
        break;
    }
}

// ---------------------------------------------------------------------------
// LV2UI interface
// ---------------------------------------------------------------------------
static LV2UI_Handle instantiate(const LV2UI_Descriptor*   descriptor,
                                const char*               plugin_uri,
                                const char*               bundle_path,
                                LV2UI_Write_Function      write_function,
                                LV2UI_Controller          controller,
                                LV2UI_Widget*             widget,
                                const LV2_Feature* const* features)
{
    (void)descriptor;
    (void)bundle_path;

    if (strcmp(plugin_uri, GUITARMIDI_URI) != 0)
        return nullptr;

    Window parent = 0;
    for (int i = 0; features[i]; ++i) {
        if (!strcmp(features[i]->URI, LV2_UI__parent))
            parent = (Window)(uintptr_t)features[i]->data;
    }
    if (!parent) {
        fprintf(stderr, "guitarmidi_ui: host does not provide ui:parent\n");
        return nullptr;
    }

    LV2_URID_Map* map = nullptr;
    LV2UI_Port_Map* port_map = nullptr;
    for (int i = 0; features[i]; ++i) {
        if (!strcmp(features[i]->URI, LV2_URID__map))
            map = (LV2_URID_Map*)features[i]->data;
        else if (!strcmp(features[i]->URI, LV2_UI__portMap))
            port_map = (LV2UI_Port_Map*)features[i]->data;
    }
    if (!map || !port_map)
        return nullptr;

    GuitarMidiUI* ui = new GuitarMidiUI();
    ui->write = write_function;
    ui->controller = controller;
    ui->debug_port = port_map->port_index(port_map->handle, "debugdata");
    ui->event_transfer = map->map(map->handle, LV2_ATOM__eventTransfer);
    ui->vector_type = map->map(map->handle, LV2_ATOM__Vector);
    ui->float_type = map->map(map->handle, LV2_ATOM__Float);
    if (ui->debug_port == LV2UI_INVALID_PORT_INDEX) {
        delete ui;
        return nullptr;
    }
    ui->drag_knob = -1;
    ui->hover_knob = -1;
    ui->dirty = true;
    ui->mapped = false;

    ui->dpy = XOpenDisplay(nullptr);
    if (!ui->dpy) {
        delete ui;
        return nullptr;
    }
    ui->screen = DefaultScreen(ui->dpy);
    ui->parent = parent;
    ui->wm_delete = XInternAtom(ui->dpy, "WM_DELETE_WINDOW", False);

    XSetWindowAttributes attr = {};
    attr.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask |
                      PointerMotionMask | LeaveWindowMask | StructureNotifyMask;
    attr.background_pixel = BlackPixel(ui->dpy, ui->screen);

    ui->win = XCreateWindow(ui->dpy, parent, 0, 0, UI_W, UI_H, 0,
                            CopyFromParent, InputOutput, CopyFromParent,
                            CWEventMask | CWBackPixel, &attr);

    // Fixed size hints (many hosts honour these for resize behaviour)
    XSizeHints hints = {};
    hints.flags = PMinSize | PMaxSize | PBaseSize;
    hints.min_width = hints.max_width = hints.base_width = UI_W;
    hints.min_height = hints.max_height = hints.base_height = UI_H;
    XSetWMNormalHints(ui->dpy, ui->win, &hints);

    XMapRaised(ui->dpy, ui->win);
    XFlush(ui->dpy);

    ui->surface = cairo_xlib_surface_create(ui->dpy, ui->win,
                                            DefaultVisual(ui->dpy, ui->screen),
                                            UI_W, UI_H);

    init_knobs(ui);
    layout_knobs(ui);

    *widget = (LV2UI_Widget)(uintptr_t)ui->win;
    return (LV2UI_Handle)ui;
}

static void cleanup(LV2UI_Handle handle)
{
    GuitarMidiUI* ui = (GuitarMidiUI*)handle;
    if (ui->surface)
        cairo_surface_destroy(ui->surface);
    for (auto& plot : ui->plots) {
        if (plot.surface)
            cairo_surface_destroy(plot.surface);
        if (plot.backbuffer)
            cairo_surface_destroy(plot.backbuffer);
        if (plot.window)
            XDestroyWindow(ui->dpy, plot.window);
    }
    if (ui->dpy) {
        XDestroyWindow(ui->dpy, ui->win);
        XCloseDisplay(ui->dpy);
    }
    delete ui;
}

static void port_event(LV2UI_Handle handle,
                       uint32_t     port_index,
                       uint32_t     buffer_size,
                       uint32_t     format,
                       const void*  buffer)
{
    GuitarMidiUI* ui = (GuitarMidiUI*)handle;
    if (port_index == ui->debug_port && format == ui->event_transfer &&
        buffer_size >= sizeof(LV2_Atom_Vector_Body) +
            (NOTE_COUNT * PLOT_COUNT + 1) * sizeof(float)) {
        const auto* atom = (const LV2_Atom*)buffer;
        if (atom->type != ui->vector_type ||
            atom->size < sizeof(LV2_Atom_Vector_Body) +
                (NOTE_COUNT * PLOT_COUNT + 1) * sizeof(float) ||
            atom->size > buffer_size)
            return;
        const auto* vector = (const LV2_Atom_Vector*)atom;
        if (vector->body.child_size != sizeof(float) || vector->body.child_type != ui->float_type)
            return;
        const auto* values = (const float*)((const uint8_t*)vector + sizeof(LV2_Atom_Vector));
        for (size_t plot_index = 0; plot_index < PLOT_COUNT; ++plot_index) {
            DebugPlot& plot = ui->plots[plot_index];
            for (size_t note = 0; note < NOTE_COUNT; ++note)
                plot.history[plot.next_column][note] = values[plot_index * NOTE_COUNT + note];
            plot.next_column = (plot.next_column + 1) % HISTORY_SIZE;
            plot.columns_filled = std::min(HISTORY_SIZE, plot.columns_filled + 1);
            plot.dirty = plot.visible;
        }
        return;
    }
    if (format != 0 || buffer_size != sizeof(float))
        return;

    const float v = *(const float*)buffer;

    for (auto& k : ui->knobs) {
        if (k.port == port_index) {
            if (k.value != v) {
                k.value = v;
                ui->dirty = true;
            }
            return;
        }
    }
}

// ui:idleInterface ----------------------------------------------------------
static int ui_idle(LV2UI_Handle handle)
{
    GuitarMidiUI* ui = (GuitarMidiUI*)handle;

    XEvent ev;
    while (XPending(ui->dpy) > 0) {
        XNextEvent(ui->dpy, &ev);
        if (ev.xany.window == ui->win) {
            handle_event(ui, &ev);
        } else {
            for (size_t i = 0; i < PLOT_COUNT; ++i) {
                if (ev.xany.window == ui->plots[i].window) {
                    handle_plot_event(ui, static_cast<int>(i), &ev);
                    break;
                }
            }
        }
    }

    if (ui->dirty) {
        draw_ui(ui);
        XFlush(ui->dpy);
        ui->dirty = false;
    }
    for (size_t i = 0; i < PLOT_COUNT; ++i) {
        if (ui->plots[i].dirty) {
            draw_debug_plot(ui, static_cast<int>(i));
            XFlush(ui->dpy);
            ui->plots[i].dirty = false;
        }
    }
    return 0;
}

static const LV2UI_Idle_Interface idle_iface = {ui_idle};

static const void* extension_data(const char* uri)
{
    if (!strcmp(uri, LV2_UI__idleInterface))
        return &idle_iface;
    return nullptr;
}

static const LV2UI_Descriptor descriptor = {
    GUITARMIDI_UI_URI,
    instantiate,
    cleanup,
    port_event,
    extension_data
};

LV2_SYMBOL_EXPORT const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index)
{
    return index == 0 ? &descriptor : nullptr;
}
