
/***
 *
 * Copyright (C) 2014-2015 EV Power, Inc.
 * systematic functions and variables
 *
 ***/

#include "libevpower.h"

#include <stdlib.h>
#include <ctype.h>
#include <libevpower/cmd.h>
#include <libevpower/uci.h>
#include <libevpower/file.h>
#include <libevpower/system.h>
#include <libevpower/const_strings.h>

#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>
#include <linux/if_ether.h>
#include <linux/mii.h>
#include <linux/sockios.h>

//#define <TODO>_ENV

#if defined(TEST_ENV)
static const char *api_authorize_url = "iot-api-test.e-chong.com/ChargerAPI";
static const char *api_record_url = "iot-api-test.e-chong.com/ChargerAPI";
static const char *api_emqx_url = "183.232.157.134";
#elif defined(UAT_ENV)
static const char *api_authorize_url = "iot-api-uat.e-chong.com/ChargerAPI";
static const char *api_record_url = "iot-api-uat.e-chong.com/ChargerAPI";
static const char *api_emqx_url = "58.251.88.14";
#elif defined(DEV_ENV)
static const char *api_authorize_url = "dvpn.e-chong.com/ChargerAPI";
static const char *api_record_url = "dvpn.e-chong.com/ChargerAPI";
static const char *api_emqx_url = "mqtt.e-chong.com";
#else //FORMAL_ENV
static const char *api_authorize_url = "vpn.e-chong.com/ChargerAPI";
static const char *api_record_url = "vpn.e-chong.com/ChargerAPI";
static const char *api_emqx_url = "emqx.e-chong.com";
#endif

static const char *api_mqtt_url = "mqtt.e-chong.com";
static const char *ieee80211_dir = "/sys/class/ieee80211";

/* the 2 belowed is unused */
static const char *api_checkin_valid = "vpn.e-chong.com/ChargerAPI"; 	// "10.168.1.180:8080/ChargerAPI" vpn.e-chong.com/ChargerAPI
static const char *api_checkin_test = "iot-api-test.e-chong.com/ChargerAPI";	//  iot-api-test.e-chong.com/ChargerAPI


static struct ifreq ifr;

char sys_platform = HW_UNKNOWN;
char sys_orphan_mode = ORPHAN_OFF;
char sys_role = ROLE_REPEATER;

void sys_set_platform(void)
{
	int ret;
	sys_platform = HW_UNKNOWN;

	ret = ev_uci_check_val("POWER_BAR", uci_addr_status_hw);
	if (ret == 1) {
		sys_platform = POWER_BAR;
		return;
	}
}

const char *sys_authorize_api()
{
	return api_authorize_url;
}

const char *sys_record_api()
{
	return api_record_url;
}

const char *sys_mqtt_url()
{
	return api_mqtt_url;
}

const char *sys_emqx_url()
{
	return api_emqx_url;
}

const char *sys_checkin_url()
{
#ifdef TEST_ENV
	return api_checkin_test;
#else
	return api_checkin_valid;
#endif
}

unsigned int sys_get_ip_addr(const char *iface)
{
	int fd, ret;
	struct ifreq ifr;

	debug_msg("iface: %s", iface);

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
		goto out;

	memset(&ifr, 0, sizeof(struct ifreq));
        strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);

	ret = ioctl(fd, SIOCGIFADDR, &ifr);
	if (ret < 0) {
		debug_msg("could not get the IP of interface '%s': %s", iface,
			  strerror(errno));
		goto close_fd;
	}

	close(fd);
	return ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;

close_fd:
	close(fd);
out:
	return 0;
}

