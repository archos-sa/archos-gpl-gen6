################################################################################
#
# xdriver_xf86-input-dmc -- DMC input driver
#
################################################################################

XDRIVER_XF86_INPUT_DMC_VERSION = 1.1.0
XDRIVER_XF86_INPUT_DMC_SOURCE = xf86-input-dmc-$(XDRIVER_XF86_INPUT_DMC_VERSION).tar.bz2
XDRIVER_XF86_INPUT_DMC_SITE = http://xorg.freedesktop.org/releases/individual/driver
XDRIVER_XF86_INPUT_DMC_AUTORECONF = YES
XDRIVER_XF86_INPUT_DMC_DEPENDENCIES = xserver_xorg-server xproto_inputproto xproto_randrproto xproto_xproto

$(eval $(call AUTOTARGETS,package/x11r7,xdriver_xf86-input-dmc))
