#include <stdio.h>
#include <stdint.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <sys/limits.h>
#include <sys/poll.h>
#include <linux/input.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/stat.h> 
#include <sys/types.h>  
#include <string.h>  
#include <cutils/log.h>

#define DEBUG_PRINT_EVENT 0

#if DEBUG_PRINT_EVENT
#define LABEL(constant) { #constant, constant }
#define LABEL_END { NULL, -1 }
struct label {
    const char *name;
    int value;
};
static struct label key_value_labels[] = {
        { "UP", 0 },
        { "DOWN", 1 },
        { "REPEAT", 2 },
        LABEL_END,
};
#include "input.h-labels.h"
#undef LABEL
#undef LABEL_END
#endif

#define STR_LEN 50
#define POINT_MAX 10
#define FILE_1M (1024*1000) 
#define FILE_SIZE (FILE_1M*2)
#define FILE_DIR "/data/zslogs/touchser"
#define FILE_PATH "/data/zslogs/touchser/touchrawdata" 
#define FILE_PATH_1 "/data/zslogs/touchser/touchrawdata1"
#define PAD_DEVICES_NAME "touch_frame_input"
#define test_bit(bit, array)    (array[bit/8] & (1<<(bit%8)))

#define ZS_LOG(fmt, ...)\
     do\
     {\
         printf("[%s]:"fmt"\n", __FUNCTION__, ##__VA_ARGS__);\
         ALOGE("[%s]:"fmt"\n", __FUNCTION__, ##__VA_ARGS__);\
     }while(0);

#define set_slot(x, sum)  \
		do{\
			sum = (((uint64_t)(x))<<40) | ((uint64_t)(sum) & ~((uint64_t)0xf<<40));\
		}while(0)
#define set_time(x, sum)  \
		do{\
			sum = (((uint64_t)(x))<<24) | ((uint64_t)(sum) & ~((uint64_t)0xffff<<24));\
		}while(0)
#define set_id(x, sum) \
		do{\
			sum = (((uint64_t)(x))<<3) | ((uint64_t)(sum) & ~((uint64_t)0x1<<23));\
		}while(0)		
#define set_x(x, sum) \
		do{\
			sum = (((uint64_t)(x))<<12) | ((uint64_t)(sum) & ~((uint64_t)0x7ff<<12));\
		}while(0)
#define set_type(x, sum)  \
		do{\
			sum = (((uint64_t)(x))<<11) | ((uint64_t)(sum) & ~((uint64_t)0x1<<11));\
		}while(0)
#define set_y(x, sum)  \
		do{\
			sum = ((uint64_t)(x)) | ((uint64_t)(sum) & ~((uint64_t)0x7ff));\
		}while(0)
#define get_slot(sum)  \
			(unsigned int)(((sum)>>40) & 0xf)
#define get_time(sum)  \
			(unsigned int)(((sum)>>24) & 0xffff)
#define get_id(sum)  \
			(unsigned int)(((sum)>>23) & 0x1)
#define get_x(sum)  \
			(unsigned int)(((sum)>>12) & 0x7ff)
#define get_type(sum)  \
			(unsigned int)(((sum)>>11) & 0x1)
#define get_y(sum)  \
			(unsigned int)( (sum) & 0x7ff)
struct pointer {
	unsigned int slot:4;		/*bit 40-43 0xf-15*/
	unsigned int time:16;		/*bit 24-39 0xffff-65535(65s)*/
	unsigned int device_id:1;	/*bit 23  0-VA 1-pad*/
	unsigned int x:11;			/*bit 12-22 0x7ff-2047*/
	unsigned int type:1;		/*bit 11 1-down 0-up*/
	unsigned int y:11;			/*bit 0-10 0x7ff-2047*/
} __attribute__((__packed__));


#define set_year(x, sum)  \
		do{\
			sum = ((x)<<20) | ((sum) & ~(0x7ffU<<20));\
		}while(0)
#define set_month(x, sum)  \
		do{\
			sum = ((x)<<16) | ((sum) & ~(0xfU<<16));\
		}while(0)
#define set_day(x, sum)  \
		do{\
			sum = ((x)<<11) | ((sum) & ~(0x1fU<<11));\
		}while(0)
#define set_hour(x, sum)  \
		do{\
			sum = ((x)<<6) | ((sum) & ~(0x1fU<<6));\
		}while(0)
#define set_min(x, sum)  \
		do{\
			sum = ((x)) | ((sum) & ~(0x3f));\
		}while(0)
#define get_year(sum)  \
			(unsigned int)((sum>>20) & 0x7ff)
#define get_month(sum)  \
			(unsigned int)((sum>>16) & 0xf)
#define get_day(sum)  \
			(unsigned int)((sum>>11) & 0x1f)
#define get_hour(sum)  \
			(unsigned int)((sum>>6) & 0x1f)
#define get_min(sum)  \
			(unsigned int)( sum & 0x3f)
struct zs_time {
	unsigned int year:11;		/*bit 20-31 0x7ff-2047*/
	unsigned int month:4;		/*bit 16-19 0xf-15 12*/
	unsigned int day:5;			/*bit 11-15 0x1f-31 31*/
	unsigned int hour:5; 		/*bit 6-10 0x1f-31 24*/
	unsigned int min:6; 		/*bit 0-5 0x3f-63 60*/
} __attribute__((__packed__));

typedef enum {
	true=1, 
	false=0
}bool;

typedef enum {
	finger_up=0,
	finger_down=1 
}finger_status;

/*global variable*/
static struct pollfd *ufds;
static char **device_names;
static int nfds;
char sum_str[200];
bool device_id = 0;

int file_write(const char* file, const char* buf)
{
	int fd;
	struct stat file_stat;  

	if ((fd= open(FILE_PATH, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR)) == -1){
		ZS_LOG("open %s fail\n",FILE_PATH);	
	}
	if (stat(file, &file_stat)<0){ 
		ZS_LOG("stat %s fail\n",FILE_PATH);	
	}
	if(file_stat.st_size > FILE_SIZE){
		close(fd);
		rename(FILE_PATH, FILE_PATH_1);
		if ((fd= open(FILE_PATH, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR)) == -1){
			ZS_LOG("open %s fail\n",FILE_PATH);	
		}
	}
	lseek(fd, 0, SEEK_END);
	write(fd, buf, strlen(buf));

	close(fd);
	return 0;
}

/*
int close_device(const char *device)
{
    int i;
    for(i = 1; i < nfds; i++) {
        if(strcmp(device_names[i], device) == 0) {
            int count = nfds - i - 1;
            free(device_names[i]);
            memmove(device_names + i, device_names + i + 1, sizeof(device_names[0]) * count);
            memmove(ufds + i, ufds + i + 1, sizeof(ufds[0]) * count);
            nfds--;
            return 0;
        }
    }
    return -1;
}
*/

static int touch_props(int fd)
{
	uint8_t bits[INPUT_PROP_CNT / 8];
	int i, j;
	int res;
	const char *bit_label;
	uint8_t keyBitmask[(KEY_MAX + 1) / 8];

	res = ioctl(fd, EVIOCGPROP(sizeof(bits)), bits);
	if(res < 0) {
		return 1;
	}
	res = ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBitmask)), keyBitmask);
	if(res < 0) {
		return 1;
	}
	for(i = 0; i < res; i++) {
		for(j = 0; j < 8; j++) {
			if (bits[i] & 1 << j) {
				if (((i*8+j == INPUT_PROP_DIRECT) || (i*8+j == INPUT_PROP_POINTER) )&&
						test_bit(BTN_TOUCH, keyBitmask)){
					return 0;
				}
			}
		}
	}
	return 1;
}

