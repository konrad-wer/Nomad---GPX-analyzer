#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "window.h"

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);
    createWindow();
}
