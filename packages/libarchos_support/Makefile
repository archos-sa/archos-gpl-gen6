PRJNAME=libarchos_support

include conf.mk

export QTDIR
export TMAKEPATH

BIN_DIR=$(DESTDIR)/$(PREFIX)/bin
LIB_DIR=$(DESTDIR)/$(PREFIX)/lib
INC_DIR=$(DESTDIR)/$(PREFIX)/include/archos
PUBLICINCLUDES= \
	aimage.h \
	alog.h \
	astring.h \
	atr.h \
	avossocket.h \
	dmalloc.h \
	extapp_msg.h \
	screen.h \

all:
	$(MAKE) src/Makefile
	$(MAKE) -C src

src/Makefile: src/$(PRJNAME).pro
	cd src && $(TMAKE) $(PRJNAME).pro -o Makefile

src/$(PRJNAME).pro: conf.mk src/$(PRJNAME).pro.tmpl
	./tmplgen \
		--template src/$(PRJNAME).pro.tmpl --output $@ \
		BUILD_TYPE=$(BUILD_TYPE) TARGET=$(TARGET)

install: all
	mkdir -p $(LIB_DIR) $(INC_DIR) $(BIN_DIR)
	cp -a src/libarchos_support.so* $(LIB_DIR)
	cd src && cp $(PUBLICINCLUDES) $(INC_DIR)

clean:
	-$(MAKE) -C src clean
	-rm src/Makefile
	-rm src/$(PRJNAME).pro
