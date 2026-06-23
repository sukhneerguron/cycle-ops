SET(SOURCES ui_HomeScreen.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    fonts/ui_font_materialsmall.c)

add_library(ui ${SOURCES})
