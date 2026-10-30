#define main previous_ui_main
#include "clock_ui_host.c"
#undef main
static struct tm *test_gmtime(const time_t *t,struct tm *o){return gmtime_s(o,t)?NULL:o;}
#define gmtime_r test_gmtime
#include "../main/touch_port_ft3168.c"
static int64_t test_now=1000000;static bool test_down;static uint16_t test_x,test_y;
int64_t esp_timer_get_time(void){return test_now;}
esp_err_t board_349_v1_touch(bool*d,uint16_t*x,uint16_t*y){*d=test_down;*x=test_x;*y=test_y;return ESP_OK;}
void audio_port_prewarm(void){}
void imu_port_poll(int64_t t){}
bool imu_port_inverted(void){return false;}
bool imu_port_accel_sample(int16_t a[3],int64_t*t){return false;}
bool rtc_port_time_trusted(void){return true;}
bool rtc_port_set_utc(int64_t t){return true;}
static void poll(tank_t*t,bool down,int x,int y){test_now+=50000;test_down=down;test_x=x;test_y=y;touch_port_poll(t);}
static void fresh_touch(tank_t*t){healthy(t);memset(t->food,0,sizeof t->food);setup_cancel(t);notice_dismiss();memset(&g_ui349,0,sizeof g_ui349);g_ui349.controls_hidden=true;wp_camera_init(&g_ui349.world_camera);g_ui349.world_ready=true;touch_port_slot_changed();poll(t,false,0,0);}
static int food_count(tank_t*t){int n=0;for(int i=0;i<MAX_FOOD;i++)n+=t->food[i].alive;return n;}
int main(void){tank_t t;fresh_touch(&t);t.n_fish=0;
 poll(&t,true,160,150);poll(&t,false,160,150);
 printf("After physical release: drag_has_prev=%d drag_active=%d\n",t.drag_has_prev,t.drag_active);
 assert(!t.drag_has_prev&&!t.drag_active);
 poll(&t,true,450,150);poll(&t,false,450,150);assert(!t.trims&&!t.slash_engaged);
 fresh_touch(&t);t.n_fish=0;poll(&t,true,160,20);poll(&t,true,180,21);poll(&t,false,180,21);assert(!t.trims&&!t.drag_has_prev);assert(food_count(&t)>0);
 fresh_touch(&t);s_sel=0;g_ui349.view=(ui349_view_t){700,"FISH",true,0};poll(&t,true,300,90);assert(!t.drag_has_prev);poll(&t,false,300,90);
 s_sel=0;g_ui349.view=(ui349_view_t){700,"FISH",true,0};poll(&t,true,580,30);poll(&t,false,580,30);assert(s_sel==-1);
 fresh_touch(&t);t.n_fish=0;g_ui349.ambient.active=true;poll(&t,true,160,20);poll(&t,false,160,20);assert(!food_count(&t)&&!t.drag_has_prev);poll(&t,true,160,20);poll(&t,false,160,20);assert(food_count(&t)>0);
 fresh_touch(&t);t.n_fish=0;for(int b=0;b<tank_veg_beds(&t);b++)tank_veg_set(&t,b,1);
 for(int x=100;x<=520;x+=20)poll(&t,true,x,150);poll(&t,false,520,150);assert(t.trims>0);
 puts("PASS deliberate continuous plant-cutting gesture remains available");
 puts("PASS real touch adapter: separate contacts without simulation ticks, feeding jitter, fish page isolation/exit, screensaver wake then feed");return 0;}
