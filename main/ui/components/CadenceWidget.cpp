#include "CadenceWidget.hpp"
#include <stdio.h>

namespace ui {

CadenceWidget::CadenceWidget(lv_obj_t* parent) {
    // Container — transparent, fills its allocated cell, flex column centred
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

    // Cadence value — large, white
    cadence_label_ = lv_label_create(container_);
    lv_label_set_text(cadence_label_, "0");
    lv_obj_set_style_text_color(cadence_label_, lv_color_white(), 0);
    lv_obj_set_style_text_font(cadence_label_, &lv_font_montserrat_28, 0);

    // Units — small, grey
    unit_label_ = lv_label_create(container_);
    lv_label_set_text(unit_label_, "rpm");
    lv_obj_set_style_text_color(unit_label_, lv_color_make(0xAA, 0xAA, 0xAA), 0);
    lv_obj_set_style_text_font(unit_label_, &lv_font_montserrat_10, 0);
}

CadenceWidget* CadenceWidget::create(lv_obj_t* parent) {
    // CadenceWidget is allocated once and lives for the lifetime of the screen.
    // LVGL owns the lv_obj_t tree; we own this C++ wrapper.
    return new CadenceWidget(parent);
}

void CadenceWidget::update(const models::RideData& data) {
    // Format — display only, zero business logic
    char buf[8];
    snprintf(buf, sizeof(buf), "%u", data.current_cadence);
    lv_label_set_text(cadence_label_, buf);
}

} // namespace ui
