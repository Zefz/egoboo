#ifndef _PROTO_H_
#define _PROTO_H_

#include "egobootypedef.h"

struct vec2_t;
struct vec3_t;
struct vec4_t;

struct Physics_Info;

struct Character;
struct Camera;
struct Mesh;
enum DAMAGE_TYPE;

namespace JF { class MPD_Mesh; };

void empty_import_directory(void);
void insert_space(int position);
void copy_one_line(int write);
int load_one_line(int read);
int load_parsed_line(int read);
void surround_space(int position);
void parse_null_terminate_comments();
int get_indentation();
void fix_operators();
int starts_with_capital_letter();
Uint32 get_high_bits();
int tell_code(int read);
void add_code(Uint32 highbits);
void parse_line_by_line();
Uint32 jump_goto(int index);
void parse_jumps(int ainumber);
void log_code(int ainumber, char* savename);
int ai_goto_colon(int read);
void get_code(int read);
void load_ai_codes(char* loadname);
int load_ai_script(char *loadname);
void reset_ai_script();
//void prime_titleimage();
//void release_all_titleimages();
void reset_sounds();
//void release_grfx( void );
//void release_map();
void release_module(void);
void close_session();
void general_error(int a, int b, char *szerrortext);
int play_sound(vec3_t & pos, Mix_Chunk *loadedwave);
void stop_sound(int whichchannel);
void play_music(int songnumber, int fadetime, int loops);
void make_newloadname(const char *modname, const char *appendname, char *newloadname);
void load_global_waves(char *modname);
void export_one_character(int character, int owner, int number);
void export_all_local_players(void);
void quit_module(void);
void quit_game(void);
void reset_tags();
int read_tag(FILE *fileread);
void read_all_tags(char *szFilename);
int tag_value(char *string);


void goto_colon(FILE* fileread, char * key = NULL);
bool goto_colon_yesno(FILE* fileread, char * key = NULL);

char get_first_letter(FILE* fileread);
bool   get_bool(FILE* fileread);
Sint32 get_int(FILE* fileread);
float  get_float(FILE* fileread);
IDSZ   get_idsz(FILE* fileread);
Uint8  get_team(FILE* fileread);
Uint32 get_damage_mods(FILE * fileread);
DAMAGE_TYPE get_damage_type(FILE* fileread);


bool   get_next_bool(FILE* fileread);
Sint32 get_next_int(FILE* fileread);
float  get_next_float(FILE* fileread);
IDSZ   get_next_idsz(FILE* fileread);
Uint32 get_next_damage_mods(FILE * fileread);
//DAMAGE_TYPE get_next_damage_type(FILE* fileread);



Uint32 display_message(int message, Uint16 character);

void load_action_names(char* loadname);
void get_name(FILE* fileread, char *szName);
void read_setup(char* filename);
void make_lighttospek(void);
void load_all_music_sounds();
void stop_music();

struct Passage;

bool open_passage(Passage & rpass);
int  break_passage(Passage & rpass, Uint16 starttile, Uint16 frames, Uint16 become, Uint8 meshfxor);
void flash_passage(Passage & rpass, Uint8 color);

//int close_passage(int passage);
void clear_passages();
//void add_shop_passage(int owner, int passage);
//void add_passage(int tlx, int tly, int brx, int bry, Uint8 open, Uint8 mask);
void flash_character_height(int character, Uint8 valuelow, Sint16 low,
                            Uint8 valuehigh, Sint16 high);
void flash_character(int character, Uint8 value);
void flash_select();
//void keep_weapons_with_holders();
//void make_prtlist(void);
void make_turntosin(void);

void play_particle_sound(int particle, Sint8 sound);
void free_one_character(int character);
void free_inventory(int character);
void attach_particle_to_character(int particle, int character, int grip);
void make_character_matrices();
int get_free_character();

