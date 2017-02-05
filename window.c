#include "GPX_parse.h"
#include "analysis.h"
#include "parse_double.h"
#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

static GtkWidget *window, *grid, *trackPicture, *openButton, *textBox,
       *labelDivision, *division, *labelValue, *value, *labelUnit,
       *refresh, *speedGraph, *labelSpeedGraph, *altitudeGraph, *labelAltitudeGraph,
       *labelSampleSize, *sampleSize, *editDivision, *drawNumbers, *labelSignature, *labelEditDivision;
static GtkTextBuffer *buffer;
static int nPoints, nDivisions = 0;
static GPX_entity *points = NULL;
static bool *divisionPoints = NULL, divisionPointsEdited = false;

static double maxX, maxY, minX, minY, cosOfLat;

void clearTextBox()
{
    gtk_text_buffer_set_text(buffer, "", 0);
}

void appendTextBox(char *text)
{
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(buffer, &iter);

    gtk_text_buffer_insert(GTK_TEXT_BUFFER(buffer), &iter, text, -1);
}

static void division_method_changed(GtkWidget *widget, gpointer data)
{
    const char *selectedItem;
    selectedItem = gtk_combo_box_get_active_id(GTK_COMBO_BOX(widget));

    gtk_range_set_value(GTK_RANGE(sampleSize), 1);

    if(!selectedItem || strcmp(selectedItem, "None") == 0)
    {
        gtk_widget_hide(labelSampleSize);
        gtk_widget_hide(sampleSize);
        gtk_label_set_text(GTK_LABEL(labelUnit), "");
    }
    if(selectedItem && strcmp(selectedItem, "Speed")  == 0)
    {
        gtk_widget_show(labelSampleSize);
        gtk_widget_show(sampleSize);
        gtk_label_set_text(GTK_LABEL(labelUnit), "km/h");
    }
    if(selectedItem && strcmp(selectedItem, "Time")  == 0)
    {
        gtk_widget_hide(labelSampleSize);
        gtk_widget_hide(sampleSize);
        gtk_label_set_text(GTK_LABEL(labelUnit), "min");
    }
    if(selectedItem && strcmp(selectedItem, "Distance")  == 0)
    {
        gtk_widget_hide(labelSampleSize);
        gtk_widget_hide(sampleSize);
        gtk_label_set_text(GTK_LABEL(labelUnit), "km");
    }
    if(selectedItem && strcmp(selectedItem, "Pace")  == 0)
    {
        gtk_widget_show(labelSampleSize);
        gtk_widget_show(sampleSize);
        gtk_label_set_text(GTK_LABEL(labelUnit), "min/km");
    }
    if(selectedItem && strcmp(selectedItem, "Angle")  == 0)
    {
        gtk_widget_show(labelSampleSize);
        gtk_widget_show(sampleSize);
        gtk_label_set_text(GTK_LABEL(labelUnit), "deg");
    }
}

void refresh_click_callback(GtkWidget *widget, gpointer user_data)
{
    if(!nPoints)
        return;

    //reading division value
    double divisionValue = 0;

    const char *unparsedText = NULL;

    unparsedText = gtk_entry_get_text(GTK_ENTRY(value));

    if(unparsedText && isDouble(unparsedText))
        divisionValue = parseDouble(unparsedText);

    //reading division value-end

    //setting division points

    if(divisionPoints != NULL)
        free(divisionPoints);

    const char *selectedItem;
    selectedItem = gtk_combo_box_get_active_id(GTK_COMBO_BOX(division));

    divisionPoints = calloc(nPoints , sizeof(bool));

    divisionPoints[nPoints - 1] = true;
    nDivisions = 0;
    int sample = (int)gtk_range_get_value(GTK_RANGE(sampleSize));

    if(divisionValue > 0 && selectedItem && strcmp(selectedItem, "Speed")  == 0)
        nDivisions = divideBySpeedOrPace(SPEED, nPoints, points, divisionPoints, divisionValue, sample);
    if(divisionValue > 0 && selectedItem && strcmp(selectedItem, "Time")  == 0)
        nDivisions = divideByTime(nPoints, points, divisionPoints, divisionValue);
    if(divisionValue > 0 && selectedItem && strcmp(selectedItem, "Distance")  == 0)
        nDivisions = divideByDistance(nPoints, points, divisionPoints, divisionValue);
    if(divisionValue > 0 && selectedItem && strcmp(selectedItem, "Pace")  == 0)
        nDivisions = divideBySpeedOrPace(PACE, nPoints, points, divisionPoints, divisionValue, sample);
    if(divisionValue > 0 && selectedItem && strcmp(selectedItem, "Angle")  == 0)
        nDivisions = divideByAngle(nPoints, points, divisionPoints, divisionValue / 180 * 3.141592, sample);

    //setting division points-end

    gtk_button_set_label(GTK_BUTTON(editDivision), "Edit division points");
    divisionPointsEdited = false;

    gtk_widget_queue_draw(trackPicture);
    gtk_widget_queue_draw(speedGraph);
    gtk_widget_queue_draw(altitudeGraph);
    analyze(nPoints, points, divisionPoints, nDivisions);
}

