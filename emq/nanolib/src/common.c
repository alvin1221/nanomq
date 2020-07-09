
/***
 *
 ***/

#include "libevpower.h"
#include <libevpower/common.h>
#include <libevpower/file.h>
#include <libevpower/cmd.h>
#include <libevpower/uci.h>
#include <libevpower/const_strings.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <net/if.h>
#include <stdarg.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <arpa/inet.h>

void log_append_to_file(char* filename, const char *format, ...)
{
	va_list arg;
	int done;

	FILE *pFile;
	pFile = fopen(filename, "a");
	if (pFile == 0) {
		return;
	}

	va_start (arg, format);
	time_t time_log = time(NULL);
	struct tm* tm_log = localtime(&time_log);
	fprintf(pFile, "%04d-%02d-%02d %02d:%02d:%02d ", tm_log->tm_year + 1900, tm_log->tm_mon + 1, tm_log->tm_mday, tm_log->tm_hour, tm_log->tm_min, tm_log->tm_sec);

	done = vfprintf (pFile, format, arg);
	va_end (arg);

	fflush(pFile);
	fclose(pFile);
}

void get_cur_time(char *dest)
{
	time_t t;
	struct tm * lt;
	time (&t);
	lt = localtime (&t);
	sprintf (dest, "%d-%02d-%02d %02d:%02d:%02d",lt->tm_year+1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
}

/**
 * 获取接口IP地址
 * @param ifname 接口名称
 * @param ipstr (out)接口地址字符串形式
 * @return 0或-1
 */
int get_if_ip(const char *ifname, char* ipstr)
{
	struct ifreq ifr;
	int fd = 0, ret = -1;

	if((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
		strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
		ifr.ifr_addr.sa_family = AF_INET;
		if (ioctl(fd, SIOCGIFADDR, &ifr) >= 0) {
			inet_ntop(AF_INET, &(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), ipstr, 16);
			ret = 0;
		}
		close(fd);
	}
	return ret;
}
/**
 * 获取指定接口的MAC地址
 * @param ifname 接口名称
 * @param ifhw (out)MAC地址,字符串形式, XX:XX:XX:XX:XX:XX
 */
int get_if_mac(const char* ifname, char* ifhw) {
	struct ifreq ifr;
	char *ptr;
	int skfd;

	if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		return -1;
	}

	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	ifr.ifr_addr.sa_family = AF_INET;

	if(ioctl(skfd, SIOCGIFHWADDR, &ifr) < 0) {
		close(skfd);
		return -1;
	}

	ptr = (char *)&ifr.ifr_addr.sa_data;
	sprintf(ifhw, "%02X:%02X:%02X:%02X:%02X:%02X",
			(ptr[0] & 0377), (ptr[1] & 0377), (ptr[2] & 0377),
			(ptr[3] & 0377), (ptr[4] & 0377), (ptr[5] & 0377));

	close(skfd);
	return 0;
}


/**
 * 获取接口子网掩码地址
 * @param ifname 接口名称
 * @param ipstr (out)接口地址字符串形式
 * @return 0或-1
 */
int get_if_netmask(const char* ifname, char* ipstr) {
	struct ifreq ifr;
	int fd = 0, ret = -1;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
		strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
		ifr.ifr_addr.sa_family = AF_INET;

		if (ioctl(fd, SIOCGIFNETMASK, &ifr) >= 0) {
			inet_ntop(AF_INET, &(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), ipstr, 16);
			ret = 0;
		}
		close(fd);
	}

	return ret;
}

int get_mem_info(unsigned long *total_mem, int *use_percent)
{
	unsigned long value;
	char tmp[4096];
	FILE* fp;

	if ((fp = fopen("/proc/meminfo", "r"))) {
		/*
			# cat /proc/meminfo
			MemTotal:         124716 kB
			MemFree:           78504 kB
			...
		 */
		fgets(tmp, sizeof(tmp), fp); // 第一行,总内存
		if (sscanf(tmp, "MemTotal: %ld kB", &value) == 1) {
			*total_mem = value * 1000;
		}

		fgets(tmp, sizeof (tmp), fp); // 第二行,可用内存
		if (sscanf(tmp, "MemFree: %ld kB", &value) == 1) {
			*use_percent = 100 - (value * 1000 * 100 / (*total_mem));
		}

		fclose(fp);
	}
	return 0;
}

