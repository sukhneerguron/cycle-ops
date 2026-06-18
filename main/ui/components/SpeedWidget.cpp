#include "SpeedWidget.hpp"
#include <stdio.h>

namespace ui {

SpeedWidget::SpeedWidget(lv_obj_t* parent) {
    // Container — transparent, fills the screen, flex column centred
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, LV_PCT(100), LV_PCT(100));
    lv_obj_center(container_);
    lv_obj_set_style_bg_opa(container_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_, 4, 0);
    lv_obj_remove_flag(container_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container_,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    // Speed value — large, white
    speed_label_ = lv_label_create(container_);
    lv_label_set_text(speed_label_, "0.0");
    lv_obj_set_style_text_color(speed_label_, lv_color_white(), 0);
    lv_obj_set_style_text_font(speed_label_, &lv_font_montserrat_28, 0);

    // Units — small, grey
    unit_label_ = lv_label_create(container_);
    lv_label_set_text(unit_label_, "km/h");
    lv_obj_set_style_text_color(unit_label_, lv_color_make(0xAA, 0xAA, 0xAA), 0);
    lv_obj_set_style_text_font(unit_label_, &lv_font_montserrat_10, 0);
}

SpeedWidget* SpeedWidget::create(lv_obj_t* parent) {
    // SpeedWidget is allocated once and lives for the lifetime of the screen.
    // LVGL owns the lv_obj_t tree; we own this C++ wrapper.
    return new SpeedWidget(parent);
}

void SpeedWidget::update(const models::RideData& data) {
    // Format — display only, zero business logic
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", data.current_speed_kph);
    lv_label_set_text(speed_label_, buf);
}

} // namespace ui