static void openButton_click(GtkWidget *widget, gpointer user_data)
{
    //opening file  -------  based on https://developer.gnome.org/gtk3/3.22/GtkFileChooserDialog.html
    GtkWidget *dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;

    dialog = gtk_file_chooser_dialog_new ("Open File", GTK_WINDOW(window), action,
                                          "_Cancel", GTK_RESPONSE_CANCEL,
                                          "_Open", GTK_RESPONSE_ACCEPT, NULL);

    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT)
    {
        char *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        filename = gtk_file_chooser_get_filename(chooser);

        if(points != NULL)
            free(points);

        points = GPX_read(filename, &nPoints);

        gtk_combo_box_set_active_id(GTK_COMBO_BOX(division), "None");
        gtk_entry_set_text(GTK_ENTRY(value), "");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(drawNumbers), TRUE);
    }

    gtk_widget_destroy(dialog);

    if(!nPoints && res == GTK_RESPONSE_ACCEPT)
    {
        clearTextBox();

        gtk_widget_queue_draw(trackPicture);
        gtk_widget_queue_draw(speedGraph);
        gtk_widget_queue_draw(altitudeGraph);

        GtkWidget *dialog;
        dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_ERROR,GTK_BUTTONS_CLOSE, "%s",
                                        "Error while opening the file. Make sure that you select a GPX file and that there are only english alphabet characters in the filename.\n");
        gtk_dialog_run(GTK_DIALOG (dialog));
        gtk_widget_destroy(dialog);
    }

    if(!nPoints || res != GTK_RESPONSE_ACCEPT)
        return;
    //opening file-end

    //finding min, max coordinates and avg latitude

    maxX = minX = points[0].lon;
    minY = maxY = points[0].lat;

    double avgLat = points[0].lat;

    for(int i = 1; i < nPoints; i++)
    {
        if(points[i].lon > maxX)
            maxX = points[i].lon;
        if(points[i].lon < minX)
            minX = points[i].lon;
        if(points[i].lat > maxY)
            maxY = points[i].lat;
        if(points[i].lat < minY)
            minY = points[i].lat;

        avgLat += points[i].lat;

    }

    avgLat /= nPoints;

    cosOfLat = cos(avgLat / 180.0 * 3.141592);

    //finding min, max coordinates and avg latitude-end

    refresh_click_callback(NULL, NULL);
}

