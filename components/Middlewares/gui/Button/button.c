#include "button.h"

static void switch_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        /* 确保布局已计算，坐标有效 */
        lv_obj_update_layout(obj);

        /* 1) switch 当前显示区域（LVGL 逻辑坐标系，已随旋转变化） */
        lv_area_t coords;
        lv_obj_get_coords(obj, &coords);

        /* 2) LVGL indev 实际收到的触摸点（同一逻辑坐标系） */
        lv_point_t pt = {0, 0};
        lv_indev_t * indev = lv_indev_active();
        if(indev) {
            lv_indev_get_point(indev, &pt);
        }

        if(lv_obj_has_state(obj, LV_STATE_CHECKED)) {
            ESP_LOGI("Button", "Switch is ON");
        } else {
            ESP_LOGI("Button", "Switch is OFF");
        }

        ESP_LOGI("Button",
                 "Switch %s | display area x1=%d y1=%d x2=%d y2=%d | "
                 "expected touch in [%d,%d]~[%d,%d] | indev point=(%d,%d)",
                 lv_obj_has_state(obj, LV_STATE_CHECKED) ? "ON" : "OFF",
                 coords.x1, coords.y1, coords.x2, coords.y2,
                 coords.x1, coords.y1, coords.x2, coords.y2,
                 pt.x, pt.y);
    }
}

void button_show(void)
{
    static lv_style_t style_switch_main;
    static lv_style_t style_switch_indicator_checked;
    static lv_style_t style_switch_knob;
    static bool inited = false;

    if(!inited) {
        /* 样式初始化保持不变 */
        lv_style_init(&style_switch_main);
        lv_style_set_bg_color(&style_switch_main, lv_color_hex(0xc4d8cb));
        lv_style_set_bg_opa(&style_switch_main, (255 * 100 / 100));
        lv_style_set_radius(&style_switch_main, 999);
        lv_style_set_pad_all(&style_switch_main, 6);
        lv_style_set_border_width(&style_switch_main, 0);

        lv_style_init(&style_switch_indicator_checked);
        lv_style_set_bg_color(&style_switch_indicator_checked, lv_color_hex(0x22c55e));
        lv_style_set_bg_opa(&style_switch_indicator_checked, (255 * 100 / 100));

        lv_style_init(&style_switch_knob);
        lv_style_set_bg_color(&style_switch_knob, lv_color_hex(0xffffff));
        lv_style_set_bg_opa(&style_switch_knob, (255 * 100 / 100));
        lv_style_set_pad_all(&style_switch_knob, 2);
        lv_style_set_border_color(&style_switch_knob, lv_color_hex(0xd1d5db));
        lv_style_set_border_width(&style_switch_knob, 1);
        lv_style_set_shadow_color(&style_switch_knob, lv_color_hex(0x000000));
        lv_style_set_shadow_opa(&style_switch_knob, (255 * 40 / 100));
        lv_style_set_shadow_width(&style_switch_knob, 16);
        lv_style_set_shadow_offset_y(&style_switch_knob, 2);

        inited = true;
    }

    lv_obj_t * screen = lv_screen_active();

    /* 创建容器，使用Flex布局垂直排列 */
    lv_obj_t * container = lv_obj_create(screen);
    lv_obj_set_size(container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_center(container);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0); /* 容器背景透明 */
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_column(container, 20, 0); /* switch和label之间的间距 */

    /* 创建开关 */
    lv_obj_t *sw_1 = lv_switch_create(container);
    lv_obj_set_size(sw_1, 120, 60);
    lv_obj_add_style(sw_1, &style_switch_main, LV_PART_MAIN);
    lv_obj_add_style(sw_1, &style_switch_indicator_checked, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_style(sw_1, &style_switch_knob, LV_PART_KNOB);
    lv_obj_add_event_cb(sw_1, switch_event_cb, LV_EVENT_ALL, NULL);

    /* 创建标签 */
    lv_obj_t *label = lv_label_create(container);
    lv_label_set_text(label, "click me!");
    lv_obj_set_style_text_color(label, lv_color_black(), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_32, 0);
}