int get_cpu_info(char *cpu_model)
{
	FILE* fp=NULL;
	char filename[32];
	char line_buffer[256];
	char *pchar;
	snprintf(filename, sizeof(filename), "/proc/cpuinfo");
    if((fp=fopen(filename, "r")) == NULL){
        return -1;
    }
    while(fgets(line_buffer, sizeof(line_buffer), fp))
    {
        line_buffer[strlen(line_buffer)-1]='\0';
        if((pchar=strstr(line_buffer, "machine"))!=NULL){
            pchar = strchr(line_buffer, ':');
            pchar+=2;
			if(strlen(pchar) < 32)
				strcpy(cpu_model, pchar);
        }
    }

    fclose(fp);
	return 0;
}
//获取开机时间，按照YYYYMMDDHHmmss
void get_boot_time(unsigned long on_time, char *dest)
{
	time_t t, t2;
	time (&t);
	t2 = t - on_time;
	struct tm * lt;
	lt = localtime (&t2);
	sprintf (dest, "%d%02d%02d%02d%02d%02d",lt->tm_year+1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
}
//是否开机60s
int is_up_120s()
{
	FILE *fp = NULL;
	char buff[256]={0};
	float run_times, free_times;

	fp = fopen("/proc/uptime", "r");
	if (fp == 0)
		return -1;
	fgets(buff, sizeof(buff), fp);
	if (sscanf(buff, "%f %f", &run_times, &free_times) == 2) {
		if(run_times < 120)
			return -1;
	}
	return 1;
}

/**
 * 执行指定的指令
 * @param cmd 指令
 * @return 执行成功0, 失败1
 */
int do_shell_cmd(const char *cmd) {
	int ret = system(cmd);

	// 取退出状态
	if (WIFEXITED(ret) && (WEXITSTATUS(ret) == 0)) {
		return 0;
	}

	return 1;
}

/**
 * 调用shell命令, 并获取命令结果
 * @param command 命令内容
 * @param result 目标容器, 调用者将容器长度不得低于512
 * @return 成功(结果长度), 失败0
 */
int get_shell_cmd(const char *command, char *result)
{
	FILE *fp;
	int length = 0;
	if ((fp = popen(command, "r"))) {
		if (fgets(result, 512, fp)) {	// 读取成功
			length = strlen(result);				// 至少读到一个换行符

			if (length < 2) {
				length = 0;
			}
			// 检查尾部是否有换行符
			else if (result[length-2] == '\r') {
				result[length-2] = 0;
				length -= 2;
			} else if (result[length-1] == '\n') {
				result[length-1] = 0;
				length -= 1;
			} else {
				result[length] = 0;
			}
		}
		pclose(fp);
	}
	return length;
}

int get_machine_id()
{
	int model;
	char *machine_id, *cmd, *pchar;
	machine_id = calloc(256, sizeof(char *));

	if(machine_id==NULL)
		return -1;
	get_cpu_info(machine_id);

	if((pchar=strstr(machine_id, "V3"))!=NULL){
		memset(machine_id,0,256);
		if(get_shell_cmd("machine-id", machine_id) > 0){
			/*
			if((pchar=strstr(machine_id, "i2c read failed"))!=NULL){
				ev_uci_save_val_string("i2c read failed", "evpower.general.machineid");
				model=XQY_UNKOWN;
			}else if((pchar=strstr(machine_id, "Fail to open /dev/i2c-0"))!=NULL){		//V3
				ev_uci_save_val_string("XQY_EG1000 V3", "evpower.general.machineid");
				model=XQY_EG1000V3;
			}else if(strlen(machine_id) == 14){		//V5
				ev_uci_save_val_string(machine_id, "evpower.general.machineid");
				model=XQY_EG1000V5;
			}else{
				ev_uci_save_val_string("Unkown", "evpower.general.machineid");
				model=XQY_UNKOWN;
			}*/
			if(strlen(machine_id) == 14){		//V5
				ev_uci_save_val_string(machine_id, "evpower.general.machineid");
				model=XQY_EG1000V5;
			}else{
				ev_uci_save_val_string("XQY_EG1000V3", "evpower.general.machineid");
				model=XQY_EG1000V3;
			}
		}else{
			ev_uci_save_val_string("XQY_EG1000V3", "evpower.general.machineid");
			model=XQY_EG1000V3;
		}
	}else if(strcmp(machine_id, "XQY_EG1000 board") == 0){	//EV4:	machine			: XQY_EG1000 board
		ev_uci_save_val_string(machine_id, "evpower.general.machineid");
		model=XQY_EG1000V1;
	} else {
		ev_uci_save_val_string("UNKOWN", "evpower.general.machineid");
		model = XQY_UNKOWN;
	}
	free(machine_id);
	return model;
}

int check_rtc_exists()
{
	if(1 == file_exists("/sys/class/rtc/rtc0/wdtimer")){
		ev_uci_save_val_string("1", "evpower.general.enable_rtc");
		return 1;
	}else{
		ev_uci_save_val_string("0", "evpower.general.enable_rtc");
		return 0;
	}
}

int check_power_on()
{
	return(cmd_run("[[ $(grep 'power_switch' /sys/kernel/debug/gpio | awk '{print $6}') == 'lo' ]]"));
}

void unlock_mount_sdcard()
{
	char lock_status[30]={0};
	if(ev_uci_data_get_val(lock_status, 30, "evpower.general.sdcard_lock") == 0){
		if(strcmp(lock_status, "off") != 0)
			ev_uci_save_val_string("off", "evpower.general.sdcard_lock");
	}
}

void lock_mount_sdcard(const char *type)
{
	char lock_status[30]={0};
	if(ev_uci_data_get_val(lock_status, 30, "evpower.general.sdcard_lock") == 0){
		if(strcmp(lock_status, type) != 0)
			ev_uci_save_val_string(type, "evpower.general.sdcard_lock");
	}
}

int kill_process_writing_sdcard()
{
	char dev[20];
	if(get_shell_cmd(cmd_get_sd_dev, dev) > 0)
	{
		if(0 == cmd_frun(cmd_is_mount, dev)){	//sd挂载时kill
			cmd_frun("kill `lsof -t /dev/%s`", dev);
			cmd_frun("kill `lsof -t /dev/%s`", dev);
		}
	}
	return 0;
}

/* return 0:unlock; 1:locked; -1:error*/
int get_sdcard_lock()
{
	int ret = -1;
	char lock_status[30]={0};
	if(ev_uci_data_get_val(lock_status, 30, "evpower.general.sdcard_lock") == 0){
		if(0 == strcmp(lock_status, "off"))
			ret = 0;
		else if(0 == strcmp(lock_status, "power_lock"))
			ret = 1;
		else if(0 == strcmp(lock_status, "reboot_lock"))
			ret = 2;
	}else{		//uci: Entry not found
		ret = -1;
		ev_uci_save_val_string("off", "evpower.general.sdcard_lock");
	}
	return ret;
}

//ro=1表示只读；ro=0表示可读写；ro=2表示未检测到tf卡
void set_sdcard_ro_rw(const char *ro)
{
	char status[10]={0};
	if(ev_uci_data_get_val(status, 10, "evpower.general.sdcard_ro") == 0){
		if(strcmp(status, ro) != 0)
			ev_uci_save_val_string(ro, "evpower.general.sdcard_ro");
	}else
		ev_uci_save_val_string("0", "evpower.general.sdcard_ro");
	return;
}
