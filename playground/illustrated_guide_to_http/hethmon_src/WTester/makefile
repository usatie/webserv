# makefile
# Created by IBM WorkFrame/2 MakeMake at 19:16:07 on 07/14/96
#
# The actions included in this make file are:
#  Compile
#  Link

.SUFFIXES:

.all: \
    .\tester.exe

.\socket.obj: \
    h:\book\wtester\socket.cpp \
    {h:\book\wtester;$(INCLUDE);}socket.hpp \
    {h:\book\wtester;$(INCLUDE);}defines.hpp
    @echo " Compile "
    icc.exe /Sp4 /Q /Fi /Si /Gm /Oc -qtune=486 /Gi /Gx /Fo".\%|fF.obj" /C h:\book\wtester\socket.cpp

.\headers.obj: \
    h:\book\wtester\headers.cpp \
    {h:\book\wtester;$(INCLUDE);}socket.hpp \
    {h:\book\wtester;$(INCLUDE);}defines.hpp \
    {h:\book\wtester;$(INCLUDE);}headers.hpp
    @echo " Compile "
    icc.exe /Sp4 /Q /Fi /Si /Gm /Oc -qtune=486 /Gi /Gx /Fo".\%|fF.obj" /C h:\book\wtester\headers.cpp

.\tester.obj: \
    h:\book\wtester\tester.cpp \
    {h:\book\wtester;$(INCLUDE);}socket.hpp \
    {h:\book\wtester;$(INCLUDE);}defines.hpp \
    {h:\book\wtester;$(INCLUDE);}headers.hpp
    @echo " Compile "
    icc.exe /Sp4 /Q /Fi /Si /Gm /Oc -qtune=486 /Gi /Gx /Fo".\%|fF.obj" /C h:\book\wtester\tester.cpp

.\tester.exe: \
    .\socket.obj \
    .\tester.obj \
    .\headers.obj \
    {$(LIB)}wsock32.lib
    @echo " Link "
    icc.exe @<<
     /B" /pmtype:vio /nologo /code:RX /data:RW"
     /B" /def"
     /B" /def:wsock32"
     /Fetester.exe 
     wsock32.lib 
     .\socket.obj
     .\tester.obj
     .\headers.obj
<<