static void _sys_get_dfl_gw(char *iface, unsigned int *gw_addr)
{
	FILE *fd;
	unsigned long dest, gw;
	char iface_tmp[IFNAMSIZ];
	int ret, flags;

	*gw_addr = 0;
	memset(iface, 0, IFNAMSIZ);

	fd = fopen(path_net_route, "r");
	if (!fd)
		goto out;

	/* skip first line */
	if (fscanf(fd, "%*[^\n]\n") < 0)
		goto out;

	while (1) {
		ret = fscanf(fd, "%63s%lx%lx%X%*[^\n]\n", iface_tmp, &dest, &gw, &flags);
		if (ret <= 0)
			break;

		if (ret != 4)
			continue;

		if (!(flags & RTF_UP))
			continue;

		if (!(flags & RTF_GATEWAY))
			continue;

		if (dest != 0)
			continue;

		*gw_addr = (unsigned int)gw;
		strncpy(iface, iface_tmp, IFNAMSIZ);
		iface[IFNAMSIZ - 1] = '\0';
		debug_msg("found default route on iface '%s' to %d.%d.%d.%d",
			  iface, (*gw_addr >> 24) & 0xFF,
			  (*gw_addr >> 16) & 0xFF, (*gw_addr >> 8) & 0xFF,
			  *gw_addr & 0xFF);
		break;
	}

out:
	fclose(fd);
}

unsigned int sys_get_dfl_gw_addr(void)
{
	char iface[IFNAMSIZ];
	unsigned int gw_addr;

	_sys_get_dfl_gw(iface, &gw_addr);

	return gw_addr;
}

void sys_get_dfl_gw_iface(char *iface)
{
	unsigned int gw_addr;

	_sys_get_dfl_gw(iface, &gw_addr);
}

int str_is_in_range(const char *str, int x, int y)
{
	char *endptr;
	int val;

	debug_msg("str = %s, %i <= %s <= %i", str, x, str, y);

	errno = 0;
	val = strtol(str, &endptr, 10);
	if ((errno == ERANGE) || (errno != 0 && val == 0))
		return -1;

	/* string does not contain a number */
	if (str == endptr)
		return -1;

	if ((val < x) || (val > y))
		return -1;

	return val;
}

int str_is_mac_addr(const char *str)
{
	unsigned char mac_addr[6];
	int ret;

	ret = sscanf(str, MAC_FMT_SCAN, MAC_ARG_SCAN(mac_addr));

	if (ret == 6)
		return 1;

	return 0;
}

void str_replace_char(char *str, char candidate, char replacement)
{
	char *ptr;

	while (1) {
		ptr = strchr(str, candidate);

		if (!ptr)
			break;

		*ptr = replacement;
	}
}

void str_remove_char(char *str, char candidate)
{
	char *ptr;

	while (1) {
		ptr = strchr(str, candidate);

		if (!ptr)
			break;

		memmove(ptr, ptr + 1, strlen(ptr + 1) + 1);
	}
}

int sys_chan2freq(int channel)
{
	/* 2.4GHz */
	if (channel < 14)
		return 2407 + (channel * 5);
	if (channel == 14)
		return 2484;
	/* 5GHz */
	return (channel + 1000) * 5;
}

int sys_freq2chan(uint32_t freq)
{
	if (freq == 2484)
		return 14;

	if (freq < 2484)
		return (freq - 2407) / 5;

	if (freq < 45000)
		return freq / 5 - 1000;

	if (freq >= 58320 && freq <= 64800)
		return (freq - 56160) / 2160;

	return 0;
}

/**
 * sys_ssid_to_str - convert a ssid to a printable format (ssid may contain not
 *  printable chars)
 * @buf: where to store the printable ssid
 * @buf_len: length of buf
 * @ssid: the ssid to convert
 * @len: the length of the ssid
 * @escape: if true the '\' will be escaped and printed as '\\'
 */
void sys_ssid_to_str(char *buf, int buf_len, const uint8_t *ssid, uint8_t len,
		     bool escape)
{
	int i, r = 0;

	if (buf_len > 0)
		buf[0] = '\0';

	for (i = 0; i < len; i++) {
		if (isprint(ssid[i]) && ssid[i] != ' ' && ssid[i] != '\\') {
			r = snprintf(buf, buf_len, "%c", ssid[i]);
		} else if (ssid[i] == ' ' && (i != 0 && i != len -1)) {
			r = snprintf(buf, buf_len, " ");
		} else {
			r = snprintf(buf, buf_len, "%sx%.2x",
				     escape ? "\\\\" : "\\", ssid[i]);
		}
		buf_len -= r;
		buf += r;
	}
}

/**
 * sys_get_ssid - converts string SSID to the enum constant
 * @ssid_str: the string to convert
 */
enum ev_ssid sys_get_ssid(const char *ssid_str)
{
	enum ev_ssid ssid;

	if (strcmp(ssid_str, "none") == 0)
		return SSID_NONE;

