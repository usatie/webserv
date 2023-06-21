# makefile
# Created by IBM WorkFrame/2 MakeMake at 21:36:33 on 15 July 1996
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

{h:\book\server1.0}.cpp.obj:
    @echo " Compile::C++ Compiler "
    icc.exe /Q /Fi /Si /O /Gm /G4 /Gf /Gi /Gx /C %s

.\3wd.exe: \
    .\socket.obj \
    .\util.obj \
    .\3wd.obj \
    .\base64.obj \
    .\cgios2.obj \
    .\config.obj \
    .\http10.obj \
    .\index.obj \
    .\scodes.obj \
    {$(LIB)}so32dll.lib \
    {$(LIB)}tcp32dll.lib
    @echo " Link::Linker "
    icc.exe @<<
     /B" /exepack:2 /packd /align:1 /nologo"
     /Fe3wd.exe 
     so32dll.lib 
     tcp32dll.lib 
     .\socket.obj
     .\util.obj
     .\3wd.obj
     .\base64.obj
     .\cgios2.obj
     .\config.obj
     .\http10.obj
     .\index.obj
     .\scodes.obj
<<

.\socket.obj: \
    h:\book\server1.0\socket.cpp \
    {h:\book\server1.0;$(INCLUDE);}defines.hpp \
    {h:\book\server1.0;$(INCLUDE);}socket.hpp

.\scodes.obj: \
    h:\book\server1.0\scodes.cpp

.\index.obj: \
    h:\book\server1.0\index.cpp \
    {h:\book\server1.0;$(INCLUDE);}defines.hpp \
    {h:\book\server1.0;$(INCLUDE);}config.hpp \
    {h:\book\server1.0;$(INCLUDE);}socket.hpp \
    {h:\book\server1.0;$(INCLUDE);}3wd.hpp \
    {h:\book\server1.0;$(INCLUDE);}scodes.hpp

.\http10.obj: \
    h:\book\server1.0\http10.cpp \
    {h:\book\server1.0;$(INCLUDE);}defines.hpp \
    {h:\book\server1.0;$(INCLUDE);}config.hpp \
    {h:\book\server1.0;$(INCLUDE);}socket.hpp \
    {h:\book\server1.0;$(INCLUDE);}3wd.hpp \
    {h:\book\server1.0;$(INCLUDE);}http10.hpp \
    {h:\book\server1.0;$(INCLUDE);}util.hpp \
    {h:\book\server1.0;$(INCLUDE);}scodes.hpp \
    {h:\book\server1.0;$(INCLUDE);}cgi.hpp

.\config.obj: \
    h:\book\server1.0\config.cpp \
    {h:\book\server1.0;$(INCLUDE);}defines.hpp \
    {h:\book\server1.0;$(INCLUDE);}config.hpp

.\cgios2.obj: \
    h:\book\server1.0\cgios2.cpp \
    {h:\book\server1.0;$(INCLUDE);}defines.hpp \
    {h:\book\server1.0;$(INCLUDE);}config.hpp \
    {h:\book\server1.0;$(INCLUDE);}socket.hpp \
    {h:\book\server1.0;$(INCLUDE);}http10.hpp \
    {h:\book\server1.0;$(INCLUDE);}cgi.hpp

.\base64.obj: \
    h:\book\server1.0\base64.cpp \
    {h:\book\server1.0;$(INCLUDE);}base64.hpp

.\3wd.obj: \
    h:\book\server1.0\3wd.cpp \
    {h:\book\server1.0;$(INCLUDE);}defines.hpp \
    {h:\book\server1.0;$(INCLUDE);}config.hpp \
    {h:\book\server1.0;$(INCLUDE);}socket.hpp \
    {h:\book\server1.0;$(INCLUDE);}3wd.hpp \
    {h:\book\server1.0;$(INCLUDE);}http10.hpp \
    {h:\book\server1.0;$(INCLUDE);}util.hpp \
    {h:\book\server1.0;$(INCLUDE);}scodes.hpp \
    {h:\book\server1.0;$(INCLUDE);}cgi.hpp

.\util.obj: \
    h:\book\server1.0\util.cpp \
    {h:\book\server1.0;$(INCLUDE);}defines.hpp \
    {h:\book\server1.0;$(INCLUDE);}config.hpp \
    {h:\book\server1.0;$(INCLUDE);}socket.hpp \
    {h:\book\server1.0;$(INCLUDE);}3wd.hpp \
    {h:\book\server1.0;$(INCLUDE);}http10.hpp \
    {h:\book\server1.0;$(INCLUDE);}util.hpp \
    {h:\book\server1.0;$(INCLUDE);}base64.hpp
