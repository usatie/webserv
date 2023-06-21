# makefile
# Created by IBM WorkFrame/2 MakeMake at 22:06:07 on 24 July 1996
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

{h:\book\server1.1}.cpp.obj:
    @echo " Compile::C++ Compiler "
    icc.exe /Q /Fi /Si /O /Gm /G4 /Gf /Gi /Gx /C %s

.\3wd.exe: \
    .\socket.obj \
    .\util.obj \
    .\3wd.obj \
    .\base64.obj \
    .\cgios2.obj \
    .\config.obj \
    .\headers.obj \
    .\http10.obj \
    .\http11.obj \
    .\index.obj \
    .\scodes.obj \
    {$(LIB)}so32dll.lib \
    {$(LIB)}tcp32dll.lib
    @echo " Link::Linker "
    icc.exe @<<
     /B" /exepack:2 /packd /align:1 /optfunc /l"
     /Fe3wd.exe 
     so32dll.lib 
     tcp32dll.lib 
     .\socket.obj
     .\util.obj
     .\3wd.obj
     .\base64.obj
     .\cgios2.obj
     .\config.obj
     .\headers.obj
     .\http10.obj
     .\http11.obj
     .\index.obj
     .\scodes.obj
<<

.\socket.obj: \
    h:\book\server1.1\socket.cpp \
    {h:\book\server1.1;$(INCLUDE);}defines.hpp \
    {h:\book\server1.1;$(INCLUDE);}socket.hpp

.\scodes.obj: \
    h:\book\server1.1\scodes.cpp

.\index.obj: \
    h:\book\server1.1\index.cpp \
    {h:\book\server1.1;$(INCLUDE);}defines.hpp \
    {h:\book\server1.1;$(INCLUDE);}config.hpp \
    {h:\book\server1.1;$(INCLUDE);}socket.hpp \
    {h:\book\server1.1;$(INCLUDE);}3wd.hpp \
    {h:\book\server1.1;$(INCLUDE);}scodes.hpp

.\http11.obj: \
    h:\book\server1.1\http11.cpp \
    {h:\book\server1.1;$(INCLUDE);}defines.hpp \
    {h:\book\server1.1;$(INCLUDE);}config.hpp \
    {h:\book\server1.1;$(INCLUDE);}socket.hpp \
    {h:\book\server1.1;$(INCLUDE);}3wd.hpp \
    {h:\book\server1.1;$(INCLUDE);}http10.hpp \
    {h:\book\server1.1;$(INCLUDE);}util.hpp \
    {h:\book\server1.1;$(INCLUDE);}headers.hpp \
    {h:\book\server1.1;$(INCLUDE);}scodes.hpp \
    {h:\book\server1.1;$(INCLUDE);}http11.hpp \
    {h:\book\server1.1;$(INCLUDE);}cgi.hpp

.\http10.obj: \
    h:\book\server1.1\http10.cpp \
    {h:\book\server1.1;$(INCLUDE);}defines.hpp \
    {h:\book\server1.1;$(INCLUDE);}config.hpp \
    {h:\book\server1.1;$(INCLUDE);}socket.hpp \
    {h:\book\server1.1;$(INCLUDE);}3wd.hpp \
    {h:\book\server1.1;$(INCLUDE);}http10.hpp \
    {h:\book\server1.1;$(INCLUDE);}util.hpp \
    {h:\book\server1.1;$(INCLUDE);}headers.hpp \
    {h:\book\server1.1;$(INCLUDE);}scodes.hpp \
    {h:\book\server1.1;$(INCLUDE);}http11.hpp \
    {h:\book\server1.1;$(INCLUDE);}cgi.hpp

.\headers.obj: \
    h:\book\server1.1\headers.cpp \
    {h:\book\server1.1;$(INCLUDE);}defines.hpp \
    {h:\book\server1.1;$(INCLUDE);}config.hpp \
    {h:\book\server1.1;$(INCLUDE);}socket.hpp \
    {h:\book\server1.1;$(INCLUDE);}3wd.hpp \
    {h:\book\server1.1;$(INCLUDE);}http10.hpp \
    {h:\book\server1.1;$(INCLUDE);}util.hpp \
    {h:\book\server1.1;$(INCLUDE);}headers.hpp \
    {h:\book\server1.1;$(INCLUDE);}scodes.hpp

.\config.obj: \
    h:\book\server1.1\config.cpp \
    {h:\book\server1.1;$(INCLUDE);}defines.hpp \
    {h:\book\server1.1;$(INCLUDE);}config.hpp

.\cgios2.obj: \
    h:\book\server1.1\cgios2.cpp \
    {h:\book\server1.1;$(INCLUDE);}defines.hpp \
    {h:\book\server1.1;$(INCLUDE);}config.hpp \
    {h:\book\server1.1;$(INCLUDE);}socket.hpp \
    {h:\book\server1.1;$(INCLUDE);}headers.hpp \
    {h:\book\server1.1;$(INCLUDE);}cgi.hpp

.\base64.obj: \
    h:\book\server1.1\base64.cpp \
    {h:\book\server1.1;$(INCLUDE);}base64.hpp

.\3wd.obj: \
    h:\book\server1.1\3wd.cpp \
    {h:\book\server1.1;$(INCLUDE);}defines.hpp \
    {h:\book\server1.1;$(INCLUDE);}config.hpp \
    {h:\book\server1.1;$(INCLUDE);}socket.hpp \
    {h:\book\server1.1;$(INCLUDE);}3wd.hpp \
    {h:\book\server1.1;$(INCLUDE);}http10.hpp \
    {h:\book\server1.1;$(INCLUDE);}util.hpp \
    {h:\book\server1.1;$(INCLUDE);}headers.hpp \
    {h:\book\server1.1;$(INCLUDE);}scodes.hpp \
    {h:\book\server1.1;$(INCLUDE);}http11.hpp \
    {h:\book\server1.1;$(INCLUDE);}cgi.hpp

.\util.obj: \
    h:\book\server1.1\util.cpp \
    {h:\book\server1.1;$(INCLUDE);}defines.hpp \
    {h:\book\server1.1;$(INCLUDE);}config.hpp \
    {h:\book\server1.1;$(INCLUDE);}socket.hpp \
    {h:\book\server1.1;$(INCLUDE);}3wd.hpp \
    {h:\book\server1.1;$(INCLUDE);}http10.hpp \
    {h:\book\server1.1;$(INCLUDE);}util.hpp \
    {h:\book\server1.1;$(INCLUDE);}base64.hpp \
    {h:\book\server1.1;$(INCLUDE);}headers.hpp
