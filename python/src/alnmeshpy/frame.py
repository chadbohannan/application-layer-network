
FRAME_END       = 0XC0
FRAME_ESC       = 0xDB
FRAME_END_T     = 0xDC
FRAME_ESC_T     = 0xDD

# returns a buffer of bytes ready to be sent on a channel
def toAX25FrameBytes(buffer):
    frameBuffer = bytearray()
    for byt in buffer:
        if byt == FRAME_ESC:
            frameBuffer.append(FRAME_ESC)
            frameBuffer.append(FRAME_ESC_T)
        elif byt == FRAME_END:
            frameBuffer.append(FRAME_END)
            frameBuffer.append(FRAME_END_T)
        else:
            try:
                x = byt.to_bytes(1,'big')
            except Exception as e:
                print(e)
            frameBuffer.append(x[0])
    frameBuffer.append(FRAME_END)
    return frameBuffer