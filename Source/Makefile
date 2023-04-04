OS=_Linux

ODir=../

C=g++
Flags =-Wl,-rpath='$$ORIGIN' -Wall -Werror -fPIC -L../ -I../OpenCortex/Include
Libs =-lm -lOpenCortex

Objects=Main.o

%.o: %.cpp
	$(C) $(Flags) -c -o $@ $^

$(ODir)/Server.o: $(Objects)
	cd ../OpenCortex/Source/ &&\
	make &&\
	cp ../libOpenCortex.so ../../libOpenCortex.so\

	$(C) $(Flags) -o $@ $^ $(Libs)
