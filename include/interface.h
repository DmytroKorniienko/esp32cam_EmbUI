#pragma once

void block_menu(Interface *interf, JsonObject *data);
void block_cam(Interface *interf, JsonObject *data);
void block_stream(Interface *interf, JsonObject *data);
void block_cam_settings(Interface *interf, JsonObject *data);

void set_refresh(Interface *interf, JsonObject *data);
void led_toggle(Interface *interf, JsonObject *data);
void set_led_bright(Interface *interf, JsonObject *data);
void set_cam(Interface *interf, JsonObject *data);
void cam_settings(Interface *interf, JsonObject *data);

