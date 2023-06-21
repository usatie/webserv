# makefile
# Created by IBM WorkFrame/2 MakeMake at 20:56:45 on 15 July 1996
#
# The actions included in this make file are:
#  Compile::C++ Compiler
#  Link::Linker

.SUFFIXES: .cpp .obj 

.all: \
    .\3wd.exe

.cpp.obj:
    @echo " Compile::C++ Compiler "
    icc.exe /Q /Fi /Si /O /Gm /G4 /Gf /Gi /Gx /C %s

{h:\book\server0.9b}.cpp.obj:
    @echo " Compile::C++ Compiler "
    icc.exe /Q /Fi /Si /O /Gm /G4 /Gf /Gi /Gx /C %s

.\3wd.exe: \
    .\scodes.obj \
    .\socket.obj \
    .\3wd.obj \
    .\config.obj \
    .\index.obj \
    {$(LIB)}so32dll.lib \
    {$(LIB)}tcp32dll.lib
    @echo " Link::Linker "
    icc.exe @<<
     /B" /exepack:2 /packd /align:1 /nologo"
     /Fe3wd.exe 
     so32dll.lib 
     tcp32dll.lib 
     .\scodes.obj
     .\socket.obj
     .\3wd.obj
     .\config.obj
     .\index.obj
<<

.\scodes.obj: \
    h:\book\server0.9b\scodes.cpp

.\index.obj: \
    h:\book\server0.9b\index.cpp \
    {h:\book\server0.9b;$(INCLUDE);}socket.hpp \
    {h:\book\server0.9b;$(INCLUDE);}defines.hpp \
    {h:\book\server0.9b;$(INCLUDE);}3wd.hpp \
    {h:\book\server0.9b;$(INCLUDE);}config.hpp \
    {h:\book\server0.9b;$(INCLUDE);}scodes.hpp

.\config.obj: \
    h:\book\server0.9b\config.cpp \
    {h:\book\server0.9b;$(INCLUDE);}defines.hpp \
    {h:\book\server0.9b;$(INCLUDE);}config.hpp

.\3wd.obj: \
    h:\book\server0.9b\3wd.cpp \
    {h:\book\server0.9b;$(INCLUDE);}socket.hpp \
    {h:\book\server0.9b;$(INCLUDE);}defines.hpp \
    {h:\book\server0.9b;$(INCLUDE);}3wd.hpp \
    {h:\book\server0.9b;$(INCLUDE);}config.hpp \
    {h:\book\server0.9b;$(INCLUDE);}scodes.hpp

.\socket.obj: \
    h:\book\server0.9b\socket.cpp \
    {h:\book\server0.9b;$(INCLUDE);}socket.hpp \
    {h:\book\server0.9b;$(INCLUDE);}defines.hpp
