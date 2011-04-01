#############################################################
#
# fusesmb
#
#############################################################

FUSESMB_SOURCE_DIR:=../packages/fusesmb
FUSESMB_VERSION:=0.8.4
FUSESMB_SOURCE:=fusesmb-$(FUSESMB_VERSION).tar.gz
FUSESMB_DIR:=$(BUILD_DIR)/fusesmb

#FIXME: OPT_TARGET_DIR needs to be set in buildroot!
OPT_TARGET_DIR:=$(TARGET_DIR)/opt
# samba will be installed on opt-fs
FUSESMB_TARGET_DIR:=$(OPT_TARGET_DIR)

$(FUSESMB_DIR)/.unpacked:
	cp -a $(FUSESMB_SOURCE_DIR) $(BUILD_DIR)
	-$(MAKE) -C $(FUSESMB_DIR) clean
	touch $(FUSESMB_DIR)/.unpacked

$(FUSESMB_DIR)/.configured: $(FUSESMB_DIR)/.unpacked
	(cd $(FUSESMB_DIR); rm -rf config.cache; \
	/usr/bin/autoreconf && \
	/usr/bin/automake --add-missing && \
	$(TARGET_CONFIGURE_OPTS) \
	CFLAGS="$(TARGET_CFLAGS) -I$(STAGING_DIR)/usr/include -DARCHOS" \
	LDFLAGS="-L$(STAGING_DIR)/lib -L$(STAGING_DIR)/usr/lib \
	-Wl,-rpath,$(FUSESMB_TARGET_DIR)/usr/lib \
	-Wl,-rpath-link,$(STAGING_DIR)/usr/lib" \
        ac_cv_prog_NMBLOOKUP=yes \
	./configure \
	--target=$(GNU_TARGET_NAME) \
	--host=$(GNU_TARGET_NAME) \
	--build=$(GNU_HOST_NAME) \
	--prefix=/usr );
	touch  $(FUSESMB_DIR)/.configured

fusesmb-compile: $(FUSESMB_DIR)/.configured
	$(MAKE) -C $(FUSESMB_DIR)

$(FUSESMB_TARGET_DIR)/usr/bin/fusesmb:
	$(MAKE) -C $(FUSESMB_DIR) DESTDIR=$(FUSESMB_TARGET_DIR) install 
	-$(STRIPCMD) --strip-unneeded $(FUSESMB_TARGET_DIR)/usr/bin/fusesmb
	-$(STRIPCMD) --strip-unneeded $(FUSESMB_TARGET_DIR)/usr/bin/fusesmb.cache
	rm -rf $(FUSESMB_TARGET_DIR)/usr/man
	touch -c $(FUSESMB_TARGET_DIR)/usr/bin/fusesmb

fusesmb: uclibc fuse samba fusesmb-compile $(FUSESMB_TARGET_DIR)/usr/bin/fusesmb

fusesmb-clean:
	-$(MAKE) DESTDIR=$(FUSESMB_TARGET_DIR) CC=$(TARGET_CC) -C $(FUSESMB_DIR) uninstall
	-$(MAKE) -C $(FUSESMB_DIR) clean

fusesmb-dirclean: fusesmb-clean
	rm -rf $(FUSESMB_DIR)

#############################################################
#
# Toplevel Makefile options
#
#############################################################
ifeq ($(strip $(BR2_PACKAGE_FUSESMB)),y)
TARGETS+=fusesmb
endif