static int open_device(const char *device)
{
    int fd;
    struct pollfd *new_ufds;
    char **new_device_names;
    char name[80];

    fd = open(device, O_RDWR);
    if(fd < 0) {
        return -1;
    }

	if(touch_props(fd)){
		//close
		return -1;	
	}
 
    name[sizeof(name) - 1] = '\0';
    if(ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
        //fprintf(stderr, "could not get device name for %s, %s\n", device, strerror(errno));
        name[0] = '\0';
    }
    new_ufds = realloc(ufds, sizeof(ufds[0]) * (nfds + 1));
    if(new_ufds == NULL) {
        ZS_LOG("out of memory\n");
        return -1;
    }
    ufds = new_ufds;
    new_device_names = realloc(device_names, sizeof(device_names[0]) * (nfds + 1));
    if(new_device_names == NULL) {
        ZS_LOG("out of memory\n");
        return -1;
    }
    device_names = new_device_names;

    ZS_LOG("name: %s\n", name);

    ufds[nfds].fd = fd;
    ufds[nfds].events = POLLIN;
    device_names[nfds] = strdup(name);
    nfds++;

    return 0;
}

static int scan_dir(const char *dirname)
{
    char devname[PATH_MAX];
    char *filename;
    DIR *dir;
    struct dirent *de;
    dir = opendir(dirname);
    if(dir == NULL)
        return -1;
    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';
    while((de = readdir(dir))) {
        if(de->d_name[0] == '.' &&
           (de->d_name[1] == '\0' ||
            (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;
        strcpy(filename, de->d_name);
        open_device(devname);
    }
    closedir(dir);
    return 0;
}

#if DEBUG_PRINT_EVENT
static const char *get_label(const struct label *labels, int value)
{
    while(labels->name && value != labels->value) {
        labels++;
    }
    return labels->name;
}
#endif

static bool print_event(int type, int code, int value)
{
	static int slot;
	static bool xy_changed[POINT_MAX] = {0};
	static uint64_t save_sum[POINT_MAX] = {0};
	static unsigned int save_time;
	int i = 0;
	bool print_sum = false;
	struct tm *p;
	struct timeval    tv;
	struct timezone   tz;
	static int pre_min;
	static int pre_hour;
	char str_buf_temp[50] = {0};
#if DEBUG_PRINT_EVENT
	char str_buf[100] = {0};
	const char *type_label, *code_label, *value_label;
#endif

	switch (type){
	case EV_SYN:
		if(code != SYN_REPORT) {
			break;
		}
		memset(sum_str, 0, sizeof(sum_str));
		for (i=0; i< POINT_MAX; i++){
			if (xy_changed[i]){
				print_sum=true;
				memset(str_buf_temp, 0, sizeof(str_buf_temp));
				sprintf(str_buf_temp ,"%-11.11llx\n", save_sum[i]);	
				strncat(sum_str, str_buf_temp, STR_LEN);
				xy_changed[i] = false;
			}
		}
		break;
	case EV_ABS:
		switch (code) {
		case ABS_MT_SLOT:
			slot = value;
			break;
		case ABS_MT_POSITION_X:
			xy_changed[slot] = true;
			set_x(value, save_sum[slot]);
			set_slot(slot, save_sum[slot]);
			set_type(finger_down , save_sum[slot]);
			set_id(device_id, save_sum[slot]);

			gettimeofday(&tv, &tz); 
			p = localtime(&tv.tv_sec); 
			if (pre_min != p->tm_min || pre_hour != p->tm_hour){
				pre_min = p->tm_min;
				pre_hour = p->tm_hour;
				set_year(1900+p->tm_year, save_time);
				set_month(1+p->tm_mon, save_time);
				set_day(p->tm_mday, save_time);
				set_hour(p->tm_hour, save_time);
				set_min(p->tm_min, save_time);	
//				printf("%05x\n", save_time);
				memset(str_buf_temp, 0, sizeof(str_buf_temp));
				sprintf(str_buf_temp,"%5x\n", save_time);
				file_write(FILE_PATH, str_buf_temp);
			}
			set_time((p->tm_sec*1000+(tv.tv_usec+500)/1000), save_sum[slot]);
			break;
		case ABS_MT_POSITION_Y:
			xy_changed[slot] = true;
			set_y(value, save_sum[slot]);
			set_slot(slot, save_sum[slot]);
			set_type(finger_down , save_sum[slot]);
			set_id(device_id, save_sum[slot]);

			gettimeofday(&tv, &tz); 
			p = localtime(&tv.tv_sec); 
			if (pre_min != p->tm_min || pre_hour != p->tm_hour){
				pre_min = p->tm_min;
				pre_hour = p->tm_hour;
				set_year(1900+p->tm_year, save_time);
				set_month(1+p->tm_mon, save_time);
				set_day(p->tm_mday, save_time);
				set_hour(p->tm_hour, save_time);
				set_min(p->tm_min, save_time);	
//				printf("%05x\n", save_time);
				memset(str_buf_temp, 0, sizeof(str_buf_temp));
				sprintf(str_buf_temp,"%5x\n", save_time);
				file_write(FILE_PATH, str_buf_temp);
			}
			set_time((p->tm_sec*1000+(tv.tv_usec+500)/1000), save_sum[slot]);
			break;
		case ABS_MT_PRESSURE:
			break;
		case ABS_MT_TRACKING_ID:
			if (value < 0){
				xy_changed[slot] = true;
				set_type(finger_up, save_sum[slot]);
				set_id(device_id, save_sum[slot]);

				gettimeofday(&tv, &tz); 
				p = localtime(&tv.tv_sec); 
				if (pre_min != p->tm_min || pre_hour != p->tm_hour){
					pre_min = p->tm_min;
					pre_hour = p->tm_hour;
					set_year(1900+p->tm_year, save_time);
					set_month(1+p->tm_mon, save_time);
					set_day(p->tm_mday, save_time);
					set_hour(p->tm_hour, save_time);
					set_min(p->tm_min, save_time);	
//					printf("%05x\n", save_time);
					memset(str_buf_temp, 0, sizeof(str_buf_temp));
					sprintf(str_buf_temp,"%5x\n", save_time);
					file_write(FILE_PATH, str_buf_temp);
				}
				set_time((p->tm_sec*1000+(tv.tv_usec+500)/1000), save_sum[slot]);
			}
			break;	
		}
		break;
	case EV_KEY:
		break;
	defaults:
		break;	
	}
#if DEBUG_PRINT_EVENT
	gettimeofday(&tv, &tz); 
	p = localtime(&tv.tv_sec); 
	memset(str_buf, 0, sizeof(str_buf));

	memset(str_buf_temp, 0, sizeof(str_buf_temp));
	sprintf(str_buf_temp, "%s", device_id?"touchpad: ":"touchscreen: ");
	strncat(str_buf, str_buf_temp, STR_LEN);

	memset(str_buf_temp, 0, sizeof(str_buf_temp));
	sprintf(str_buf_temp, "[%02d.%02d.%-3.3ld]",
			p->tm_min, p->tm_sec, (tv.tv_usec+500)/1000);
	strncat(str_buf, str_buf_temp, STR_LEN);

	type_label = get_label(ev_labels, type);
	code_label = NULL;
	value_label = NULL;

	switch(type) {
	case EV_SYN:
		code_label = get_label(syn_labels, code);
		break;
	case EV_KEY:
		code_label = get_label(key_labels, code);
		value_label = get_label(key_value_labels, value);
		break;
	case EV_REL:
		code_label = get_label(rel_labels, code);
		break;
	case EV_ABS:
		code_label = get_label(abs_labels, code);
		switch(code) {
		case ABS_MT_TOOL_TYPE:
			value_label = get_label(mt_tool_labels, value);
		}
		break;
	case EV_MSC:
		code_label = get_label(msc_labels, code);
		break;
	case EV_LED:
		code_label = get_label(led_labels, code);
		break;
	case EV_SND:
		code_label = get_label(snd_labels, code);
		break;
	case EV_SW:
		code_label = get_label(sw_labels, code);
		break;
	case EV_REP:
		code_label = get_label(rep_labels, code);
		break;
	case EV_FF:
		code_label = get_label(ff_labels, code);
		break;
	case EV_FF_STATUS:
		code_label = get_label(ff_status_labels, code);
		break;
	}

	if (type_label) {
		memset(str_buf_temp, 0, sizeof(str_buf_temp));
		sprintf(str_buf_temp, "%s", type_label);
		strncat(str_buf, str_buf_temp, STR_LEN);
	} else {
		memset(str_buf_temp, 0, sizeof(str_buf_temp));
		sprintf(str_buf_temp,"-%4x-", type);
		strncat(str_buf, str_buf_temp, STR_LEN);
	}
	if (code_label) {
		memset(str_buf_temp, 0, sizeof(str_buf_temp));
		if (code == ABS_MT_POSITION_X){
			sprintf(str_buf_temp,"-[x:");
		} else if (code == ABS_MT_POSITION_Y) {
			sprintf(str_buf_temp,"-[y:");	
		} else {
			sprintf(str_buf_temp,"-[%s %04x]-", code_label, code);
		}
		strncat(str_buf, str_buf_temp, STR_LEN);
	} else {
		memset(str_buf_temp, 0, sizeof(str_buf_temp));
		sprintf(str_buf_temp,"-%4x-", code);
		strncat(str_buf, str_buf_temp, STR_LEN);
	}
	if (value_label) {
		memset(str_buf_temp, 0, sizeof(str_buf_temp));
		sprintf(str_buf_temp,"%s", value_label);
		strncat(str_buf, str_buf_temp, STR_LEN);
	} else {
		memset(str_buf_temp, 0, sizeof(str_buf_temp));
		if(code == ABS_MT_POSITION_X || code == ABS_MT_POSITION_Y) {
			sprintf(str_buf_temp,"%d]-", value);
		} else {
			sprintf(str_buf_temp,"-[%d]-", value);
		}
		strncat(str_buf, str_buf_temp, STR_LEN);
	}
	strncat(str_buf, "\n", 2);
	printf("%s", str_buf);
#endif
	return print_sum; 
}

static int read_notify(const char *dirname, int nfd)
{
    int res;
    char devname[PATH_MAX];
    char *filename;
    char event_buf[512];
    int event_size;
    int event_pos = 0;
    struct inotify_event *event;

    res = read(nfd, event_buf, sizeof(event_buf));
    if(res < (int)sizeof(*event)) {
        if(errno == EINTR)
            return 0;
        ZS_LOG("could not get event, %s\n", strerror(errno));
        return 1;
    }

    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';

    while(res >= (int)sizeof(*event)) {
        event = (struct inotify_event *)(event_buf + event_pos);
        if(event->len) {
            strcpy(filename, event->name);
            if(event->mask & IN_CREATE) {
                open_device(devname);
            } /*else {
                close_device(devname);
            }*/
        }
        event_size = sizeof(*event) + event->len;
        res -= event_size;
        event_pos += event_size;
    }
    return 0;
}

int main(void)
{
	int i;
	int res;
	struct input_event event;
	const char *device_path = "/dev/input";

	if (-1 == access(FILE_DIR, F_OK)){
		if (mkdir(FILE_DIR, 0664)) {
			if (mkdir(FILE_DIR, 0664)) {
				ZS_LOG("touchser: mkdir dir error");
                return 1;
            }
		}
	}

	nfds = 1;
	ufds = calloc(1, sizeof(ufds[0]));
	ufds[0].fd = inotify_init();
	ufds[0].events = POLLIN;

	res = scan_dir(device_path);
	if (res < 0) {
        ZS_LOG("touchser: scan dir error");
		return 1;
	}

    ZS_LOG("touchser: start");
	while(1) {
		poll(ufds, nfds, -1);
		if (ufds[0].revents & POLLIN) {
			read_notify(device_path, ufds[0].fd);
		}
		for(i = 1; i < nfds; i++) {
			if(ufds[i].revents) {
				if(ufds[i].revents & POLLIN) {
					res = read(ufds[i].fd, &event, sizeof(event));
					if(res < (int)sizeof(event)) {
						ZS_LOG("could not get event\n");
                        continue;
					}
					if(event.code == ABS_MT_PRESSURE) {
						continue;	
					}
					if (!strcmp(device_names[i], PAD_DEVICES_NAME)){
						device_id = 1;//touchpad
					} else {
						device_id = 0;//touchscreen
					}
					if(print_event(event.type, event.code, event.value)){
						#if DEBUG_PRINT_EVENT
						printf("\n");
						#endif
						file_write(FILE_PATH, sum_str);
					}
				}
			}
		}
	}
    ZS_LOG("touchser: end");
	return 0;
}