static gboolean speedGraph_draw_callback(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    if(!nPoints || points == NULL)
        return FALSE;

    //setting cairo

    cairo_set_line_width (cr, 0.5);

    GdkRGBA color;

    gdk_rgba_parse(&color, "rgb(000,000,255)");
    gdk_cairo_set_source_rgba (cr, &color);

    //setting cairo-end

    double x, y, maxSpeed = 0, minSpeed = 100000, v, scaleY, separation;

    guint width, height;

    width = gtk_widget_get_allocated_width (widget);
    height = gtk_widget_get_allocated_height (widget);


    for(int i = 1; i < nPoints; i++)
    {
        if(!points[i - 1].lastInSegment && points[i].time - points[i - 1].time)
        {
            double scale = cos((points[i].lat + points[i - 1].lat) / 2 / 180.0 * 3.141592);
            x = (points[i].lon - points[i - 1].lon) * 40075 * scale / 360;
            y = (points[i].lat - points[i - 1].lat) * 40030 / 360;

            v = sqrt(x * x + y * y) / (points[i].time - points[i - 1].time) * 3600;

            maxSpeed = v > maxSpeed ? v : maxSpeed;
            minSpeed = v < minSpeed ? v : minSpeed;
        }
    }

    separation = (double) width / nPoints;

    if(maxSpeed - minSpeed != 0)
        scaleY = height / (maxSpeed - minSpeed);
    else if (maxSpeed > 0)
        scaleY = height / (maxSpeed * 2);
    else
        scaleY = height / 1;


    for(int i = 1; i < nPoints; i++)
    {
        if(!points[i - 1].lastInSegment && points[i].time - points[i - 1].time)
        {
            double scale = cos((points[i].lat + points[i - 1].lat) / 2 / 180.0 * 3.141592);
            x = (points[i].lon - points[i - 1].lon) * 40075 * scale / 360;
            y = (points[i].lat - points[i - 1].lat) * 40030 / 360;

            v = sqrt(x * x + y * y) / (points[i].time - points[i - 1].time) * 3600;
        }
        if(i > 1)
        {
            cairo_line_to(cr, i * separation * 0.9 + 0.05 * width, height - (v - minSpeed) * scaleY * 0.9 - 0.05 * height);
            cairo_stroke(cr);
        }

        cairo_move_to(cr, (i * separation) * 0.9 + 0.05 * width, height - (v - minSpeed) * scaleY * 0.9 - 0.05 * height);
    }

    gdk_rgba_parse(&color, "rgb(255,000,000)");
    gdk_cairo_set_source_rgba (cr, &color);

    for(int i = 1; i < nPoints - 1; i++)
    {
        if(divisionPoints[i])
        {
            cairo_move_to(cr, i * separation * 0.9 + 0.05 * width, height * 0.95);
            cairo_line_to(cr, i * separation * 0.9 + 0.05 * width, height * 0.05);
            cairo_stroke(cr);
        }
    }

    cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);

    cairo_select_font_face(cr, "Arial", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10);

    char line[100];

    sprintf(line, "Max speed: %.3f km/h", maxSpeed);
    cairo_move_to(cr, 0.05 * width, 0.04 * height);
    cairo_show_text(cr, line);

    sprintf(line, "Min speed: %.3f km/h", minSpeed);
    cairo_move_to(cr, 0.05 * width, height);
    cairo_show_text(cr, line);


    return FALSE;
}

static gboolean altitudeGraph_draw_callback(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    if(!nPoints || points == NULL)
        return FALSE;

    //setting cairo

    cairo_set_line_width (cr, 0.5);

    GdkRGBA color;

    gdk_rgba_parse(&color, "rgb(000,000,255)");
    gdk_cairo_set_source_rgba (cr, &color);

    //setting cairo-end

    double maxAltitude = -100000, minAltitude = 100000, scaleY, separation;

    guint width, height;

    width = gtk_widget_get_allocated_width(widget);
    height = gtk_widget_get_allocated_height(widget);

    for(int i = 0; i < nPoints; i++)
    {
        maxAltitude = points[i].ele > maxAltitude ? points[i].ele : maxAltitude;
        minAltitude = points[i].ele < minAltitude ? points[i].ele : minAltitude;
    }

    separation = (double) width / nPoints;

    if(maxAltitude - minAltitude != 0)
        scaleY = height / (maxAltitude - minAltitude);
    else if (maxAltitude > 0)
        scaleY = height / (maxAltitude * 2);
    else
        scaleY = height / 1;


    for(int i = 0; i < nPoints; i++)
    {
        if(i > 0)
        {
            cairo_line_to(cr, i * separation * 0.9 + 0.05 * width, height - (points[i].ele - minAltitude) * scaleY * 0.9 - 0.05 * height);
            cairo_stroke(cr);
        }

        cairo_move_to(cr, i * separation * 0.9 + 0.05 * width, height - (points[i].ele - minAltitude) * scaleY * 0.9 - 0.05 * height);
    }


    gdk_rgba_parse(&color, "rgb(255,000,000)");
    gdk_cairo_set_source_rgba (cr, &color);

    for(int i = 1; i < nPoints - 1; i++)
    {
        if(divisionPoints[i])
        {
            cairo_move_to(cr, i * separation * 0.9 + 0.05 * width, height * 0.95);
            cairo_line_to(cr, i * separation * 0.9 + 0.05 * width, height * 0.05);
            cairo_stroke(cr);
        }
    }

    cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);

    cairo_select_font_face(cr, "Arial", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10);

    char line[100];

    sprintf(line, "Max altitude: %.3f m", maxAltitude);
    cairo_move_to(cr, 0.05 * width, 0.04 * height);
    cairo_show_text(cr, line);

    sprintf(line, "Min altitude: %.3f m", minAltitude);
    cairo_move_to(cr, 0.05 * width, height);
    cairo_show_text(cr, line);


    return FALSE;
}

