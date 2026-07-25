#include <gtk/gtk.h>
#include <stdio.h>

#include "types.h"
#include "cpu.h"
#include "loader.h"
#include "memory.h"

typedef struct {
    GtkWidget *window;
    GtkWidget *lbl_r[8];       
    GtkWidget *lbl_flag[4];     
    GtkWidget *lbl_status;
    GtkWidget *terminal_view;
    GtkTextBuffer *terminal_buffer;
    
    PDP11 cpu;
    gboolean loaded;
} AppState;

static AppState app;

static void update_registers(void)
{
    char buf[32];
    static const char *reg_names[] = {
        "R0", "R1", "R2", "R3", "R4", "R5", "SP", "PC"
    };
    
    for (int i = 0; i < 8; i++) {
        snprintf(buf, sizeof(buf), "%s: %06o", 
                 reg_names[i], app.cpu.reg[i]);
        gtk_label_set_text(GTK_LABEL(app.lbl_r[i]), buf);
    }
    
    snprintf(buf, sizeof(buf), "N: %d", (app.cpu.psw >> 3) & 1);
    gtk_label_set_text(GTK_LABEL(app.lbl_flag[0]), buf);
    
    snprintf(buf, sizeof(buf), "Z: %d", (app.cpu.psw >> 2) & 1);
    gtk_label_set_text(GTK_LABEL(app.lbl_flag[1]), buf);
    
    snprintf(buf, sizeof(buf), "V: %d", (app.cpu.psw >> 1) & 1);
    gtk_label_set_text(GTK_LABEL(app.lbl_flag[2]), buf);
    
    snprintf(buf, sizeof(buf), "C: %d", app.cpu.psw & 1);
    gtk_label_set_text(GTK_LABEL(app.lbl_flag[3]), buf);
}

static void on_open_file(GtkButton *button, gpointer data)
{
    (void)button;
    (void)data;
    
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Открыть .lda файл",
        GTK_WINDOW(app.window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Отмена", GTK_RESPONSE_CANCEL,
        "_Открыть", GTK_RESPONSE_ACCEPT,
        NULL);
    
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "PDP-11 LDA файлы");
    gtk_file_filter_add_pattern(filter, "*.lda");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    GtkFileFilter *all_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(all_filter, "Все файлы");
    gtk_file_filter_add_pattern(all_filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), all_filter);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        
        cpu_init(&app.cpu);
        
        int result = loader_load_tape(&app.cpu, filename);
        
        char status[512];
        if (result == 0) {
            snprintf(status, sizeof(status), "Загружен: %s (PC=%06o)", filename, app.cpu.reg[PC]);
            app.loaded = TRUE;
        } else {
            snprintf(status, sizeof(status), "Ошибка загрузки: %s", filename);
            app.loaded = FALSE;
        }
        
        gtk_label_set_text(GTK_LABEL(app.lbl_status), status);
        update_registers();
        
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
}

static void on_step(GtkButton *button, gpointer data)
{
    (void)button;
    (void)data;
    
    if (!app.loaded) {
        gtk_label_set_text(GTK_LABEL(app.lbl_status), "Сначала загрузите файл!");
        return;
    }
    
    cpu_step(&app.cpu);
    update_registers();
    
    if (!app.cpu.running) {
        gtk_label_set_text(GTK_LABEL(app.lbl_status), "Программа завершена");
    }
}

static void on_run(GtkButton *button, gpointer data)
{
    (void)button;
    (void)data;
    
    if (!app.loaded) {
        gtk_label_set_text(GTK_LABEL(app.lbl_status), "Сначала загрузите файл!");
        return;
    }
    
    app.cpu.running = 1;
    int steps = 0;
    while (app.cpu.running && steps < 100000) {
        cpu_step(&app.cpu);
        steps++;
    }
    
    update_registers();
    gtk_label_set_text(GTK_LABEL(app.lbl_status), "Программа выполнена");
}

static void on_reset(GtkButton *button, gpointer data)
{
    (void)button;
    (void)data;
    
    cpu_init(&app.cpu);
    app.loaded = FALSE;
    update_registers();
    gtk_label_set_text(GTK_LABEL(app.lbl_status), "CPU сброшен");
}

