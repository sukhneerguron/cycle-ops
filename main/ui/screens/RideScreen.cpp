#include "RideScreen.hpp"
#include "ui/components/SpeedWidget.hpp"
#include "ui/components/CadenceWidget.hpp"
#include "ui/theme/Colors.hpp"
#include "esp_log.h"
#include "gui/ui.h"

namespace ui {

static const char* TAG = "RideScreen";

void RideScreen::create(SpeedWidget& speed, CadenceWidget& cadence, lv_subject_t* ride_subject) {
    ESP_LOGI(TAG, "Creating RideScreen");

    lv_obj_t* parent = ui_Container5;

    // Root row — splits the parent container into two equal columns
    lv_obj_t* row = lv_obj_create(parent);
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
    speed.create(left_cell, ride_subject);
    cadence.create(right_cell, ride_subject);
}

} // namespace ui