static gboolean trackPicture_draw_callback(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    if(!nPoints)
        return FALSE;

    double scale;
    guint width, height;
    GdkRGBA color;
    bool blue = false;

    //calculating coefficients

    width = gtk_widget_get_allocated_width(widget);
    height = gtk_widget_get_allocated_height(widget);

    int shorterSide = width < height ? width : height;

    if(maxY - minY > (maxX - minX) * cosOfLat)
        scale = shorterSide / (maxY - minY) * 0.9;
    else
        scale = shorterSide / (maxX - minX) / cosOfLat * 0.9;

    double vx = (width - ((maxX - minX) * scale * cosOfLat )) / 2.0;
    double vy = (height - ((maxY - minY) * scale)) / 2.0;

    //calculating coefficients-end

    //setting cairo

    cairo_set_line_width (cr, 2);

    gdk_rgba_parse(&color, "rgb(255,201,015)");
    gdk_cairo_set_source_rgba (cr, &color);

    //setting cairo-end

    //drawing path

    cairo_move_to(cr, (points[0].lon - minX) * scale * cosOfLat + vx, height - (points[0].lat - minY) * scale - vy);

    for(int i = 1; i < nPoints; i++)
    {
        if(!points[i - 1].lastInSegment)
        {
            cairo_line_to(cr, (points[i].lon - minX) * scale * cosOfLat + vx, height - (points[i].lat - minY) * scale - vy);
            cairo_stroke(cr);
        }

        cairo_move_to(cr, (points[i].lon - minX) * scale * cosOfLat + vx, height - (points[i].lat - minY) * scale - vy);

        if(divisionPoints[i] == 1)
        {
            if(!blue)
                gdk_rgba_parse(&color, "rgb(000,162,232)");
            else
                gdk_rgba_parse(&color, "rgb(255,201,015)");

            blue = !blue;
            gdk_cairo_set_source_rgba (cr, &color);
        }
    }

    //drawing path-end

    //marking start and end

    gdk_rgba_parse(&color, "rgb(000,255,000)");
    gdk_cairo_set_source_rgba (cr, &color);

    cairo_arc (cr, (points[0].lon - minX) * scale * cosOfLat + vx, height - (points[0].lat - minY) * scale - vy, 5, 0, 2 * G_PI);
    cairo_fill(cr);

    gdk_rgba_parse(&color, "rgb(255,000,000)");
    gdk_cairo_set_source_rgba (cr, &color);

    cairo_arc (cr, (points[nPoints - 1].lon - minX) * scale * cosOfLat + vx, height - (points[nPoints - 1].lat - minY) * scale - vy, 5, 0, 2 * G_PI);
    cairo_fill(cr);

    //marking start and end-end

    //drawing segment numbers

    cairo_select_font_face(cr, "Arial", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 10);

    if(nDivisions && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(drawNumbers)))
    {
        gdk_rgba_parse(&color, "rgb(000,000,000)");
        gdk_cairo_set_source_rgba (cr, &color);

        char num[32];
        int seg_num = 1;
        for(int i = 0; i < nPoints - 1; i++)
            if(divisionPoints[i] == 1 || i == 0)
            {
                cairo_move_to(cr, (points[i].lon - minX) * scale * cosOfLat + vx, height - (points[i].lat - minY) * scale - vy + height / 50.0);
                sprintf(num, "%d", seg_num);
                cairo_show_text(cr, num);
                seg_num++;
            }
    }
    //drawing segment numbers-end

    return FALSE;
}