static GtkWidget *create_register_panel(void)
{
    GtkWidget *frame = gtk_frame_new("Регистры");
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    
    static const char *reg_names[] = {
        "R0", "R1", "R2", "R3", "R4", "R5", "SP", "PC"
    };
    
    PangoFontDescription *font = pango_font_description_from_string("Monospace 12");
    
    for (int i = 0; i < 8; i++) {
        char text[32];
        snprintf(text, sizeof(text), "%s: 000000", reg_names[i]);
        app.lbl_r[i] = gtk_label_new(text);
        gtk_widget_override_font(app.lbl_r[i], font);
        gtk_widget_set_halign(app.lbl_r[i], GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid), app.lbl_r[i], 0, i, 1, 1);
    }
    
    pango_font_description_free(font);
    
    gtk_container_add(GTK_CONTAINER(frame), grid);
    return frame;
}

static GtkWidget *create_flag_panel(void)
{
    GtkWidget *frame = gtk_frame_new("Флаги");
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(box), 10);
    
    PangoFontDescription *font = pango_font_description_from_string("Monospace 12");
    
    static const char *flag_names[] = {"N", "Z", "V", "C"};
    for (int i = 0; i < 4; i++) {
        char text[16];
        snprintf(text, sizeof(text), "%s: 0", flag_names[i]);
        app.lbl_flag[i] = gtk_label_new(text);
        gtk_widget_override_font(app.lbl_flag[i], font);
        gtk_box_pack_start(GTK_BOX(box), app.lbl_flag[i], TRUE, TRUE, 0);
    }
    
    pango_font_description_free(font);
    
    gtk_container_add(GTK_CONTAINER(frame), box);
    return frame;
}

static GtkWidget *create_button_panel(void) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(box), 5);
    
    GtkWidget *btn_open = gtk_button_new_with_label("Открыть .lda");
    GtkWidget *btn_run = gtk_button_new_with_label("Пуск");
    GtkWidget *btn_step = gtk_button_new_with_label("Шаг");
    GtkWidget *btn_reset = gtk_button_new_with_label("Сброс");
    
    g_signal_connect(btn_open, "clicked", G_CALLBACK(on_open_file), NULL);
    g_signal_connect(btn_run, "clicked", G_CALLBACK(on_run), NULL);
    g_signal_connect(btn_step, "clicked", G_CALLBACK(on_step), NULL);
    g_signal_connect(btn_reset, "clicked", G_CALLBACK(on_reset), NULL);
    
    gtk_box_pack_start(GTK_BOX(box), btn_open, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), btn_run, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), btn_step, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), btn_reset, FALSE, FALSE, 0);
    
    return box;
}

static GtkWidget *create_terminal_panel(void) {
    GtkWidget *frame = gtk_frame_new("Терминал программы");
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    app.terminal_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app.terminal_view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(app.terminal_view), TRUE);
    app.terminal_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app.terminal_view));
    
    gtk_container_add(GTK_CONTAINER(scroll), app.terminal_view);
    gtk_container_add(GTK_CONTAINER(frame), scroll);
    
    gtk_widget_set_size_request(scroll, 400, 200);
    
    return frame;
}

static void create_main_window(void) {
    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.window), "PDP-11 Simulator");
    gtk_window_set_default_size(GTK_WINDOW(app.window), 900, 600);
    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
    gtk_container_add(GTK_CONTAINER(app.window), vbox);
    
    gtk_box_pack_start(GTK_BOX(vbox), create_button_panel(), FALSE, FALSE, 0);
    
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
    
    GtkWidget *left_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(left_vbox), create_register_panel(), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(left_vbox), create_flag_panel(), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), left_vbox, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(hbox), create_terminal_panel(), TRUE, TRUE, 0);
    
    app.lbl_status = gtk_label_new("Готов к работе");
    gtk_widget_set_halign(app.lbl_status, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), app.lbl_status, FALSE, FALSE, 5);
    
    gtk_widget_show_all(app.window);
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);
    
    cpu_init(&app.cpu);
    app.loaded = FALSE;
    
    create_main_window();
    update_registers();
    
    gtk_main();
    
    return 0;
}