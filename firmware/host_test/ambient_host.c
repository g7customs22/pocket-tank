#define main existing_clock_tests
#include "clock_ui_host.c"
#undef main
int main(void){
 ambient_t a;ambient_defaults(&a);assert(a.config.timeout==3&&!strcmp(a.config.zip,"46219"));
 ambient_idle(&a,299999999,0,true);assert(!a.active);ambient_idle(&a,300000000,0,true);assert(a.active);
 assert(ambient_contact(&a,true)&&!a.active);assert(ambient_contact(&a,true));assert(ambient_contact(&a,false));assert(!ambient_contact(&a,false));
 ambient_idle(&a,600000000,300000000,false);assert(!a.active);a.config.timeout=0;ambient_idle(&a,999999999,0,true);assert(!a.active);
 ambient_action(&a,AMBIENT_OPEN);ambient_action(&a,AMBIENT_DIGIT+4);ambient_action(&a,AMBIENT_PLUS);assert(!strcmp(a.draft.zip,"46210"));ambient_action(&a,AMBIENT_CANCEL);assert(!strcmp(a.config.zip,"46219")&&!a.pending);
 ambient_action(&a,AMBIENT_OPEN);ambient_action(&a,AMBIENT_TIMEOUT);ambient_action(&a,AMBIENT_SAVE);assert(a.pending&&a.config.timeout==0&&a.draft.timeout==1);
 ambient_config_t bad=a.config;bad.zip[5]='x';assert(!ambient_config_valid(&bad));bad=a.config;bad.zip[0]='a';assert(!ambient_config_valid(&bad));
 tank_t t;healthy(&t);uint16_t *src=calloc(TANK_W*TANK_H,2),*dst=calloc(640*172,2);assert(src&&dst);
 ui349_state_t u={0};ambient_defaults(&u.ambient);u.world_ready=true;wp_camera_init(&u.world_camera);u.controls_hidden=true;
 u.ambient.active=true;u.ambient.clock_valid=true;u.ambient.hour=9;u.ambient.minute=41;u.ambient.pm=true;u.ambient.weather_valid=true;u.ambient.temperature=68;u.ambient.code=2;strcpy(u.ambient.place,"Indianapolis");
 ui349_render(&u,&t,src,dst);ppm("ambient-preview.ppm",dst);
 u.ambient.active=false;u.view.key=200;ambient_action(&u.ambient,AMBIENT_OPEN);ui349_render(&u,&t,src,dst);ppm("ambient-settings.ppm",dst);
 bool down;int x,y;assert(ui349_input(&u,true,300,120,&down,&x,&y)==UI349_CONSUMED&&!down);assert(ui349_input(&u,false,300,120,&down,&x,&y)==AMBIENT_MINUS&&!down);
 u.ambient.active=true;u.ambient.editor=false;u.view.key=0;u.ambient.weather_valid=false;u.ambient.clock_valid=false;ui349_render(&u,&t,src,dst);ppm("ambient-offline.ppm",dst);
 u.ambient.editor=true;ui349_configure(&u,(ui349_view_t){1000,"RESET",false,0});assert(!u.ambient.editor&&!u.ambient.active);
 u.view.key=200;u.clock_picker=true;assert(ui349_input(&u,true,40,100,&down,&x,&y)==UI349_CONSUMED);assert(ui349_input(&u,false,40,100,&down,&x,&y)!=AMBIENT_OPEN);
 free(src);free(dst);puts("PASS timeout boundary/off/modal, entire wake-contact consumption, ZIP/cancel/save validation, settings routing and render previews");return 0;
}
