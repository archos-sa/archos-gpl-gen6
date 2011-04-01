PRJNAME=apdf

include conf.mk

export QTDIR
export TMAKEPATH

BIN_DIR=$(DESTDIR)/$(PREFIX)/bin
ICONS_DIR=$(DESTDIR)/$(PREFIX)/share/apdf/icons

# Should tmplgen be installed in the staging dir?
TMPLGEN=../libarchos_support/tmplgen

all:
	$(MAKE) src/Makefile
	$(MAKE) -C src

src/Makefile: src/$(PRJNAME).pro
	cd src && $(TMAKE) $(PRJNAME).pro -o Makefile

src/$(PRJNAME).pro: conf.mk src/$(PRJNAME).pro.tmpl
	$(TMPLGEN) \
		--template src/$(PRJNAME).pro.tmpl --output $@ \
		BUILD_TYPE=$(BUILD_TYPE) \
		PREFIX=$(PREFIX) \
		TARGET=$(TARGET) \
		LIBARCHOS_SUPPORT_DIR=$(LIBARCHOS_SUPPORT_DIR) \
		POPPLER_DIR=$(POPPLER_DIR) \
		PDF_SOURCE_RES_WIDTH=$(PDF_SOURCE_RES_WIDTH) \
		PDF_SOURCE_RES_HEIGHT=$(PDF_SOURCE_RES_HEIGHT)

install: all
	mkdir -p $(BIN_DIR) $(ICONS_DIR)
	cp -a src/apdf $(BIN_DIR)
	tar -C src/icons --exclude .svn -c . | tar -C $(ICONS_DIR) -xv

clean:
	-$(MAKE) -C src clean
	-rm src/Makefile
	-rm src/$(PRJNAME).pro
