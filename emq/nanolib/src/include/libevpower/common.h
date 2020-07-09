#define		XQY_EG1000V5			1			//ev5----有machineid，有usb
#define 	XQY_EG1000V3			2			//ev5----无machineid，有usb
#define 	XQY_EG1000V1			3			//ev3,ev4----无machineid，无usb
#define 	XQY_UNKOWN				4

void log_append_to_file(char* filename, const char *format, ...);
void get_cur_time(char *dest);
int get_if_ip(const char *ifname, char* ipstr);
int get_if_mac(const char* ifname, char* ifhw);
int get_if_netmask(const char* ifname, char* ipstr);
int get_mem_info(unsigned long *total_mem, int *use_percent);
int get_cpu_info(char *cpu_model);
void get_boot_time(unsigned long on_time, char *dest);
int is_up_120s();
int get_shell_cmd(const char *command, char *result);
int get_machine_id();
int check_rtc_exists();
int check_power_on();
void unlock_mount_sdcard();
void lock_mount_sdcard(const char *type);
int kill_process_writing_sdcard();
int get_sdcard_lock();
void set_sdcard_ro_rw(const char *ro);