static gboolean graph_click(GtkWidget *widget, GdkEventButton  *event, gpointer user_data)
{
    guint width;
    width = gtk_widget_get_allocated_width(widget);

    double pos = (event->x - width * 0.05);

    if(nPoints && divisionPointsEdited && pos > 0 && pos < width * 0.9)
    {
        int start = (int)round(pos / (width * 0.9) * nPoints);

        for(int i = 0; i < nPoints / 150 + 1 && start - i > 0 && start + i < nPoints - 1; i++)  //searching for division point to delete
        {
            if(divisionPoints[start + i] == true)
            {
                divisionPoints[start + i] = false;
                gtk_widget_queue_draw(speedGraph);
                gtk_widget_queue_draw(altitudeGraph);
                nDivisions--;
                return FALSE;
            }
            if(divisionPoints[start - i] == true)
            {
                divisionPoints[start - i] = false;
                gtk_widget_queue_draw(speedGraph);
                gtk_widget_queue_draw(altitudeGraph);
                nDivisions--;
                return FALSE;
            }
        }

        if(start > 0 && start < nPoints - 1)
        {
            divisionPoints[start] = true;   //adding division point
            gtk_widget_queue_draw(speedGraph);
            gtk_widget_queue_draw(altitudeGraph);
            nDivisions++;
        }
    }

    return FALSE;
}

void editDivision_click(GtkWidget *widget, gpointer user_data)
{
    if(!divisionPointsEdited)   //starting edition
        gtk_button_set_label(GTK_BUTTON(widget), "Done");
    else    //updating graph and analysis
    {
        gtk_button_set_label(GTK_BUTTON(widget), "Edit division points");
        analyze(nPoints, points, divisionPoints, nDivisions); //tu się wypierdziela
        gtk_widget_queue_draw(trackPicture);
    }

    divisionPointsEdited = !divisionPointsEdited;
    gtk_widget_set_sensitive(refresh, !divisionPointsEdited);
    gtk_widget_set_sensitive(division, !divisionPointsEdited);
    gtk_widget_set_sensitive(value, !divisionPointsEdited);
    gtk_widget_set_sensitive(openButton, !divisionPointsEdited);
    gtk_widget_set_sensitive(sampleSize, !divisionPointsEdited);
    gtk_widget_set_visible(labelEditDivision, divisionPointsEdited);
}

static void destructor()
{
    if(points != NULL)
        free(points);
    if(divisionPoints != NULL)
        free(divisionPoints);
}

