#include <stdio.h>
#include <stdlib.h>
#include <libftdi1/ftdi.h>
#include <unistd.h>
#include <time.h>
#include "sema.c"
#define REPORT_INTERVAL 5
#define DMX_FREQUENCY 43000
#define DMX_BAUDRATE 250000
#define DMX_OPEN_INTERVAL 5
struct DmxOutput {
  struct ftdi_context *ftdi;
  struct ftdi_version_info version;
  unsigned int ftdi_chipid;
  int state;
  unsigned char universe[513];
  int universe_count;
  int last_universe_count;
  time_t last_report;
  time_t last_open;
};
static struct rk_sema timer_sem;	

void dmx_print_ftdi_version(struct DmxOutput *dmx) {
  dmx->version = ftdi_get_library_version();
  printf("Initialized libftdi %s (major: %d, minor: %d, micro: %d, snapshot ver: %s)\n",
    dmx->version.version_str, dmx->version.major, dmx->version.minor, dmx->version.micro,
    dmx->version.snapshot_str);
}

int dmx_open(struct DmxOutput *dmx) {
  int ret;
  dmx->last_open = time(NULL);
  if ((ret = ftdi_usb_open(dmx->ftdi, 0x0403, 0x6001)) < 0) {
    fprintf(stderr, "unable to open ftdi device: %d (%s)\n", ret,
      ftdi_get_error_string(dmx->ftdi));
    return EXIT_FAILURE;
  }
  if ((ret = ftdi_set_baudrate(dmx->ftdi, DMX_BAUDRATE)) < 0) {
    fprintf(stderr, "unable to set baudrate on ftdi device: %d (%s)\n", ret,
      ftdi_get_error_string(dmx->ftdi));
    return EXIT_FAILURE;
  }

  if ((ret = ftdi_set_line_property(dmx->ftdi, BITS_8, STOP_BIT_2, NONE)) < 0) {
    fprintf(stderr, "unable to set line properties on ftdi device: %d (%s)\n", ret,
      ftdi_get_error_string(dmx->ftdi));
    return EXIT_FAILURE;
  }
  dmx->state += 1;
  return 0;
}


void dmx_reset(struct DmxOutput *dmx)  {
  ftdi_usb_close(dmx->ftdi);
  dmx->state = 1;
}

void dmx_write_universe(struct DmxOutput *dmx){
  int ret = ftdi_write_data(dmx->ftdi, dmx->universe, 513);
  if (ret != 513) {
    printf("Writing universe failed, resetting device\n");
    dmx_reset(dmx);
  }
  ((struct DmxOutput*)dmx)->universe_count += 1;
}


void dmx_cleanup(struct DmxOutput *dmx) {
  int ret;
  if (dmx->state > 1) {
    if ((ret = ftdi_usb_close(dmx->ftdi)) < 0) {
      fprintf(stderr, "unable to close ftdi device: %d (%s)\n", ret, ftdi_get_error_string(dmx->ftdi));
    }
  }
  if (dmx->state > 0) {
    ftdi_free(dmx->ftdi);
  }
  printf("Ftdi device closed.\n");
   free(dmx);
}

void dmx_report(struct DmxOutput *dmx) {
  if (dmx->last_report == 0) {
    dmx->last_report = time(NULL);
  }
  double elapsed = (double)(time(NULL) - dmx->last_report);
  if (((int)elapsed) > REPORT_INTERVAL) {
    float universes = (float)(dmx->universe_count - dmx->last_universe_count);
    printf("Writing %.2f universes per second\n", universes / elapsed);
    dmx->last_universe_count = dmx->universe_count;
    dmx->last_report = time(NULL);
  }
}


struct DmxOutput* dmx_new() {
  struct DmxOutput *dmx = malloc(sizeof(struct DmxOutput));
  if (dmx==NULL) return NULL;
  dmx->state = 0;
  if ((dmx->ftdi = ftdi_new()) == 0) {
    fprintf(stderr, "ftdi_new failed\n");
    free(dmx);
    return NULL;
  }
  dmx->state += 1;
  dmx_print_ftdi_version(dmx);
  if (dmx_open(dmx) != 0){
    return dmx;
  }
  
  if (dmx->ftdi->type == TYPE_R) {
      ftdi_read_chipid(dmx->ftdi, &(dmx->ftdi_chipid));
      printf("FTDI chipid: %X\n", dmx->ftdi_chipid);
  }
  return dmx;
}

int dmx_check(struct DmxOutput* dmx) {
  if (dmx->state == 1 && dmx->last_open + DMX_OPEN_INTERVAL < time(NULL)) {
    dmx_open(dmx);
  }
  return dmx->state - 2;
}

static void timersignalhandler()  {
	rk_sem_post(&timer_sem);
}


void set_periodic_timer(long delay) {
	rk_sem_init(&timer_sem, 0);
	signal(SIGALRM, timersignalhandler);
	struct itimerval tval = { 
		/* subsequent firings */ .it_interval = { .tv_sec = 0, .tv_usec = delay }, 
		/* first firing */       .it_value = { .tv_sec = 0, .tv_usec = delay }};

	setitimer(ITIMER_REAL, &tval, (struct itimerval*)0);
}

int main(void) {
  struct DmxOutput* dmx = NULL;
  printf("**********************  DMX Master 1.0 **********************\n");
  if ((dmx = dmx_new()) == 0) {
    printf("Failed to initialize dmx device");
    return EXIT_FAILURE;
  }
  printf("Timer init\n");
	set_periodic_timer(DMX_FREQUENCY);
  dmx->universe[1] = 255;
  while (TRUE) {
    if (dmx_check(dmx) == 0) {
      dmx_write_universe(dmx);
      dmx_report(dmx);
    }
    rk_sem_wait(&timer_sem);
  }
  dmx_cleanup(dmx);
  return EXIT_SUCCESS;
}