struct Search_Params;
bool find_target_in_block(Search_Params &params, vec3_t & pos, float chrx, float chry, Uint16 facing,
                          bool onlyfriends, bool anyone, Uint8 team,
                          Uint16 donttarget, Uint16 oldtarget);

Uint16 find_target(Search_Params &params, vec3_t & pos, Uint16 facing,
                   Uint16 targetangle, bool onlyfriends, bool anyone,
                   Uint8 team, Uint16 donttarget, Uint16 oldtarget);


void disaffirm_attached_particles(Uint16 character, Uint32 pip_type);
Uint16 number_of_attached_particles(Uint16 character);
void reaffirm_attached_particles(Uint16 character);

void move_particles(Physics_Info & loc_phys, float deltaTime);
void attach_particles();
void free_all_characters();
void show_stat(Uint16 statindex);
void check_stats();
void check_screenshot();
bool dump_screenshot();
void add_stat(Uint16 character);
void move_to_top(Uint16 character);
void sort_stat();
float terp_dir(Uint16 majordir, float dx2, float dy2);

bool set_frame(Character & chr, Uint16 frame, Uint8 lip);
bool set_frame(Uint16 character, Uint16 frame, Uint8 lip);

void reset_character_alpha(Uint16 character);
void reset_character_accel(Uint16 character);
void detach_character_from_mount(Uint16 character, bool ignorekurse,
                                 bool doshop);
void spawn_bump_particles(Uint16 character, Uint16 particle);
int generate_number(int numbase, int numrand);
void drop_money(Uint16 character, Uint16 money);
void call_for_help(Uint16 character);
void give_experience(int character, int amount, Uint8 xptype);
void give_team_experience(Uint8 team, int amount, Uint8 xptype);
void damage_character(Uint16 character, Uint16 direction,
                      int damagebase, int damagerand, Uint8 damagetype, Uint8 team,
                      Uint16 attacker, Uint16 effects);
void kill_character(Uint16 character, Uint16 killer);
void spawn_poof(Uint16 character, Uint16 profile);
//void tilt_characters_to_terrain();

Uint16 change_armor(Uint16 character, Uint16 skin);
void change_character(Uint16 cnt, Uint16 profile, Uint8 skin,
                      Uint8 leavewhich);
Uint16 get_target_in_block(int x, int y, Uint16 character, bool items,
                           bool friends, bool enemies, bool dead, bool seeinvisible, IDSZ idsz,
                           bool excludeid);
Uint16 get_nearby_target(Uint16 character, bool items,
                         bool friends, bool enemies, bool dead, IDSZ idsz);
bool cost_mana(Uint16 character, int amount, Uint16 killer);
Uint16 find_distant_target(Uint16 character, int maxdistance);
void switch_team(int character, Uint8 team);
void get_nearest_in_block(int x, int y, Uint32 character, bool items,
                          bool friends, bool enemies, bool dead, bool seeinvisible, IDSZ idsz);
Uint32 get_nearest_target(Uint32 character, bool items,
                          bool friends, bool enemies, bool dead, IDSZ idsz);
Uint32 get_wide_target(Uint32 character, bool items,
                       bool friends, bool enemies, bool dead, IDSZ idsz, bool excludeid);
void issue_clean(Uint16 character);
int restock_ammo(Uint16 character, IDSZ idsz);
void issue_order(Uint16 character, Uint32 order);
void issue_special_order(Uint32 order, IDSZ idsz);
void set_alerts(int character);
int module_reference_matches(char *szLoadName, IDSZ idsz);
void add_module_idsz(char *szLoadName, IDSZ idsz);
void attach_character_to_mount(Uint16 character, Uint16 mount,
                               Uint16 grip);
void drop_keys(Uint32 character);
void drop_all_items(Uint16 character);
void character_grab_stuff(int chara, int grip, Uint8 people);
void character_swipe(Uint16 cnt, Uint8 grip);
void move_characters(Physics_Info & loc_phys, float dframe);
int add_player(Uint16 character, Uint16 player, Uint8 device);
void clear_messages();
void clear_select();
void add_select(Uint16 character);



