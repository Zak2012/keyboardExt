CC = x86_64-w64-mingw32-g++
CC32 = i686-w64-mingw32-g++
STD = c++2a
APP_NAME = keyboardExt

# available var {CC,CC32,INC,LIB,SRC,LIB,STD,APP_NAME}

INC	= ./include
LIBPATH = ./lib
SRC = ./src/*.cpp
LIB = -lUser32 -lShell32 -lKernel32
CMDCOM =-Wall -std=$(STD) $(SRC) $(APP_NAME).res -I$(INC) -L$(LIBPATH) $(LIB) -o $(APP_NAME)

res :
	windres ./src/$(APP_NAME).rc -O coff -o $(APP_NAME).res

debug : res
	ccache $(CC) -ggdb3 $(CMDCOM).dbg.exe

debug32 : res
	ccache $(CC32) -ggdb3 $(CMDCOM)32.dbg.exe

release : res
	ccache $(CC) -mwindows $(CMDCOM).exe

release32 : res
	ccache $(CC32) $(CMDCOM)32.exe

target : dependencies
	action

