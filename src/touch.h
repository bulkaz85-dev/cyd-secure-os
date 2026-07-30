#pragma once
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include "board_pins.h"
#include "power.h"

class Touch {
public:
    void begin() {
        touchSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
        ts.begin(touchSPI);
        ts.setRotation(1);
    }

    // Returns true if a touch was read; fills x,y in screen coordinates (0-320,0-240)
    bool read(int16_t &x, int16_t &y) {
        if (!ts.touched()) return false;
        TS_Point p = ts.getPoint();
        x = map(p.x, TOUCH_MIN_X, TOUCH_MAX_X, 0, 320);
        y = map(p.y, TOUCH_MIN_Y, TOUCH_MAX_Y, 0, 240);
        x = constrain(x, 0, 320);
        y = constrain(y, 0, 240);
        power.notifyActivity(); // any real touch resets the idle/sleep timer
        return true;
    }

private:
    SPIClass touchSPI = SPIClass(VSPI);
    XPT2046_Touchscreen ts = XPT2046_Touchscreen(XPT2046_CS, XPT2046_IRQ);
};