	if (strcmp(ssid_str, "service") == 0)
		return SSID_SERVICE;

	/* the orphan interface always comes with a number at the end */
	if (strncmp(ssid_str, "orphan", strlen("orphan")) == 0)
		return SSID_ORPHAN;

	if (strcmp(ssid_str, "meship") == 0)
		return SSID_MESH;

	if (strncmp(ssid_str, "lan", 3) == 0)
		return SSID_LAN;

	if (strncmp(ssid_str, "ssid", 4) != 0)
		return SSID_UNKNOWN;

	ssid = strtol(ssid_str + 4, NULL, 10);

	if (ssid > SSID_MAX)
		return SSID_UNKNOWN;

	return ssid;
}

/**
 * sys_ssid_is_enabled - check if ssid is enabled
 * @ssid: the identifier of the ssid to check
 *
 * Returns true if the ssid is enabled, false otherwise
 */
bool sys_ssid_is_enabled(enum ev_ssid ssid)
{
	if (ssid == SSID_MESH)
		return true;

	return ev_uci_check_val("1", uci_addr_net_ssid_auto, ssid) == 1;
}

/**
 * sys_ssid_is_bridged - check if ssid is configured to be bridged into the LAN
 * @ssid: the identifier of the ssid to check
 *
 * Return: True if the ssid is bridged into the LAN, false otherwise
 */
bool sys_ssid_is_bridged(enum ev_ssid ssid)
{
	/* bridging with the LAN is available only for valid SSIDs */
	if (ssid < SSID_1)
		return false;

	if (ev_uci_check_val("1", uci_addr_vlan_ssid_lanbr, ssid) == 1)
		return true;

	return false;
}

/**
 * sys_ssid_has_vlan_tag - check if ssid has a vlan tag configured
 * @ssid: the identifier of the ssid to check
 *
 * Return: True if the ssid is vlan tagged, false otherwise
 */
bool sys_ssid_has_vlan_tag(enum ev_ssid ssid)
{
	char ssid_vlan[6];
	int ret;

	/* bridging with the LAN is available only for valid SSIDs */
	if (ssid < SSID_1)
		return false;

	ret = ev_uci_data_get_val(ssid_vlan, sizeof(ssid_vlan),
				  uci_addr_vlan_ssid_lanid, ssid);
	if (!ret && (strlen(ssid_vlan) > 0))
		return true;

	return false;
}

int sys_get_mac_addr(const char *iface, unsigned char *mac_buff)
{
	int fd, ret = -1;
	struct ifreq ifr;

	debug_msg("iface: %s", iface);

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
		goto out;

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, iface, IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ - 1] = '\0';

	ret = ioctl(fd, SIOCGIFHWADDR, &ifr);
	if (ret < 0) {
		debug_msg("Could not determine the MAC of interface '%s': %s\n",
			  iface, strerror(errno));
		goto close_fd;
	}

	memcpy(mac_buff, ifr.ifr_hwaddr.sa_data, 6);
	ret = 0;

close_fd:
	close(fd);
out:
	return ret;
}

char *sys_url_encode(const char *str)
{
	static const char conv[] = { '0', '1', '2', '3', '4', '5', '6', '7',
				     '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	size_t pos, res_pos;
	char *res;

	res = malloc(strlen(str) * 3 + 1);
	if (!res)
		return NULL;

	for (pos = 0, res_pos = 0; str[pos]; pos++, res_pos++) {
		if ((str[pos] == '-') || (str[pos] == '_') ||
		    (str[pos] == '.') || isdigit(str[pos]) ||
		    isupper(str[pos]) || islower(str[pos])) {
			res[res_pos] = str[pos];
		} else {
			res[res_pos] = '%';
			res_pos++;
			res[res_pos] = conv[(str[pos] >> 4) % sizeof(conv)];
			res_pos++;
			res[res_pos] = conv[str[pos] % sizeof(conv)];
		}
	}
	res[res_pos] = '\0';

	return res;
}

int sys_set_mac_addr(const char *iface, unsigned char *mac_buff)
{
	int fd, ret = -1;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
		goto out;

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, iface, IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ - 1] = '\0';
	ret = ioctl(fd, SIOCGIFHWADDR, &ifr);
	if (ret < 0) {
		debug_msg("could not set the MAC of interface '%s': %s",
			  iface, strerror(errno));
		goto close_fd;
	}

	memcpy(ifr.ifr_hwaddr.sa_data, mac_buff, ETH_ALEN);

	ret = ioctl(fd, SIOCSIFHWADDR, &ifr);
	if (ret < 0) {
		debug_msg("could not set the MAC of interface '%s': %s",
			  iface, strerror(errno));
		goto close_fd;
	}

	ret = 0;

close_fd:
	close(fd);
out:
	return ret;
}