//void read_mouse();
//void read_key();
//void read_joystick();
//void reset_press();
//void read_input();
void check_add(Uint8 key, char bigletter, char littleletter);
//void create_szfpstext(int frames);
void camera_look_at(vec2_t & pos);
//void project_view();
//void make_renderlist();
//void GCamera.make_matrix()();
//void figure_out_what_to_draw();
void bound_camera();
void bound_camera_track();
void set_one_player_latch(Uint16 player);
void read_local_latches(void);
void adjust_camera_angle(int height);
//void move_camera();
void make_onwhichfan(void);
void do_weather_spawn();
//void animate_tiles();
void stat_return();
void pit_kill();

int find_module(char *smallname);

void listen_for_packets();
void buffer_player_latches();
void unbuffer_player_latches();

void clear_orders();
Uint16 get_empty_order();
void chug_orders();
void resize_characters();
void update_game(float deltaTime);
void update_timers();
//void load_basic_textures(char *modname);
//Uint16 action_number();
//Uint16 action_frame();
void read_pair(FILE* fileread, Uint16 & pbase, Uint16 & prand);
void read_next_pair(FILE* fileread, Uint16 & pbase, Uint16 & prand);

void undo_pair(int base, int rand, float & from, float & to);
void ftruthf(FILE* filewrite, char* text, Uint8 truth);
void fdamagf(FILE* filewrite, char* text, Uint8 damagetype);
void factiof(FILE* filewrite, char* text, Uint8 action);
void fgendef(FILE* filewrite, char* text, Uint8 gender);
void fpairof(FILE* filewrite, char* text, int base, int rand);
void funderf(FILE* filewrite, char* text, char* usename);
void export_one_character_name(char *szSaveName, Uint16 character);
void export_one_character_profile(char *szSaveName, Uint16 character);
void export_one_character_skin(char *szSaveName, Uint16 character);
void get_message(FILE* fileread);
//void check_copy(char* loadname, int object);
//int load_one_object(int skin, char* tmploadname);
//void load_bars(char* szBitmap);
//void load_map(char* szModule, int sysmem);
//void load_font(char* szBitmap, char* szSpacing, int sysmem);
//void make_water();
//void read_wawalite(char *modname);
void reset_teams();
void reset_messages();
void make_randie();
void load_module(Camera & cam, Mesh & msh, char *smallname);
void render_prt();
//void render_shadow(int character);
//void render_bad_shadow(int character);
void render_refprt();
void render_fan(Mesh & msh, Uint32 fan, Uint32 texid = (Uint32)(-1));
void render_water_fan(Mesh & msh, Uint32 fan, Uint8 layer);
void render_enviromad(Uint16 character, Uint8 trans);
void render_texmad(Uint16 character, Uint8 trans);
void render_mad(Uint16 character, Uint8 trans);
void render_refmad(int tnc, Uint8 trans);
//void render_water();
//void draw_scene_sadreflection();
//void draw_scene_zreflection();
//bool get_mesh_memory();
//void draw_blip(Uint8 color, int x, int y);
//void draw_one_icon(int icontype, int x, int y, Uint8 sparkle);
//void draw_one_font(int fonttype, int x, int y);
//void draw_map(int x, int y);
//int draw_one_bar(int bartype, int x, int y, int ticks, int maxticks);
//void draw_string(char *szText, int x, int y);
//int length_of_word(char *szText);
//int draw_wrap_string(char *szText, int x, int y, int maxx);
//int draw_status(Uint16 character, int x, int y);
//void draw_text();
//void flip_pages();
//void draw_scene();
void send_rts_order(int x, int y, Uint8 order, Uint8 target);
//void build_select(float tlx, float tly, float brx, float bry, Uint8 team);
//Uint16 build_select_target(float tlx, float tly, float brx, float bry, Uint8 team);
//void move_rtsxy();
//void do_cursor_rts();
//void draw_main();

