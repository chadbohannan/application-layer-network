#ifndef FRAME_H_
#define FRAME_H_

#include "alntypes.h"

class Framer {
public:   
    Framer(void (*out)(uint8));
    void write(uint8 data);
    void end();

    void (*out)(uint8);
};

#endif
