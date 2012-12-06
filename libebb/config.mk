# libev
EVINC  = ../libev
EVLIB  = ../libev/.libs
EVLIBS = -L${EVLIB} -lev

# GnuTLS, comment if you don't want it (necessary for HTTPS)
GNUTLSLIB   = /usr/lib
GNUTLSINC   = /usr/include
GNUTLSLIBS  = -L${GNUTLSLIB} -lgnutls
GNUTLSFLAGS = -DHAVE_GNUTLS
INCS += -I${GNUTLSINC}

# includes and libs
INCS += -I${EVINC}
LIBS = ${EVLIBS} ${GNUTLSLIBS}

# flags
CPPFLAGS = -DVERSION=\"$(VERSION)\" ${GNUTLSFLAGS}
CFLAGS   += -O2 -g -Werror -Wall ${INCS} ${CPPFLAGS} -fPIC -DEBB_DEFAULT_TIMEOUT=30.0

LDFLAGS  = -s ${LIBS}
LDOPT    = -shared
SUFFIX   = so
SONAME   = -Wl,-soname,$(OUTPUT_LIB)

# Solaris
#CFLAGS  = -fast ${INCS} -DVERSION=\"$(VERSION)\" -fPIC
#LDFLAGS = ${LIBS}
#SONAME  = 

# Darwin
# LDOPT  = -dynamiclib 
# SUFFIX = dylib
# SONAME = -current_version $(VERSION) -compatibility_version $(VERSION)

# compiler and linker
CC = cc
RANLIB = ranlib
