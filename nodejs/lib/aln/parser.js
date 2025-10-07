const ByteBuffer = require('bytebuffer')
const { Packet } = require('./packet')

const StateBuffering = 0
const StateEscaped = 1
const End = 0xC0
const Esc = 0xDB
const EndT = 0xDC
const EscT = 0xDD

function toAx25Frame (content) {
  const frame = new ByteBuffer(4096)
  for (let i = 0; i < content.length; i++) {
    const b = content.charCodeAt(i)
    if (b === End) {
      frame.append([Esc])
      frame.append([EndT])
    } else if (b === Esc) {
      frame.append([Esc])
      frame.append([EscT])
    } else {
      frame.append([b])
    }
  }
  frame.append([End])
  const offset = frame.offset
  return frame.reset().toBinary(0, offset)
}

class Parser {
  constructor (onPacket) {
    this.state = StateBuffering
    this.buffer = new ByteBuffer(4096)
    this.onPacket = onPacket
  }

  reset () {
    this.state = StateBuffering
    this.buffer.reset()
  }

  acceptPacket () {
    const offset = this.buffer.offset
    const buf = this.buffer.reset().toBinary(0, offset)
    const p = new Packet(buf)
    this.onPacket(p)
    this.reset()
  }

  readAx25FrameBytes (data, length) {
    for (let i = 0; i < length; i++) {
      const b = data.charCodeAt(i)
      if (b === End) {
        this.acceptPacket()
      } else if (this.state === StateEscaped) {
        if (b === EndT) {
          this.buffer.append([End])
        } else if (b === EscT) {
          this.buffer.append([Esc])
        }
        this.state = StateBuffering
      } else if (b === Esc) {
        this.state = StateEscaped
      } else {
        this.buffer.append([b])
      }
    }
  }
}

module.exports.Parser = Parser
module.exports.toAx25Frame = toAx25Frame
