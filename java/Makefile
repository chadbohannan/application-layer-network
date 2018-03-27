JFLAGS = -g
JC = javac
.SUFFIXES: .java .class
.java.class:
		$(JC) $(JFLAGS) $*.java

CLASSES = \
		Packet.java \
		Parser.java \
		Example.java 

default: classes

classes: $(CLASSES:.java=.class)

clean:
		$(RM) *.class