int createWindow()
{
    points = NULL;
    divisionPoints = NULL;

    //Window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(destructor), NULL);

    gtk_window_set_title(GTK_WINDOW(window), "Nomad - GPX analyzer");
    gtk_container_set_border_width(GTK_CONTAINER(window),  10);
    //Window-end

    //grid
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_row_homogeneous(GTK_GRID(grid), TRUE);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);

    gtk_container_add(GTK_CONTAINER(window), grid);
    //grid-end

    //trackPicture
    trackPicture = gtk_drawing_area_new();
    gtk_grid_attach(GTK_GRID(grid), trackPicture, 0, 0, 8, 8);
    g_signal_connect(trackPicture, "draw", G_CALLBACK(trackPicture_draw_callback), NULL);

    gtk_widget_set_size_request(trackPicture, 400, 400);
    //trackPicture-end

    //labelSpeedGraph
    labelSpeedGraph = gtk_label_new("Average speed in consecutive measurement points:");
    gtk_grid_attach(GTK_GRID(grid), labelSpeedGraph, 0, 8, 8, 1);
    //labelSpeedGraph-end

    //speedGraph
    speedGraph = gtk_drawing_area_new();
    gtk_grid_attach(GTK_GRID(grid), speedGraph, 0, 9, 8, 4);
    g_signal_connect(speedGraph, "draw", G_CALLBACK(speedGraph_draw_callback), NULL);

    g_signal_connect(speedGraph, "button-press-event", G_CALLBACK(graph_click), "speed");
    gtk_widget_set_events(speedGraph, GDK_BUTTON_PRESS_MASK);
    //speedGraph-end

    //labelAltitudeGraph
    labelAltitudeGraph = gtk_label_new("Altitude in consecutive measurement points:");
    gtk_grid_attach(GTK_GRID(grid), labelAltitudeGraph, 8, 8, 8, 1);
    //labelAltitudeGraph-end

    //altitudeGraph
    altitudeGraph = gtk_drawing_area_new();
    gtk_grid_attach(GTK_GRID(grid), altitudeGraph, 8, 9, 8, 4);
    g_signal_connect(altitudeGraph, "draw", G_CALLBACK(altitudeGraph_draw_callback), NULL);

    g_signal_connect(altitudeGraph, "button-press-event", G_CALLBACK(graph_click), "altitude");
    gtk_widget_set_events(altitudeGraph, GDK_BUTTON_PRESS_MASK);
    //altitudeGraph-end

    //textBox
    buffer = gtk_text_buffer_new(NULL);

    textBox = gtk_text_view_new_with_buffer(buffer);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textBox), FALSE);

    gtk_widget_set_size_request(textBox, 400, 400);
    //textBox-end

    //scrolledWindow
    GtkWidget *scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolledWindow), textBox);

    gtk_grid_attach(GTK_GRID(grid), scrolledWindow, 8, 0, 8, 8);

    //scrolledWindow-end

    //openButton
    openButton = gtk_button_new_with_label("Open track");
    g_signal_connect(openButton, "clicked", G_CALLBACK(openButton_click), NULL);
    gtk_grid_attach(GTK_GRID(grid), openButton, 16, 0, 4, 1);
    //openButton-end

    //refresh
    refresh = gtk_button_new_with_label("Refresh");
    gtk_grid_attach(GTK_GRID(grid), refresh, 16, 1, 4, 1);

    g_signal_connect(refresh, "clicked", G_CALLBACK(refresh_click_callback), NULL);
    //refresh-end

    //labelDivision
    labelDivision = gtk_label_new("Division");
    gtk_grid_attach(GTK_GRID(grid), labelDivision, 16, 2, 4, 1);
    //labelDivision

    //division
    division = gtk_combo_box_text_new();

    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(division), "None", "None");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(division), "Speed", "Speed change");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(division), "Time", "Time");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(division), "Distance", "Distance");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(division), "Pace", "Pace change");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(division), "Angle", "Angle change");

    g_signal_connect(GTK_COMBO_BOX(division), "changed", G_CALLBACK(division_method_changed), NULL);
    gtk_grid_attach(GTK_GRID(grid), division, 16, 3, 4, 1);
    //division-end

    //labelValue
    labelValue = gtk_label_new("Division value");
    gtk_grid_attach(GTK_GRID(grid), labelValue, 16, 4, 4, 1);
    //labelValue-end

    //value
    value = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), value, 16, 5, 3, 1);
    //value-end

    //labelUnit
    labelUnit = gtk_label_new("");
    gtk_grid_attach(GTK_GRID(grid), labelUnit, 19, 5, 1, 1);
    //labelUnit-end

    //drawNumbers
    drawNumbers = gtk_check_button_new_with_label("Draw segment numbers");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(drawNumbers), TRUE);

    gtk_grid_attach(GTK_GRID(grid), drawNumbers, 16, 8, 4, 1);
    //drawNumbers-end

    //editDivision
    editDivision = gtk_button_new_with_label("Edit division points");
    gtk_grid_attach(GTK_GRID(grid), editDivision, 16, 9, 4, 1);
    g_signal_connect(editDivision, "clicked", G_CALLBACK(editDivision_click), NULL);
    //editDivision-end

    //labelSignature
    labelSignature = gtk_label_new("Created by Konrad Werblinski");
    //gtk_label_se

    gtk_grid_attach(GTK_GRID(grid), labelSignature, 16, 12, 4, 1);
    //labelSignature-end

    gtk_widget_show_all(window);

    //labelSampleSize
    labelSampleSize = gtk_label_new("Sample size");
    gtk_grid_attach(GTK_GRID(grid), labelSampleSize, 16, 6, 4, 1);
    //labelSampleSize-end

    //sampleSize
    sampleSize = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1, 20, 1);
    gtk_grid_attach(GTK_GRID(grid), sampleSize, 16, 7, 4, 1);
    //sampleSize-end

    //labelEditDivision

    labelEditDivision = gtk_label_new("Click on the graphs");
    gtk_grid_attach(GTK_GRID(grid), labelEditDivision, 16, 10, 4, 1);

    //labelEditDivision-end

    gtk_main();

    return 0;
}