//int load_one_title_image(int titleimage, char *szLoadName);
int get_module_data(int modnumber, char *szLoadName);
int get_module_summary(char *szLoadName);
//void load_blip_bitmap();
//void draw_trimx(int x, int y, int length);
//void draw_trimy(int x, int y, int length);
//void draw_trim_box(int left, int top, int right, int bottom);
//void draw_trim_box_opening(int left, int top, int right, int bottom, float amount);
//void load_menu();
//void draw_titleimage(int image, int x, int y);
//void do_cursor();
//void menu_service_select();
//void menu_start_or_join();
void draw_module_tag(int module, int y);
int get_skin(char *filename);
void check_player_import(char *dirname);
//void menu_pick_player(int module);
//void menu_module_loading(int module);
//void menu_choose_host();
//void menu_choose_module();
//void menu_boot_players();
//void menu_end_text();
//void menu_initial_text();
//void fiddle_with_menu();
void release_menu_trim();
void release_menu();
void reset_timers();
void reset_camera();
//void sdlinit(int argc, char **argv);
//int glinit(int argc, char **argv);
void gltitle();
int DirGetAttrib(char *fromdir);

//---------------------------------------------------------------------------------------------
// Filesystem functions
void fs_init();
const char *fs_getTempDirectory();
const char *fs_getImportDirectory();
const char *fs_getGameDirectory();
const char *fs_getSaveDirectory();
int fs_fileIsDirectory(const char *filename);
int fs_createDirectory(const char *dirname);
int fs_removeDirectory(const char *dirname);
void fs_deleteFile(const char *filename);
void fs_copyFile(const char *source, const char *dest);
void fs_removeDirectoryAndContents(const char *dirname);
void fs_copyDirectory(const char *sourceDir, const char *destDir);

// Enumerate directory contents
const char *fs_findFirstFile(const char *path, const char *extension);
const char *fs_findNextFile(void);
void fs_findClose();

//---------------------------------------------------------------------------------------------
// Networking functions
void net_initialize();
void net_shutDown();
void net_logf(const char *format, ...);

void net_startNewPacket();

void packet_addUnsignedByte(Uint8 uc);
void packet_addSignedByte(Sint8 sc);
void packet_addUnsignedShort(Uint16 us);
void packet_addSignedShort(Sint16 ss);
void packet_addUnsignedInt(Uint32 ui);
void packet_addSignedInt(Sint32 si);
void packet_addString(char *string);

void net_sendPacketToHost();
void net_sendPacketToAllPlayers();
void net_sendPacketToHostGuaranteed();
void net_sendPacketToAllPlayersGuaranteed();
void net_sendPacketToOnePlayerGuaranteed(int player);
void input_net_message();

void net_updateFileTransfers();
int  net_pendingFileTransfers();

void net_copyFileToAllPlayers(char *source, char *dest);
void net_copyFileToHost(char *source, char *dest);
void net_copyDirectoryToHost(char *dirname, char *todirname);
void net_copyDirectoryToAllPlayers(char *dirname, char *todirname);
void net_sayHello();
void cl_talkToHost();
void sv_talkToRemotes();

int sv_hostGame();
int cl_joinGame(const char *hostname);

void find_open_sessions();
void sv_letPlayersJoin();
void stop_players_from_joining();
//int create_player(int host);
//void turn_on_service(int service);

//---------------------------------------------------------------------------------------------
// Platform specific functions

// Functions in this section are implemented separately on each platform. (Filesystem stuff
// could go here as well.)

void sys_initialize(); // Allow any setup necessary for platform specific code
void sys_shutdown(); // Allow any necessary cleanup for platform specific code

double sys_getTime(); // Return the current time, in seconds

#include "graphic.h"
#include "Mesh.h"

//void twist_to_normal(Uint8 twist, float * nrm_vec);
#endif //#ifndef _PROTO_H_
