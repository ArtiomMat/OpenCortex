OS=_Linux

IDir=../Include
LDir=./
ODir=../

C=g++
Flags=-Wall -Werror -fPIC -L$(LDir) -I$(IDir)
Libs=-lm

Objects=Misc.o Fixed.o NeuronsModel$(OS).o NeuronsModel.o Threads$(OS).o

%.o: %.cpp
	$(C) $(Flags) -c -o $@ $^

$(ODir)/libOctoAI.so: $(Objects)
	$(C) $(Flags) -shared -o $@ $^ $(Libs)
