import os
r, w = os.pipe()
fr = os.fdopen(r, "rb")
fw = os.fdopen(w, "wb")
fw.write(b"test")
fw.flush()
print(fr.read(4))