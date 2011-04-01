#ifndef EXTAPP_MSG_H
#define EXTAPP_MSG_H

// needs to be keept in sync with avx6/packages/libarchos_support/src/extapp_msg.h

#define EXTAPP_MAGIC 			0xdeadbeaf

#define EXTAPP_PACKET_STARTUP_READY		1
#define EXTAPP_PACKET_EXIT_TO_HOME		2
#define EXTAPP_PACKET_BATTERY_STATUS		3
#define EXTAPP_PACKET_LANG_CHANGED		4
#define EXTAPP_PACKET_NETWORK_STATUS		5
#define EXTAPP_PACKET_TERMINATE			6
#define EXTAPP_PACKET_REQUEST_BATTERY_STATUS	7
#define EXTAPP_PACKET_REQUEST_NETWORK_STATUS	8
#define EXTAPP_PACKET_NO_POWEROFF		9
#define EXTAPP_PACKET_ALLOW_POWEROFF		10
#define EXTAPP_PACKET_BACKLIGHT_ON		11
#define EXTAPP_PACKET_OPEN_URL                  12
#define EXTAPP_PACKET_TIMEFORMAT_CHANGED	13
/* product key is a zero terminated string */
#define EXTAPP_PACKET_REQUEST_PRODUCTKEY	14
#define EXTAPP_PACKET_PRODUCTKEY		15
#define EXTAPP_PACKET_SET_VOLUME		16
#define EXTAPP_PACKET_GPS_PLUGGED		17
#define EXTAPP_PACKET_SET_BACKLIGHT		18
#define EXTAPP_PACKET_GPS_ONLY_PLUGGED		19

#define EXTAPP_PACKET_REQUEST_SOUND_STATUS	20
#define EXTAPP_PACKET_SOUND_STATUS	21



typedef struct {
	int magic;
	int packet_type;
	unsigned int data_length;
} extapp_msg_header_t;

#define EXTAPP_MSG_MAX_DATA_LENGTH 8192
#define EXTAPP_LANG_CODE_LENGTH 4

typedef enum { batt_lvl0 = 0, batt_lvl1, batt_lvl2, batt_lvl3, low_batt_warning = 98, batt_loading = 99 } extapp_msg_battery_t;
typedef enum { wifi_off = -1, wifi_disc = 0, wifi_con } extapp_msg_wifi_stat_t;
typedef enum { time24 = 0, time12 } extapp_msg_timeformat_t;
typedef enum { GPS_UNPLUGGED = 0, GPS_PLUGGED = 1 } extapp_msg_plugged_t;
typedef enum { EXTAPP_BACKLIGHT_LOW = 0, EXTAPP_BACKLIGHT_HIGH = 1 } extapp_msg_backlight_level_t;


typedef enum {
	net_none,
	net_ethernet_connected,
	net_ethernet_disconnected,
	net_wifi_connected,
	net_wifi_disconnected
} extapp_msg_network_t;

typedef struct {
	int speaker_activated;
	int volume;
	int mute;
} extapp_msg_sound_t;

typedef struct {
    extapp_msg_header_t header;
    void *data;
} extapp_msg_t;

#endif // EXTAPP_MSG_H
