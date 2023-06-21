# makefile
# Created by IBM WorkFrame/2 MakeMake at 23:15:46 on 07/24/96
#
# The actions included in this make file are:
#  Compile
#  Link

.SUFFIXES:

.all: \
    .\3wd.exe

.\socket.obj: \
    h:\book\wserver1.1\socket.cpp \
    {h:\book\wserver1.1;$(INCLUDE);}socket.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}defines.hpp
    @echo " Compile "
    icc.exe /Sp4 /Q /Fi /Si /Gm /Oc -qtune=486 /Gi /Gx /Fo".\%|fF.obj" /C h:\book\wserver1.1\socket.cpp

.\scodes.obj: \
    h:\book\wserver1.1\scodes.cpp
    @echo " Compile "
    icc.exe /Sp4 /Q /Fi /Si /Gm /Oc -qtune=486 /Gi /Gx /Fo".\%|fF.obj" /C h:\book\wserver1.1\scodes.cpp

.\index.obj: \
    h:\book\wserver1.1\index.cpp \
    {h:\book\wserver1.1;$(INCLUDE);}socket.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}defines.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}3wd.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}config.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}scodes.hpp
    @echo " Compile "
    icc.exe /Sp4 /Q /Fi /Si /Gm /Oc -qtune=486 /Gi /Gx /Fo".\%|fF.obj" /C h:\book\wserver1.1\index.cpp

.\http11.obj: \
    h:\book\wserver1.1\http11.cpp \
    {h:\book\wserver1.1;$(INCLUDE);}socket.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}defines.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}3wd.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}config.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}scodes.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}headers.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}http11.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}http10.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}util.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}cgi.hpp
    @echo " Compile "
    icc.exe /Sp4 /Q /Fi /Si /Gm /Oc -qtune=486 /Gi /Gx /Fo".\%|fF.obj" /C h:\book\wserver1.1\http11.cpp

.\http10.obj: \
    h:\book\wserver1.1\http10.cpp \
    {h:\book\wserver1.1;$(INCLUDE);}socket.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}defines.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}3wd.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}config.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}scodes.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}http10.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}http11.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}util.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}cgi.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}headers.hpp
    @echo " Compile "
    icc.exe /Sp4 /Q /Fi /Si /Gm /Oc -qtune=486 /Gi /Gx /Fo".\%|fF.obj" /C h:\book\wserver1.1\http10.cpp

.\headers.obj: \
    h:\book\wserver1.1\headers.cpp \
    {h:\book\wserver1.1;$(INCLUDE);}socket.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}defines.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}3wd.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}config.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}scodes.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}http10.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}util.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}headers.hpp
    @echo " Compile "
    icc.exe /Sp4 /Q /Fi /Si /Gm /Oc -qtune=486 /Gi /Gx /Fo".\%|fF.obj" /C h:\book\wserver1.1\headers.cpp

.\config.obj: \
    h:\book\wserver1.1\config.cpp \
    {h:\book\wserver1.1;$(INCLUDE);}defines.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}config.hpp
    @echo " Compile "
    icc.exe /Sp4 /Q /Fi /Si /Gm /Oc -qtune=486 /Gi /Gx /Fo".\%|fF.obj" /C h:\book\wserver1.1\config.cpp

.\cgiwin.obj: \
    h:\book\wserver1.1\cgiwin.cpp \
    {h:\book\wserver1.1;$(INCLUDE);}defines.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}config.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}cgi.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}headers.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}socket.hpp
    @echo " Compile "
    icc.exe /Sp4 /Q /Fi /Si /Gm /Oc -qtune=486 /Gi /Gx /Fo".\%|fF.obj" /C h:\book\wserver1.1\cgiwin.cpp

.\base64.obj: \
    h:\book\wserver1.1\base64.cpp \
    {h:\book\wserver1.1;$(INCLUDE);}base64.hpp
    @echo " Compile "
    icc.exe /Sp4 /Q /Fi /Si /Gm /Oc -qtune=486 /Gi /Gx /Fo".\%|fF.obj" /C h:\book\wserver1.1\base64.cpp

.\3wd.obj: \
    h:\book\wserver1.1\3wd.cpp \
    {h:\book\wserver1.1;$(INCLUDE);}socket.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}defines.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}3wd.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}config.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}scodes.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}headers.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}http10.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}http11.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}util.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}cgi.hpp
    @echo " Compile "
    icc.exe /Sp4 /Q /Fi /Si /Gm /Oc -qtune=486 /Gi /Gx /Fo".\%|fF.obj" /C h:\book\wserver1.1\3wd.cpp

.\util.obj: \
    h:\book\wserver1.1\util.cpp \
    {h:\book\wserver1.1;$(INCLUDE);}defines.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}config.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}socket.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}3wd.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}http10.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}util.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}base64.hpp \
    {h:\book\wserver1.1;$(INCLUDE);}headers.hpp
    @echo " Compile "
    icc.exe /Sp4 /Q /Fi /Si /Gm /Oc -qtune=486 /Gi /Gx /Fo".\%|fF.obj" /C h:\book\wserver1.1\util.cpp

.\3wd.exe: \
    .\socket.obj \
    .\util.obj \
    .\3wd.obj \
    .\base64.obj \
    .\cgiwin.obj \
    .\config.obj \
    .\headers.obj \
    .\http10.obj \
    .\http11.obj \
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
     .\headers.obj
     .\http10.obj
     .\http11.obj
     .\index.obj
     .\scodes.obj
<<
