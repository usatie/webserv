# makefile
# Created by IBM WorkFrame/2 MakeMake at 21:30:54 on 30 June 1996
#
# The actions included in this make file are:
#  Compile::C++ Compiler
#  Link::Linker

.SUFFIXES: .cpp .obj 

.all: \
    .\Tester.exe

.cpp.obj:
    @echo " Compile::C++ Compiler "
    icc.exe /Q /Fi /Si /Gm /G4 /Gf /Gi /Gx /O+ /Oc+ /C %s

{h:\book\tester}.cpp.obj:
    @echo " Compile::C++ Compiler "
    icc.exe /Q /Fi /Si /Gm /G4 /Gf /Gi /Gx /O+ /Oc+ /C %s

.\Tester.exe: \
    .\socket.obj \
    .\tester.obj \
    .\headers.obj \
    {$(LIB)}so32dll.lib \
    {$(LIB)}tcp32dll.lib
    @echo " Link::Linker "
    icc.exe @<<
    /Fi /Si /Gm /G4 /Gf /Gi /Gx /O+ /Oc+
     /B" /exepack:2 /packd /align:1 /nologo /l"
     /FeTester.exe 
     so32dll.lib 
     tcp32dll.lib 
     .\socket.obj
     .\tester.obj
     .\headers.obj
<<

.\socket.obj: \
    h:\book\tester\socket.cpp \
    {h:\book\tester;$(INCLUDE);}socket.hpp \
    {h:\book\tester;$(INCLUDE);}defines.hpp

.\headers.obj: \
    h:\book\tester\headers.cpp \
    {h:\book\tester;$(INCLUDE);}socket.hpp \
    {h:\book\tester;$(INCLUDE);}defines.hpp \
    {h:\book\tester;$(INCLUDE);}headers.hpp

.\tester.obj: \
    h:\book\tester\tester.cpp \
    {h:\book\tester;$(INCLUDE);}socket.hpp \
    {h:\book\tester;$(INCLUDE);}defines.hpp \
    {h:\book\tester;$(INCLUDE);}headers.hpp
