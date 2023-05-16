#include "framer.h"

Framer::Framer(void (*out)(uint8)) {
    this->out = out;
}

void Framer::write(uint8 data) {
    if (data == FRAME_END) {
        out(FRAME_ESC);
        out(FRAME_END_T);
    } else if (data == FRAME_ESC) {
        out(FRAME_ESC);
        out(FRAME_ESC_T);
    } else {
        out(data);
    }
}

void Framer::end() {
    out(FRAME_END);
}
