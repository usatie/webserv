# makefile
# Created by IBM WorkFrame/2 MakeMake at 23:52:29 on 07/15/96
#
# The actions included in this make file are:
#  Compile
#  Link

.SUFFIXES:

.all: \
    .\3wd.exe

.\socket.obj: \
    h:\book\wserver1.0\socket.cpp \
    {h:\book\wserver1.0;$(INCLUDE);}socket.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}defines.hpp
    @echo " Compile "
    icc.exe /Sp4 /Q /Fi /Si /Gm /Oc -qtune=486 /Gi /Gx /Fo".\%|fF.obj" /C h:\book\wserver1.0\socket.cpp

.\scodes.obj: \
    h:\book\wserver1.0\scodes.cpp
    @echo " Compile "
    icc.exe /Sp4 /Q /Fi /Si /Gm /Oc -qtune=486 /Gi /Gx /Fo".\%|fF.obj" /C h:\book\wserver1.0\scodes.cpp

.\index.obj: \
    h:\book\wserver1.0\index.cpp \
    {h:\book\wserver1.0;$(INCLUDE);}socket.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}defines.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}3wd.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}config.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}scodes.hpp
    @echo " Compile "
    icc.exe /Sp4 /Q /Fi /Si /Gm /Oc -qtune=486 /Gi /Gx /Fo".\%|fF.obj" /C h:\book\wserver1.0\index.cpp

.\http10.obj: \
    h:\book\wserver1.0\http10.cpp \
    {h:\book\wserver1.0;$(INCLUDE);}socket.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}defines.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}3wd.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}config.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}scodes.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}http10.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}util.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}cgi.hpp
    @echo " Compile "
    icc.exe /Sp4 /Q /Fi /Si /Gm /Oc -qtune=486 /Gi /Gx /Fo".\%|fF.obj" /C h:\book\wserver1.0\http10.cpp

.\config.obj: \
    h:\book\wserver1.0\config.cpp \
    {h:\book\wserver1.0;$(INCLUDE);}defines.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}config.hpp
    @echo " Compile "
    icc.exe /Sp4 /Q /Fi /Si /Gm /Oc -qtune=486 /Gi /Gx /Fo".\%|fF.obj" /C h:\book\wserver1.0\config.cpp

.\cgiwin.obj: \
    h:\book\wserver1.0\cgiwin.cpp \
    {h:\book\wserver1.0;$(INCLUDE);}defines.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}config.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}cgi.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}socket.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}http10.hpp
    @echo " Compile "
    icc.exe /Sp4 /Q /Fi /Si /Gm /Oc -qtune=486 /Gi /Gx /Fo".\%|fF.obj" /C h:\book\wserver1.0\cgiwin.cpp

.\base64.obj: \
    h:\book\wserver1.0\base64.cpp \
    {h:\book\wserver1.0;$(INCLUDE);}base64.hpp
    @echo " Compile "
    icc.exe /Sp4 /Q /Fi /Si /Gm /Oc -qtune=486 /Gi /Gx /Fo".\%|fF.obj" /C h:\book\wserver1.0\base64.cpp

.\3wd.obj: \
    h:\book\wserver1.0\3wd.cpp \
    {h:\book\wserver1.0;$(INCLUDE);}socket.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}defines.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}3wd.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}config.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}scodes.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}http10.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}util.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}cgi.hpp
    @echo " Compile "
    icc.exe /Sp4 /Q /Fi /Si /Gm /Oc -qtune=486 /Gi /Gx /Fo".\%|fF.obj" /C h:\book\wserver1.0\3wd.cpp

.\util.obj: \
    h:\book\wserver1.0\util.cpp \
    {h:\book\wserver1.0;$(INCLUDE);}defines.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}config.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}socket.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}3wd.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}http10.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}util.hpp \
    {h:\book\wserver1.0;$(INCLUDE);}base64.hpp
    @echo " Compile "
    icc.exe /Sp4 /Q /Fi /Si /Gm /Oc -qtune=486 /Gi /Gx /Fo".\%|fF.obj" /C h:\book\wserver1.0\util.cpp

.\3wd.exe: \
    .\socket.obj \
    .\util.obj \
    .\3wd.obj \
    .\base64.obj \
    .\cgiwin.obj \
    .\config.obj \
    .\http10.obj \
    .\index.obj \
    .\scodes.obj \
    {$(LIB)}wsock32.lib
    @echo " Link "
    icc.exe @<<
     /B" /pmtype:vio /nologo /code:RX /data:RW"
     /B" /def"
     /B" /def:wsock32"
     /Fe3wd.exe 
     wsock32.lib 
     .\socket.obj
     .\util.obj
     .\3wd.obj
     .\base64.obj
     .\cgiwin.obj
     .\config.obj
     .\http10.obj
     .\index.obj
     .\scodes.obj
<<
