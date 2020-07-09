
/***
 *
 *
 ***/

#ifndef __LIBEV_SYSTEM_H__
#define __LIBEV_SYSTEM_H__

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

enum {
	HW_UNKNOWN = 0,
	POWER_BAR,
	REGULAR
};

enum {
	ORPHAN_OFF = 0,
	ORPHAN_MASTER,
	ORPHAN_CLIENT,
};

enum {
	ROLE_REPEATER = 0,
	ROLE_GATEWAY,
	ROLE_ORPHAN,
};

enum ev_ssid {
	SSID_NONE = -6,
	SSID_UNKNOWN,
	SSID_LAN,
	SSID_SERVICE,
	SSID_ORPHAN,
	SSID_BATVLAN,
	/* MESH must be the one before the SSID_N group */
	SSID_MESH,
	/* SSIDs identifier are supposed to start at 1 */
	SSID_1,
	SSID_2,
	SSID_3,
	SSID_4,
	/* the following constant must always be the last */
	__SSID_LAST,
	SSID_MAX = __SSID_LAST - 1,
};

#define NG_FW_LEN_MAX	(3 /* $MAJ. */ + 3 /* $MIN. */ + \
			 3 /* $PATCH. */ + 8 /* -$SHORT_ID */)

#define BIT(x)	(1 << x)

#define DASH_SIG_DEBUG_FILE	"/tmp/dash_sig.debug"

#define ev_for_each_ssid(_ssid) for (_ssid = SSID_1; _ssid <= SSID_MAX; _ssid++)

#define SERVICE_IFACE_ID	"service"
#define TUNNEL_IFACE		"ng0"

#define MESH_VLAN_ID_BASE	10
#define MESH_VLAN_BR_OFFSET	20
#define SERVICE_VLAN_ID		989
#define ROAMING_VLAN_ID_BASE	990

/* the format to use to create the VIFs name */
#define VIF_SSID_FMT	"ap%d_%d"

/* mac address printing and scanning helpers */
#define _STRINGIFY(S)	#S
#define MAC_FMT_PRINT	_STRINGIFY(%.2hhX:%.2hhX:%.2hhX:%.2hhX:%.2hhX:%.2hhX)
#define MAC_FMT_SCAN	_STRINGIFY(%hhx:%hhx:%hhx:%hhx:%hhx:%hhx)
#define MAC_ARG_PRINT(_mac) _mac[0], _mac[1], _mac[2], _mac[3], _mac[4], _mac[5]
#define MAC_ARG_PRINT_OFF(_mac, _off) _mac[0], _mac[1], _mac[2], _mac[3], \
	_mac[4], (_mac[5] + _off)
#define MAC_ARG_SCAN(_mac) &_mac[0], &_mac[1], &_mac[2], &_mac[3], &_mac[4], &_mac[5]

#define UHTTPD_PORT_BASE	8080

#define CERTS_PATH		"/etc/ssl/certs/"

#define LOCK_DEBUG	"/tmp/lock-debug"
#define LOCK_FMT	"/var/lock/%s.lock"

#define ISOLA_MARK		0x40000000
#define ISOLA_MASK		0x40000000

#define FORMAL_ENV 1
//MQTT
#define MQTT_SERVER_PORT 1883
#define MQTT_INTERVAL 20

extern char sys_platform;
extern char sys_orphan_mode;
extern char sys_role;

void sys_set_platform(void);
unsigned int sys_get_ip_addr(const char *iface);
unsigned int sys_get_dfl_gw_addr(void);
void sys_get_dfl_gw_iface(char *iface);

int str_is_in_range(const char *str, int x, int y);
int str_is_mac_addr(const char *str);
void str_replace_char(char *str, char candidate, char replacement);
void str_remove_char(char *str, char candidate);

int sys_chan2freq(int channel);
int sys_freq2chan(uint32_t freq);
void sys_ssid_to_str(char *buff, int buff_len, const uint8_t *ssid, uint8_t len,
		     bool escape);

enum ev_ssid sys_get_ssid(const char *ssid_str);
void sys_ssid_set_enabled(enum ev_ssid ssid, bool enabled);
bool sys_ssid_is_enabled(enum ev_ssid ssid);
bool sys_ssid_is_bridged(enum ev_ssid ssid);
bool sys_ssid_has_vlan_tag(enum ev_ssid ssid);
int sys_get_mac_addr(const char *iface, unsigned char *mac_buff);
char *sys_url_encode(const char *str);

extern const char *sys_checkin_url();
extern const char *sys_mqtt_url();
extern const char *sys_emqx_url();
extern const char *sys_record_api();
extern const char *sys_authorize_api();

int sys_set_mac_addr(const char *iface, unsigned char *mac_buff);
bool sys_iface_exists(const char *ifname);
void sys_logger(const char *name, const char *fmt, ...);

unsigned int sys_get_netmask(const char *iface);
int sys_get_primary_mac_addr(unsigned char *mac_buff);

void ev_lock_name(pid_t pid, const char *fname, int line, const char *func,
		  const char *fmt, const char *lock_name);
void ev_unlock_name(pid_t pid, const char *fname, int line, const char *func,
		    const char *fmt, const char *lock_name);


#define ev_lock(_name) ev_lock_name(getpid(), __FILE__, __LINE__, __FUNCTION__,\
				    LOCK_FMT, _name)
#define ev_unlock(_name) ev_unlock_name(getpid(), __FILE__, __LINE__,\
					__FUNCTION__, LOCK_FMT, _name)

int sys_get_iface_def_table(const char *iface, char *table_buff,
			    int table_buff_len);
int sys_mac2wiphy_idx(const char *mac);
int sys_uci2wiphy_idx(int uci_radio_idx);
void sys_bridge_fw_status(void);
void sys_bridge_fw_unset(void);
void sys_bridge_fw_set(void);

#endif /* __LIBNG_SYSTEM_H__ */
