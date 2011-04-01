FFMPEG_VERSION=
FFMPEG_SOURCE=ffmpeg.tar.gz
FFMPEG_CAT:=$(ZCAT)
FFMPEG_SITE=http://$(BR2_SOURCEFORGE_MIRROR).dl.sourceforge.net/sourceforge/netcat
FFMPEG_DIR:=$(BUILD_DIR)/ffmpeg
FFMPEG_BINARY=ffmpeg
FFMPEG_TARGET_BINARY=ffmpeg

$(DL_DIR)/$(FFMPEG_SOURCE):
	$(WGET) -P $(DL_DIR) $(FFMPEG_SITE)/$(FFMPEG_SOURCE)

ffmpeg-source: $(DL_DIR)/$(FFMPEG_SOURCE)

$(FFMPEG_DIR)/.unpacked: $(DL_DIR)/$(FFMPEG_SOURCE)
	$(FFMPEG_CAT) $(DL_DIR)/$(FFMPEG_SOURCE) | tar -C $(BUILD_DIR) $(TAR_OPTIONS) -
	toolchain/patch-kernel.sh $(FFMPEG_DIR) package/ffmpeg/ \*.patch
	touch $@

$(FFMPEG_DIR)/.configured: $(FFMPEG_DIR)/.unpacked
	(cd $(FFMPEG_DIR); rm -rf config.cache; \
		$(TARGET_CONFIGURE_OPTS) \
		$(TARGET_CONFIGURE_ARGS) \
		./configure \
		--arch=$(ARCH) \
		--cc=$(TARGET_CC) \
		--cross-compile \
		--prefix=/usr  \
		--mandir=/usr/share/man \
		--libdir=/usr/lib \
		--enable-gpl  \
		--enable-shared \
		--disable-ffmpeg --disable-ffplay --disable-ffserver --disable-vhook \
		--enable-libfaad --enable-libfaac \
	)
	#--enable-liba52 
	#--enable-libfaad
	#--enable-libfaac
	#--enable-libmp3lame 
	#--enable-libvorbis 
	#--enable-libxvid 
	touch $@

ffmpeg-compile: $(FFMPEG_DIR)/.configured
	$(MAKE) -C $(FFMPEG_DIR)

$(TARGET_DIR)/$(FFMPEG_TARGET_BINARY): $(FFMPEG_DIR)/$(FFMPEG_BINARY)
	#install -D $(FFMPEG_DIR)/$(FFMPEG_BINARY) $(TARGET_DIR)/$(FFMPEG_TARGET_BINARY)
	#$(STRIPCMD) $(STRIP_STRIP_ALL) $@


$(TARGET_DIR)/usr/lib/libavcodec.so.51: ffmpeg-compile
	cp $(FFMPEG_DIR)/libavcodec/libavcodec.so.51 $@
	rm -f $(TARGET_DIR)/usr/lib/libavcodec.so && ln -s $(@F) $(TARGET_DIR)/usr/lib/libavcodec.so

$(TARGET_DIR)/usr/lib/libavformat.so.51: ffmpeg-compile
	cp $(FFMPEG_DIR)/libavformat/libavformat.so.51 $@
	rm -f $(TARGET_DIR)/usr/lib/libavformat.so && ln -s $(@F) $(TARGET_DIR)/usr/lib/libavformat.so

$(TARGET_DIR)/usr/lib/libavutil.so.49: ffmpeg-compile
	cp $(FFMPEG_DIR)/libavutil/libavutil.so.49 $@
	rm -f $(TARGET_DIR)/usr/lib/libavutil.so && ln -s $(@F) $(TARGET_DIR)/usr/lib/libavutil.so

ffmpeg-install: $(TARGET_DIR)/usr/lib/libavcodec.so.51 $(TARGET_DIR)/usr/lib/libavformat.so.51 $(TARGET_DIR)/usr/lib/libavutil.so.49

ffmpeg: uclibc zlib libfaac libfaad2 ffmpeg-install
	mkdir -p $(STAGING_DIR)/usr/include/ffmpeg
	cp $(FFMPEG_DIR)/libavcodec/libavcodec.a $(STAGING_DIR)/usr/lib/libavcodec.a
	cp $(FFMPEG_DIR)/libavcodec/libavcodec.so $(STAGING_DIR)/usr/lib/libavcodec.so
	cp $(FFMPEG_DIR)/libavcodec/avcodec.h $(STAGING_DIR)/usr/include/ffmpeg/avcodec.h
	cp $(FFMPEG_DIR)/libavformat/libavformat.a $(STAGING_DIR)/usr/lib/libavformat.a
	cp $(FFMPEG_DIR)/libavformat/libavformat.so $(STAGING_DIR)/usr/lib/libavformat.so
	cp $(FFMPEG_DIR)/libavformat/avformat.h $(STAGING_DIR)/usr/include/ffmpeg/avformat.h
	cp $(FFMPEG_DIR)/libavutil/libavutil.a $(STAGING_DIR)/usr/lib/libavutil.a
	cp $(FFMPEG_DIR)/libavutil/libavutil.so $(STAGING_DIR)/usr/lib/libavutil.so
	cp $(FFMPEG_DIR)/libavutil/avutil.h $(STAGING_DIR)/usr/include/ffmpeg
	cp $(FFMPEG_DIR)/libavutil/common.h $(STAGING_DIR)/usr/include/ffmpeg
	cp $(FFMPEG_DIR)/libavutil/log.h $(STAGING_DIR)/usr/include/ffmpeg
	cp $(FFMPEG_DIR)/libavutil/rational.h $(STAGING_DIR)/usr/include/ffmpeg
	cp $(FFMPEG_DIR)/libavutil/mathematics.h $(STAGING_DIR)/usr/include/ffmpeg
	cp $(FFMPEG_DIR)/libavformat/avio.h $(STAGING_DIR)/usr/include/ffmpeg
	cp $(FFMPEG_DIR)/libavutil/integer.h $(STAGING_DIR)/usr/include/ffmpeg
	cp $(FFMPEG_DIR)/libavutil/intfloat_readwrite.h $(STAGING_DIR)/usr/include/ffmpeg
	cp $(FFMPEG_DIR)/libavutil/mem.h $(STAGING_DIR)/usr/include/ffmpeg

ffmpeg-clean:
	rm -f $(TARGET_DIR)/$(FFMPEG_TARGET_BINARY)
	-$(MAKE) -C $(FFMPEG_DIR) clean

ffmpeg-dirclean:
	rm -rf $(FFMPEG_DIR)
#############################################################
#
# Toplevel Makefile options
#
#############################################################
ifeq ($(strip $(BR2_PACKAGE_FFMPEG)),y)
TARGETS+=ffmpeg
endif
