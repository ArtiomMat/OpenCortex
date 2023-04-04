OS=_Linux

IDir=../Include
LDir=./
ODir=../

C=g++
Flags=-Wall -Werror -fPIC -L$(LDir) -I$(IDir)
Libs=-lm -lpng -ljpeg

Objects=Misc.o Fixed.o NeuronsModel$(OS).o NeuronsModel.o Threads$(OS).o TMap2D.o

%.o: %.cpp
	$(C) $(Flags) -c -o $@ $^

$(ODir)/libOctoAI.so: $(Objects)
	$(C) $(Flags) -shared -o $@ $^ $(Libs)