bool sys_iface_exists(const char *ifname)
{
	/* if_nametoindex() returns 0 if the index cannot be retrieved */
	return if_nametoindex(ifname);
}

void sys_logger(const char *name, const char *fmt, ...)
{
	va_list args;
#if defined LIBEV_DEBUG
	char msg[512];
#endif

	openlog(name, LOG_PID, LOG_DAEMON | LOG_WARNING);

	va_start(args, fmt);
#if defined LIBEV_DEBUG
	/* print the message to standard debugging channels if DEBUG was
	 * enabled
	 */
	vsnprintf(msg, sizeof(msg), fmt, args);
	debug_msg("%s", msg);
#endif
	vsyslog(0, fmt, args);
	va_end(args);

	closelog();
}

#ifdef DEBUG_LOCK

static void ev_lock_print(pid_t pid, const char *fname, int line,
			  const char *func, const char *msg,
			  const char *lock_file)
{
	FILE *fp = fopen(LOCK_DEBUG, "a");

	if (!fp) {
		debug_msg("cannot open " LOCK_DEBUG);
		return;
	}

	fprintf(fp, "%d %s %s (%s:%d) %s %s\n", pid, ev_time_str(), func,
		fname, line, msg, lock_file);
	fflush(fp);
	fclose(fp);
}

#else

static void ev_lock_print(pid_t NG_UNUSED(pid), const char NG_UNUSED(*fname),
			  int NG_UNUSED(line), const char NG_UNUSED(*func),
			  const char NG_UNUSED(*msg),
			  const char NG_UNUSED(*lock_file))
{
}

#endif

void ev_lock_name(pid_t pid, const char *fname, int line, const char *func,
		  const char *fmt, const char *lock_name)
{
	char lock_file[50];

	snprintf(lock_file, sizeof(lock_file), fmt, lock_name);

	ev_lock_print(pid, fname, line, func, "locking", lock_file);

	cmd_frun(cmd_lock, lock_file);

	ev_lock_print(pid, fname, line, func, "locked", lock_file);
}

void ev_unlock_name(pid_t pid, const char *fname, int line, const char *func,
		    const char *fmt, const char *lock_name)
{
	char lock_file[50];

	snprintf(lock_file, sizeof(lock_file), fmt, lock_name);
	cmd_frun(cmd_unlock, lock_file);

	ev_lock_print(pid, fname, line, func, "unlocked", lock_file);
}

int sys_get_iface_def_table(const char *iface, char *table_buff,
			    int table_buff_len)
{
	char *hyphen_ptr;
	int ret = -1;

	hyphen_ptr = strchr(iface, '-');
	if (hyphen_ptr)
		iface = hyphen_ptr + 1;

	ret = ev_uci_data_get_val(table_buff, table_buff_len,
				  uci_addr_network_def_ip4table, iface);
	if (ret != 0)
		return ret;

	debug_msg("iface = %s, table = %s", iface, table_buff);

	return ret;
}

unsigned int sys_get_netmask(const char *iface)
{
	int fd, ret;

	debug_msg("iface: %s", iface);

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
		goto out;

	memset(&ifr, 0, sizeof(struct ifreq));
        strncpy(ifr.ifr_name, iface, IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ - 1] = '\0';

	ret = ioctl(fd, SIOCGIFNETMASK, &ifr);
	if (ret < 0) {
		debug_msg("could not get the netmask of interface '%s': %s",
			  iface, strerror(errno));
		goto close_fd;
	}

	close(fd);
	return ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;

close_fd:
	close(fd);
out:
	return 0;
}

