OS=_Linux

IDir=../Include
LDir=./
ODir=../

C=g++
Flags=-Wall -Werror -fPIC -L$(LDir) -I$(IDir)
Libs=-lm -lpng -ljpeg

Objects=Misc.o TF8.o Threads$(OS).o TMap2D.o TNeuralModel$(OS).o TNeuralModel.o

%.o: %.cpp
	$(C) $(Flags) -c -o $@ $^

$(ODir)/libOpenCortex.so: $(Objects)
	$(C) $(Flags) -shared -o $@ $^ $(Libs)
