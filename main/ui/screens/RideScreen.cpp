#include "RideScreen.hpp"
#include "ui/components/SpeedWidget.hpp"
#include "ui/components/CadenceWidget.hpp"
#include "esp_log.h"

namespace ui {

static const char* TAG = "RideScreen";

RideWidgets RideScreen::create() {
    ESP_LOGI(TAG, "Creating RideScreen");

    lv_obj_t* screen = lv_obj_create(nullptr);

    // Black background to match the OLED
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    // No padding or border on the screen itself — full 128×64 canvas
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_style_border_width(screen, 0, 0);

    // Root row — splits the screen into two equal columns
    lv_obj_t* row = lv_obj_create(screen);
    lv_obj_set_size(row, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_pad_column(row, 0, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row,
                          LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    // Left cell — SpeedWidget
    lv_obj_t* left_cell = lv_obj_create(row);
    lv_obj_set_size(left_cell, LV_PCT(50), LV_PCT(100));
    lv_obj_set_style_bg_opa(left_cell, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(left_cell, 0, 0);
    lv_obj_set_style_pad_all(left_cell, 0, 0);
    lv_obj_remove_flag(left_cell, LV_OBJ_FLAG_SCROLLABLE);

    // Right cell — CadenceWidget
    lv_obj_t* right_cell = lv_obj_create(row);
    lv_obj_set_size(right_cell, LV_PCT(50), LV_PCT(100));
    lv_obj_set_style_bg_opa(right_cell, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(right_cell, 0, 0);
    lv_obj_set_style_pad_all(right_cell, 0, 0);
    lv_obj_remove_flag(right_cell, LV_OBJ_FLAG_SCROLLABLE);

    // Build widgets inside their respective cells
    SpeedWidget*   speed_widget   = SpeedWidget::create(left_cell);
    CadenceWidget* cadence_widget = CadenceWidget::create(right_cell);

    lv_screen_load(screen);

    return RideWidgets{speed_widget, cadence_widget};
}

} // namespace ui