int sys_get_primary_mac_addr(unsigned char *mac_buff)
{
	int ret = -1, fd, offset;

	sys_set_platform();

	switch (sys_platform) {
	case POWER_BAR:
		offset = 0;
		break;
	default:
		goto out;
	}

	fd = open("/dev/mtd7ro", O_RDONLY);
	if (fd < 0)
		goto out;

	lseek(fd, offset, SEEK_SET);
	ret = read(fd, mac_buff, ETH_ALEN);
	if (ret < 0)
		goto close;

	debug_msg("primary mac addr: " MAC_FMT_PRINT "",
		  MAC_ARG_PRINT(mac_buff));

close:
	close(fd);
out:
	return ret;
}

void sys_ssid_set_enabled(enum ev_ssid ssid, bool enabled)
{
	int ssid_vid;

	ssid_vid = MESH_VLAN_ID_BASE + ssid;

	ev_uci_save_val_int(enabled, uci_addr_net_ssid_auto, ssid);
	ev_uci_save_val_int(enabled, uci_addr_network_vlan_auto, "bat0",
			    ssid_vid);

	sys_set_platform();
	switch (sys_platform) {
	case POWER_BAR:
		ev_uci_save_val_int(!enabled, uci_addr_wifi_ap_disabled, 0,
				    ssid);
		break;
	default:
		break;
	}


}

int sys_mac2wiphy_idx(const char *mac)
{
	struct dirent *dir_entry;
	DIR *dir = NULL;
	char path_buff[300], mac_buff[18];
	int ret, index = -1;

	dir = opendir(ieee80211_dir);
	if (!dir) {
		debug_msg("Error - unable to open '%s': %s",
			  ieee80211_dir, strerror(errno));
		goto out;
	}

	while (1) {
		dir_entry = readdir(dir);
		if (!dir_entry)
			break;

		/* try to read the mac address file in this subdirectory */
		snprintf(path_buff, sizeof(path_buff), "%s/%s/macaddress",
			 ieee80211_dir, dir_entry->d_name);
		ret = file_read_string(path_buff, mac_buff, sizeof(mac_buff));
		if (ret <= 0)
			continue;

		if (strcasecmp(mac_buff, mac) != 0)
			continue;

		snprintf(path_buff, sizeof(path_buff), "%s/%s/index",
			 ieee80211_dir, dir_entry->d_name);
		index = file_read_int(path_buff);

		debug_msg("found index=%d for %s", index, mac);

		break;
	}

out:
	if (dir)
		closedir(dir);

	return index;
}

int sys_uci2wiphy_idx(int uci_radio_idx)
{
	char mac[18];
	int ret;

	ret = ev_uci_data_get_val(mac, sizeof(mac), "wireless.radio%d.macaddr",
				  uci_radio_idx);
	if (ret < 0) {
		debug_msg("can get mac addre from radio%d", uci_radio_idx);
		return -1;
	}

	return sys_mac2wiphy_idx(mac);
}

static void sys_bridge_fw_status_iface(char *name, void *uci_data,
				       void NG_UNUSED(*data))
{
	char *disabled, *type;
	int ret, ip_ret, ip6_ret;

	ret = ev_uci_data_get_val_ptr(uci_data, uci_val_disabled, &disabled);
	if ((ret == 1) && (disabled[0] == '1'))
		return;

	ret = ev_uci_data_get_val_ptr(uci_data, uci_val_type, &type);
	if (!ret || !type || (strcmp(type, uci_val_bridge) != 0))
		return;

	ip_ret = file_read_int(fmt_bridge_call_iptables, name);
	ip6_ret = file_read_int(fmt_bridge_call_ip6tables, name);

	printf("br-%s:\n", name);
	printf("\tIPv4: %s\n",
	       ip_ret == 1 ? uci_val_enabled : uci_val_disabled);
	printf("\tIPv6: %s\n",
	       ip6_ret == 1 ? uci_val_enabled : uci_val_disabled);
}

void sys_bridge_fw_status(void)
{
	ev_uci_config_foreach(uci_val_network, uci_val_interface,
			      &sys_bridge_fw_status_iface, NULL);
}

static void sys_bridge_fw_unset_iface(char *name, void *uci_data,
				      void NG_UNUSED(*data))
{
	char *type;
	int ret;

	ret = ev_uci_data_get_val_ptr(uci_data, uci_val_type, &type);
	if (!ret || !type || (strcmp(type, uci_val_bridge) != 0))
		return;

	file_write_int(0, fmt_bridge_call_iptables, name);
	file_write_int(0, fmt_bridge_call_ip6tables, name);
}

