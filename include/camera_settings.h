#ifndef CAMERA_SETTINGS_H
#define CAMERA_SETTINGS_H

#include <esp_camera.h>

// Camera settings functions
bool setCameraQuality(int quality) {
    sensor_t * s = esp_camera_sensor_get();
    if (!s) return false;
    
    s->set_quality(s, quality);
    return true;
}

bool setCameraContrast(int contrast) {
    sensor_t * s = esp_camera_sensor_get();
    if (!s) return false;
    
    s->set_contrast(s, contrast);
    return true;
}

bool setCameraBrightness(int brightness) {
    sensor_t * s = esp_camera_sensor_get();
    if (!s) return false;
    
    s->set_brightness(s, brightness);
    return true;
}

bool setCameraSaturation(int saturation) {
    sensor_t * s = esp_camera_sensor_get();
    if (!s) return false;
    
    s->set_saturation(s, saturation);
    return true;
}

bool setCameraFrameSize(framesize_t frameSize) {
    sensor_t * s = esp_camera_sensor_get();
    if (!s) return false;
    
    s->set_framesize(s, frameSize);
    return true;
}

// Helper function to reset camera to default settings
bool resetCameraSettings() {
    sensor_t * s = esp_camera_sensor_get();
    if (!s) return false;
    
    s->set_brightness(s, 0);     // -2 to 2
    s->set_contrast(s, 0);       // -2 to 2
    s->set_saturation(s, 0);     // -2 to 2
    s->set_special_effect(s, 0); // 0 = no effect
    s->set_whitebal(s, 1);       // 1 = enable auto white balance
    s->set_awb_gain(s, 1);       // 1 = enable auto white balance gain
    s->set_wb_mode(s, 0);        // 0 = auto mode
    s->set_exposure_ctrl(s, 1);  // 1 = enable auto exposure
    s->set_aec2(s, 1);           // 1 = enable auto exposure (AEC DSP)
    s->set_gain_ctrl(s, 1);      // 1 = enable auto gain control
    s->set_agc_gain(s, 0);       // 0 = no gain
    s->set_gainceiling(s, (gainceiling_t)0); // 0 = 2x gain
    s->set_bpc(s, 1);            // 1 = enable black pixel correction
    s->set_wpc(s, 1);            // 1 = enable white pixel correction
    s->set_raw_gma(s, 1);        // 1 = enable gamma correction
    s->set_lenc(s, 1);           // 1 = enable lens correction
    s->set_hmirror(s, 0);        // 0 = disable horizontal mirror
    s->set_vflip(s, 0);          // 0 = disable vertical flip
    s->set_dcw(s, 1);            // 1 = enable DCW (downsize)
    s->set_colorbar(s, 0);       // 0 = disable color bar
    
    return true;
}

#endif // CAMERA_SETTINGS_H 