#############################################################
#
# ffmpeg_tiny
#
#############################################################
FFMPEG_TINY_DIR:=$(BUILD_DIR)/ffmpeg_tiny

FFMPEG_TINY_SRC=ffmpeg_tiny_r8470.tgz

#$(DL_DIR)/ffmpeg_tiny:
#	(cd $(DL_DIR) ;\
#	svn checkout svn://svn.mplayerhq.hu/ffmpeg/trunk ffmpeg )

$(FFMPEG_TINY_DIR)/.unpacked: package/ffmpeg_tiny/cookfixed.patch
	(cd $(BUILD_DIR) ; \
	tar -xvzf $(DL_DIR)/$(FFMPEG_TINY_SRC) ; \
	mv ffmpeg $(FFMPEG_TINY_DIR) )
	cp $(FFMPEG_TINY_DIR)/libavcodec/cook.c $(FFMPEG_TINY_DIR)/libavcodec/cook_float.h
	cp $(FFMPEG_TINY_DIR)/libavcodec/cook.c $(FFMPEG_TINY_DIR)/libavcodec/cookdata_float.h
	toolchain/patch-kernel.sh $(FFMPEG_TINY_DIR) package/ffmpeg_tiny/ \*.patch
	touch $@

$(FFMPEG_TINY_DIR)/.configured: $(FFMPEG_TINY_DIR)/.unpacked
	(cd $(FFMPEG_TINY_DIR); \
	$(TARGET_CONFIGURE_OPTS) \
		$(TARGET_CONFIGURE_ARGS) \
	./configure \
	--arch=$(ARCH) \
	--cc=$(TARGET_CC) \
	--cross-compile \
	--prefix=/usr  \
	--libdir=/usr/lib \
	--enable-shared \
	--disable-static \
	--disable-mmx \
	--enable-memalign-hack );
	touch $@

$(FFMPEG_TINY_DIR)/.compiled: $(FFMPEG_TINY_DIR)/.configured
	$(TARGET_CONFIGURE_OPTS) \
	$(MAKE) -C $(FFMPEG_TINY_DIR) lib
	touch $@

$(FFMPEG_TINY_DIR)/.installed: $(FFMPEG_TINY_DIR)/.compiled
	mkdir -p $(STAGING_DIR)/usr/include/ffmpeg
	cp $(FFMPEG_TINY_DIR)/libavcodec/libavcodec.so $(STAGING_DIR)/usr/lib/libavcodec.so
	cp $(FFMPEG_TINY_DIR)/libavcodec/avcodec.h $(STAGING_DIR)/usr/include/ffmpeg/avcodec.h
	cp $(FFMPEG_TINY_DIR)/libavutil/libavutil.so $(STAGING_DIR)/usr/lib/libavutil.so
	cp $(FFMPEG_TINY_DIR)/libavutil/avutil.h $(STAGING_DIR)/usr/include/ffmpeg
	cp $(FFMPEG_TINY_DIR)/libavutil/common.h $(STAGING_DIR)/usr/include/ffmpeg
	cp $(FFMPEG_TINY_DIR)/libavutil/log.h $(STAGING_DIR)/usr/include/ffmpeg
	cp $(FFMPEG_TINY_DIR)/libavutil/rational.h $(STAGING_DIR)/usr/include/ffmpeg
	cp $(FFMPEG_TINY_DIR)/libavutil/mathematics.h $(STAGING_DIR)/usr/include/ffmpeg
	cp $(FFMPEG_TINY_DIR)/libavformat/avio.h $(STAGING_DIR)/usr/include/ffmpeg
	cp $(FFMPEG_TINY_DIR)/libavutil/integer.h $(STAGING_DIR)/usr/include/ffmpeg
	cp $(FFMPEG_TINY_DIR)/libavutil/intfloat_readwrite.h $(STAGING_DIR)/usr/include/ffmpeg
	cp $(FFMPEG_TINY_DIR)/libavutil/mem.h $(STAGING_DIR)/usr/include/ffmpeg

	cp $(FFMPEG_TINY_DIR)/libavcodec/libavcodec.so.51 $(TARGET_DIR)/usr/lib/
	rm -f $(TARGET_DIR)/usr/lib/libavcodec.so && ln -s libavcodec.so.51 $(TARGET_DIR)/usr/lib/libavcodec.so
	
	cp $(FFMPEG_TINY_DIR)/libavutil/libavutil.so.49 $(TARGET_DIR)/usr/lib/
	rm -f $(TARGET_DIR)/usr/lib/libavutil.so && ln -s libavutil.so.49 $(TARGET_DIR)/usr/lib/libavutil.so

	touch $@

ffmpeg_tiny: uclibc $(FFMPEG_TINY_DIR)/.installed

ffmpeg_tiny-clean:
	rm -f $(STAGING_DIR)/usr/liblibav*
	rm -f $(TARGET_DIR)/usr/lib/libav*
	rm -rf $(STAGING_DIR)/usr/include/ffmpeg
	-$(MAKE) -C $(FFMPEG_TINY_DIR) clean

ffmpeg_tiny-dirclean:
	rm -rf $(FFMPEG_TINY_DIR)

#############################################################
#
# Toplevel Makefile options
#
#############################################################
ifeq ($(strip $(BR2_PACKAGE_FFMPEG_TINY)),y)
TARGETS+=ffmpeg_tiny
endif
