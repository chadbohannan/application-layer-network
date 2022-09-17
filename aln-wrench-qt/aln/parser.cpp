#include "parser.h"
#include "ax25frame.h"

Parser::Parser()
{
    buffer.setBuffer(&bytes);
    buffer.open(QIODevice::Append);
}

void Parser::reset(){
    bytes.clear();
    buffer.reset();
    state = STATE_BUFFERING;
}

void Parser::acceptPacket() {
    emit onPacket(new Packet(bytes));
    reset();
}

void Parser::read(QByteArray data) {
    for( int i = 0; i < data.length(); i++){
        if (data[i] == End){
            acceptPacket();
        } else if (state == STATE_ESCAPED) {
            if (data[i] == EndT) {
                buffer.write(&End);
            }else if (data[i] == EscT) {
                buffer.write(&Esc);
            }
            state = STATE_BUFFERING;
        } else if (data[i] == Esc) {
            state = STATE_ESCAPED;
        } else {
            buffer.write(data.data()+i, 1);
        }
    }
}