void sys_bridge_fw_unset(void)
{
	ev_uci_config_foreach(uci_val_network, uci_val_interface,
			      &sys_bridge_fw_unset_iface, NULL);
}

#define FILE_WRITE_INT(_val, _fmt, _args...) \
do { \
	char tmp[20]; \
	snprintf(tmp, sizeof(tmp), _fmt, _args); \
	if (file_write_int(_val, _fmt, _args) <= 0) \
		sys_logger(str_netcfgd, "Can't set %s to %d", tmp, _val); \
} while (0);

void sys_bridge_fw_set(void)
{
	char *lan, dfl_iface[IFNAMSIZ];
	bool layer7_enabled = true;
	bool splashpage_enabled = false;
	enum ev_ssid ssid;
	int ret;

	sys_bridge_fw_unset();

        if (ev_uci_check_val("1", uci_addr_udshape_l7matching_disabled) == 1)
		layer7_enabled = false;

	/* Since the splashpage functionality always depends on DNS it
	 * suffcies to check whether or not DNS redirect is enabled. */
	ev_for_each_ssid(ssid) {
		if (!sys_ssid_is_enabled(ssid))
			continue;

		if (ev_uci_check_val("1", uci_addr_net_ssid_dns_redirect, ssid) == 1)
			splashpage_enabled = true;
	}

	ret = ev_uci_check_val(uci_val_gateway, uci_addr_status_mode);
	/* repeater mode */
	if (ret != 1) {
		if (!layer7_enabled && !splashpage_enabled)
			return;

		/* repeaters always bridge with br-meship */
		FILE_WRITE_INT(1, fmt_bridge_call_iptables, uci_val_meship);
		FILE_WRITE_INT(1, fmt_bridge_call_ip6tables, uci_val_meship);
		// TODO: dns intercept might require bridge fw on br-ssidX
		return;
	}

	/* gateway mode */
	cmd_run(cmd_get_default_iface);
	if (cmd_output_len < 3) {
		sys_logger(str_netcfgd,
			   "No default route iface in sys_bridge_fw_set()");
		return;
	}
	strncpy(dfl_iface, cmd_output_buff, sizeof(dfl_iface));
	dfl_iface[sizeof(dfl_iface) - 1] = '\0';

	if (strncmp(dfl_iface, "br-", 3) != 0) {
		debug_msg("Bogus default route iface in sys_bridge_fw_set(): %s",
			  dfl_iface);
		return;
	}

	lan = dfl_iface + 3;

	ev_for_each_ssid(ssid) {
		/* splashpage is enabled per ssid */
		splashpage_enabled = false;

		if (!sys_ssid_is_enabled(ssid))
			continue;

		/* Since the splashpage functionality always depends on DNS it
		 * suffices to check whether or not DNS redirect is enabled
		 */
		if (ev_uci_check_val("1", uci_addr_net_ssid_dns_redirect,
				     ssid) == 1)
			splashpage_enabled = true;

		if (!layer7_enabled &&
		    !splashpage_enabled)
			continue;

		/* vlan tagged bridge */
		if (sys_ssid_has_vlan_tag(ssid)) {
			FILE_WRITE_INT(1, fmt_ssid_bridge_call_iptables, ssid);
			FILE_WRITE_INT(1, fmt_ssid_bridge_call_ip6tables, ssid);
		/* pure bridge mode */
		} else if (sys_ssid_is_bridged(ssid)) {
			FILE_WRITE_INT(1, fmt_bridge_call_iptables, lan);
			FILE_WRITE_INT(1, fmt_bridge_call_ip6tables, lan);
		/* routing */
		} else {
			/* With roaming vlans disabled all bridging is turned
			 * off (in routing mode)
			 */
			if (ev_uci_check_val("1",
					     uci_addr_status_vlan_allowed) < 1)
				continue;

			FILE_WRITE_INT(1, fmt_ssid_bridge_call_iptables, ssid);
			FILE_WRITE_INT(1, fmt_ssid_bridge_call_ip6tables, ssid);
		}
	}
}

#undef FILE_WRITE_INT
