CC = gcc
ECHO = echo
RM = rm -f
TAR = tar
ZIP = zip
MKDIR = mkdir
CHMOD = chmod
CP = rsync -R
CFLAGS = -Wall -O3
CPPFLAGS = -I.
LDFLAGS = -lm -lassimp -lSDL2_image
PACKNAME = API8_2024
PROGNAME = demo
VERSION = 1.0
distdir = $(PACKNAME)_$(PROGNAME)-$(VERSION)
HEADERS = $(wildcard headers/*.h)
SOURCES = main.c $(wildcard sources/*.c)
MSVCSRC = $(patsubst %,<ClCompile Include=\"%\\\" \\/>,$(SOURCES))
OBJ = $(SOURCES:.c=.o)
DOXYFILE = documentation/Doxyfile
VSCFILES = $(PROGNAME).vcxproj $(PROGNAME).sln
EXTRAFILES = COPYING $(wildcard shaders/*.?s models/* audio/* ) $(VSCFILES)
DISTFILES = $(SOURCES) Makefile $(HEADERS) $(DOXYFILE) $(EXTRAFILES)
ifneq (,$(shell ls -d /usr/local/include 2>/dev/null | tail -n 1))
	CPPFLAGS += -I/usr/local/include
endif
ifneq (,$(shell ls -d $(HOME)/local/include 2>/dev/null | tail -n 1))
	CPPFLAGS += -I$(HOME)/local/include
endif
ifneq (,$(shell ls -d /usr/local/lib 2>/dev/null | tail -n 1))
	LDFLAGS += -L/usr/local/lib
endif
ifneq (,$(shell ls -d $(HOME)/local/lib 2>/dev/null | tail -n 1))
	LDFLAGS += -L$(HOME)/local/lib
endif
ifeq ($(shell uname),Darwin)
	MACOSX_DEPLOYMENT_TARGET = 11.0
        CFLAGS += -mmacosx-version-min=$(MACOSX_DEPLOYMENT_TARGET)
        LDFLAGS += -framework OpenGL -mmacosx-version-min=$(MACOSX_DEPLOYMENT_TARGET)
else
        LDFLAGS += -lGL
endif
CPPFLAGS += $(shell sdl2-config --cflags)
LDFLAGS  += -lGL4Dummies $(shell sdl2-config --libs) -lSDL2_mixer -lSDL2_image -lSDL2_ttf
all: $(PROGNAME)
$(PROGNAME): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $(PROGNAME)
	upx --best $(PROGNAME)
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
dist: distdir
	$(CHMOD) -R a+r $(distdir)
	$(TAR) zcvf $(distdir).tgz $(distdir)
	$(RM) -r $(distdir)
# $(MAKE) msvc # Comment out this line if not needed
zip: distdir
	$(CHMOD) -R a+r $(distdir)
	$(ZIP) -r $(distdir).zip $(distdir)
	$(RM) -r $(distdir)
# $(MAKE) msvc # Comment out this line if not needed
distdir: $(DISTFILES)
	$(RM) -r $(distdir)
	$(MKDIR) $(distdir)
	$(CHMOD) 777 $(distdir)
	$(CP) $(DISTFILES) $(distdir)
doc: $(DOXYFILE)
	cat $< | sed -e "s/PROJECT_NAME *=.*/PROJECT_NAME = $(PROGNAME)/" |\
	  sed -e "s/PROJECT_NUMBER *=.*/PROJECT_NUMBER = $(VERSION)/" >> $<.new
	mv -f $<.new $<
	cd documentation && doxygen && cd ..
msvc: $(VSCFILES)
	@echo "Now these files ($?) already exist. If you wish to regenerate them, you should first delete them manually."
$(VSCFILES):
	@if [ -f ../../Windows/templates/gl4dSample$(suffix $@) ]; then \
		echo "Generating $@ ..."; \
		cat ../../Windows/templates/gl4dSample$(suffix $@) | sed -e "s/INSERT_PROJECT_NAME/$(PROGNAME)/g" | sed -e "s/INSERT_TARGET_NAME/$(PROGNAME)/" | sed -e "s/INSERT_SOURCE_FILES/$(MSVCSRC)/" > $@; \
	else \
		echo "Template file ../../Windows/templates/gl4dSample$(suffix $@) not found. Skipping generation of $@"; \
	fi
clean:
	@$(RM) -r $(PROGNAME) $(OBJ) *~ $(distdir).tgz $(distdir).zip gmon.out	\
	  core.* documentation/*~ shaders/*~ documentation/html