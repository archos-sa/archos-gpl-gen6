#############################################################
#
# apdf
#
#############################################################
APDF_SOURCE_DIR:=../packages/apdf
APDF_DIR:=$(BUILD_DIR)/apdf

# This file is used to configure the way we build apdf
APDF_CONF_MK:=conf.mk

APDF_FINAL_CONF_MK:=$(APDF_DIR)/conf.mk

APDF_TARGET_DIR:=$(TARGET_DIR)
APDF_PREFIX=/opt/usr

#
# Source resolution
# FIXME: These settings come from avx, not sure they are ok anymore
#
ifeq ($(strip $(BR2_TARGET_ARCHOS_A405)),y)
      PDF_SOURCE_RES_WIDTH = 600
      PDF_SOURCE_RES_HEIGHT = 850
else
      ifeq ($(strip $(BR2_TARGET_ARCHOS_A405HDD)),y)
            PDF_SOURCE_RES_WIDTH = 600
            PDF_SOURCE_RES_HEIGHT = 850
      else
            PDF_SOURCE_RES_WIDTH = 1200
            PDF_SOURCE_RES_HEIGHT = 1700
      endif
endif

$(APDF_DIR)/.unpacked:
	rm -rf $(APDF_DIR)
	cp -a $(APDF_SOURCE_DIR) $(BUILD_DIR)
	-$(MAKE) -C $(APDF_DIR) clean
	-rm $(APDF_FINAL_CONF_MK)
	touch $(APDF_DIR)/.unpacked

apdf-source: $(APDF_DIR)/.unpacked

$(APDF_DIR)/.configured: $(APDF_DIR)/.unpacked
	echo "PREFIX=$(APDF_PREFIX)"                          >> $(APDF_FINAL_CONF_MK)
	echo "ARCH=$(ARCH)"                                   >> $(APDF_FINAL_CONF_MK)
	echo "BUILD_DIR=$(BUILD_DIR)"                         >> $(APDF_FINAL_CONF_MK)
	echo "STAGING_DIR=$(STAGING_DIR)"                     >> $(APDF_FINAL_CONF_MK)
	echo "BR2_QTE_VERSION=$(BR2_QTE_VERSION)"             >> $(APDF_FINAL_CONF_MK)
	echo "BR2_QTE_TMAKE_VERSION=$(BR2_QTE_TMAKE_VERSION)" >> $(APDF_FINAL_CONF_MK)
	echo "PDF_SOURCE_RES_WIDTH=$(PDF_SOURCE_RES_WIDTH)"   >> $(APDF_FINAL_CONF_MK)
	echo "PDF_SOURCE_RES_HEIGHT=$(PDF_SOURCE_RES_HEIGHT)" >> $(APDF_FINAL_CONF_MK)
	cat package/apdf/$(APDF_CONF_MK)                      >> $(APDF_FINAL_CONF_MK)
	touch  $(APDF_DIR)/.configured

$(APDF_DIR)/.compiled: $(APDF_DIR)/.configured
	$(MAKE) -C $(APDF_DIR)
	touch $(APDF_DIR)/.compiled

$(STAGING_DIR)$(APDF_PREFIX)/bin/apdf: $(APDF_DIR)/.compiled
	$(MAKE) -C $(APDF_DIR) install DESTDIR=$(STAGING_DIR)
	touch -c $(STAGING_DIR)$(APDF_PREFIX)/bin/apdf

$(APDF_TARGET_DIR)$(APDF_PREFIX)/bin/apdf: $(STAGING_DIR)$(APDF_PREFIX)/bin/apdf
	mkdir -p $(APDF_TARGET_DIR)$(APDF_PREFIX)/bin/
	cp -dpf $(STAGING_DIR)$(APDF_PREFIX)/bin/apdf $(APDF_TARGET_DIR)$(APDF_PREFIX)/bin/
	-$(STRIPCMD) $(STRIP_STRIP_UNNEEDED) $(APDF_TARGET_DIR)$(APDF_PREFIX)/bin/apdf

	# FIXME: Don't install all icons
	mkdir -p $(APDF_TARGET_DIR)$(APDF_PREFIX)/share/
	cp -a $(STAGING_DIR)$(APDF_PREFIX)/share/apdf $(APDF_TARGET_DIR)$(APDF_PREFIX)/share/

	# Install helper script
	cp -dpf package/apdf/pdf $(APDF_TARGET_DIR)$(APDF_PREFIX)/bin/

apdf: qte libarchos_support poppler $(APDF_TARGET_DIR)$(APDF_PREFIX)/bin/apdf

apdf-clean:
	-$(MAKE) DESTDIR=$(STAGING_DIR) -C $(APDF_DIR) uninstall
	-rm $(APDF_TARGET_DIR)$(APDF_PREFIX)/bin/apdf
	-$(MAKE) -C $(APDF_DIR) clean

apdf-dirclean:
	rm -rf $(APDF_DIR)

#############################################################
#
# Toplevel Makefile options
#
#############################################################
ifeq ($(strip $(BR2_PACKAGE_APDF)),y)
TARGETS+=apdf
endif
