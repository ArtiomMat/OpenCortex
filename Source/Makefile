OS=_Linux

ODir=../

C=g++
Flags =-Wl,-rpath='$$ORIGIN' -Wall -Werror -fPIC -L../ -I../OctoAI/Include
Libs =-lm -lOctoAI

Objects=Main.o

%.o: %.cpp
	$(C) $(Flags) -c -o $@ $^

$(ODir)/Server.o: $(Objects)
	cd ../OctoAI/Source/ &&\
	make &&\
	cp ../libOctoAI.so ../../libOctoAI.so\

	$(C) $(Flags) -o $@ $^ $(Libs)
	$(C) -S $(Flags) -o WHAT Main.cpp
