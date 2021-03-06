// To-do: Fix code formatting
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "display.h"
#define NEW(x) malloc(sizeof(x))
#define BETW(x,min,max) (min<x&&x<max)
#define WIDTH 80
#define HEIGHT 24
#define AREA (WIDTH*HEIGHT)
#define TUNNELS (AREA/240)
#define CHECKER(x) (x%2^(x/WIDTH%2))
// bool type definition
typedef enum {false,true} bool;
// Tile type definition
typedef struct tile_t {
	char fg,bg,*fg_c,*bg_c;
} tile_t;
// Function prototypes
void print_help();
void draw_tile(tile_t tile);
void draw_pos(tile_t *zone,int pos);
void draw_board(tile_t *zone);
bool char_in_string(char c,char *string);
int dir_offset(char dir);
void set_fg (tile_t *tile,char fg,char *fg_c);
void set_bg (tile_t *tile,char bg,char *bg_c);
void set_tile (tile_t *tile,char fg,char *fg_c,char bg,char *bg_c);
tile_t *random_tile(tile_t *zone);
tile_t *random_grass(tile_t *zone);
tile_t *random_floor(tile_t *zone);
int abs(int n);
char move_tile(tile_t *zone,int pos,char dir);
void update (tile_t *zone);
bool try_summon(tile_t *tile,char fg,char *fg_c);
void spawn_player(tile_t *zone,int *pc);
char move_player(tile_t *zone,char dir,int *pc);
void set_wall(tile_t *tile,int pos);
void set_floor(tile_t *tile,int pos);
void set_door(tile_t *tile);
void make_building(tile_t *zone,int pos,int h,int w);
void make_random_building(tile_t *zone);
void cull_walls(tile_t *zone);
void create_field(tile_t *field,int b,int m,int a,int s);
void create_dungeon(tile_t *dungeon,int b,int m);
int dist_to_wall(tile_t *zone,int pos,char dir);
bool make_path(tile_t *zone,int pos);
// Global definitions
static char
	*blood="\e[1;37;41m",
    	*grass_colors[2]={TERM_COLORS_40M[GREEN],TERM_COLORS_40M[LGREEN]},
	*floor_colors[2]={TERM_COLORS_40M[WHITE],TERM_COLORS_40M[LGRAY]},
	*wall_colors[2]={TERM_COLORS_40M[RED],TERM_COLORS_40M[LRED]},
	*rock_colors[2]={TERM_COLORS_40M[GRAY],TERM_COLORS_40M[LGRAY]},
	*grass_chars="\"\',.`",
	*creatures="@&A$Z";
static FILE *debug_log;
// Main function
int main(int argc,char **argv)
{
	// Open the debug log
	debug_log=fopen("debug.log","w+");
	// Seed the RNG
	srand(time(NULL));
	// Set terminal attributes
	set_terminal_canon(false);
	set_cursor_visibility(0);
	// Variable definitions
	tile_t *field=malloc(AREA*sizeof(tile_t))
		,*dungeon=malloc(AREA*sizeof(tile_t))
		,*c_z;
	int p_c;
	// Parse command line arguments
	int buildings=20,monsters=20,animals=20,soldiers=20; // Defaults
	switch (argc)
	{
		case 5:
			sscanf(argv[4],"%i",&soldiers);
		case 4:
			sscanf(argv[3],"%i",&animals);
		case 3:
			sscanf(argv[2],"%i",&monsters);
		case 2:
			sscanf(argv[1],"%i",&buildings);
	}
	// Initialize field
	create_field(field,buildings,monsters,animals,soldiers);
	c_z=field;
	// Initialize player
	spawn_player(c_z,&p_c);
	// Draw board
	clear_screen();
	draw_board(c_z);
	// Print help prompt
	move_cursor(0,HEIGHT-1);
	printf("%sPress ? now for help.",RESET_COLOR);
	if (fgetc(stdin)=='?')
		print_help();
	draw_board(c_z);
	// Control loop
	char input;
	bool has_scepter=false;
	for (;;) {
		input=fgetc(stdin);
		if (input==27&&fgetc(stdin)==91)
			input=fgetc(stdin);
		//fprintf(debug_log,"Input registered: \'%c\'\n",input);
		if (input=='q')
			break;
		if (input=='>'&&c_z[p_c].bg=='>'&&c_z[p_c].fg=='@') {
			fprintf(debug_log,"Entering dungeon!\n");
			char *old_color=c_z[p_c].fg_c;
			set_fg(&c_z[p_c],'\0',NULL);
			create_dungeon(dungeon,buildings,monsters);
			c_z=dungeon;
			for (p_c=0;c_z[p_c].bg!='<';p_c++);
			set_fg(&c_z[p_c],'@',old_color);
			draw_board(c_z);
			continue;
		} else if (input=='<'&&c_z[p_c].bg=='<'&&c_z[p_c].fg=='@') {
			fprintf(debug_log,"Exiting dungeon!\n");
			char *old_color=c_z[p_c].fg_c;
			for (p_c=0;field[p_c].bg!='>';p_c++);
			set_fg(&field[p_c],'@',old_color);
			draw_board(field);
			c_z=field;
			continue;
		} else if (input=='z'&&has_scepter) {
			fprintf(debug_log,"Summoning zombie!\n");
			int target=p_c+dir_offset(fgetc(stdin));
			try_summon(&c_z[target],'Z',TERM_COLORS_40M[TEAL]);
			draw_pos(c_z,target);
			update(c_z);
			continue;
		} else if (input=='Z'&&has_scepter) {
			update(c_z);
			for (int i=1;i<=9;i++) {
				try_summon(&c_z[p_c+dir_offset(i+'0')]
						,'Z',TERM_COLORS_40M[TEAL]);
				draw_pos(c_z,p_c+dir_offset(i+'0'));
			}
			continue;
		} else if (input=='o'&&has_scepter) {
			input=fgetc(stdin);
			fprintf(debug_log,"Opening portal!\n");
			int target=p_c+2*dir_offset(input);
			if (!c_z[p_c+dir_offset(input)].fg) {
				try_summon(&c_z[target],'O',TERM_COLORS_40M[PURPLE]);
				draw_pos(c_z,target);
			}
			update(c_z);
			continue;
		} else if (input=='R'&&has_scepter
				&&c_z[p_c].fg!='@') {
			fprintf(debug_log,"Resurrecting player!\n");
			set_fg(&c_z[p_c],'@',TERM_COLORS_40M[LBLUE]);
			has_scepter=false;
			draw_pos(c_z,p_c);
		} else if (input=='S') {
			clear_screen();
			draw_board(c_z);
			continue;
		}
		switch (move_player(c_z,input,&p_c)) {
			case 'I':
				has_scepter=true;
				c_z[p_c].fg_c=TERM_COLORS_40M[PURPLE];
				draw_pos(c_z,p_c);
				break;
			case 'O':
				fprintf(debug_log,"Entering portal!\n");
				char *old_color=c_z[p_c].fg_c;
				create_field(field,buildings,monsters,
						animals,soldiers);
				fprintf(debug_log,"Spawning player...\n");
				spawn_player(field,&p_c);
				set_fg(&field[p_c],'@',old_color);
				draw_board(field);
				c_z=field;
				fprintf(debug_log,"Done!\n");
				continue;

		}
		update(c_z);
	}
	// Clean up terminal
	set_terminal_canon(true);
	printf("%s",RESET_COLOR);
	set_cursor_visibility(1);
	move_cursor(0,HEIGHT);
	// Exit (success)
	return 0;
}
// Function definitions
void print_help()
{
	clear_screen();
	move_cursor(0,0);
	printf("%s",RESET_COLOR);
	printf("\
Mogue is a very simple turn-based roguelike.\n\
Creatures kill one another by moving on top of each other.\n\
\n\
h/j/k/l/y/u/b/n, arrow keys, or numpad 1-9 for movement.\n\
> and < enter and exit dungeons respectively when on stairs.\n\
\n\
Use Shift-S to redraw the game if you resize the window.\n\
\n\
When you have the scepter from the dungeon:\n\
	z+(direction) summons a zombie\n\
	o+(direction) opens a portal\n\
	Shift-Z summons zombies all around you\n\
	Shift-R resurrects you, destroying your scepter\n\
\n\
Press any key to continue.");
	fgetc(stdin);
	clear_screen();
	return;
}
void draw_tile(tile_t tile)
{
	printf("%s%c",tile.fg?tile.fg_c:tile.bg_c
			,tile.fg?tile.fg:tile.bg);
}
void draw_pos(tile_t *zone,int pos)
{
	move_cursor(pos%WIDTH,pos/WIDTH);
	draw_tile(zone[pos]);
}
void draw_board(tile_t *zone)
{
	for (int i=0;i<AREA;i++)
		draw_pos(zone,i);
}
bool char_in_string(char c,char *string)
{
	for (;*string;string++)
		if (c==*string)
			return true;
	return false;
}
int dir_offset(char dir)
{
	int offset=0;
	if (char_in_string(dir,"kyuA789")) // North
		offset-=WIDTH;
	else if (char_in_string(dir,"jbnB123")) // South
		offset+=WIDTH;
	if (char_in_string(dir,"hybD147")) // West
		offset-=1;
	else if (char_in_string(dir,"lunC369")) // East
		offset+=1;
	return offset;
}
void set_fg (tile_t *tile,char fg,char *fg_c)
{
	tile->fg=fg;
	tile->fg_c=fg_c;
}
void set_bg (tile_t *tile,char bg,char *bg_c)
{
	tile->bg=bg;
	tile->bg_c=bg_c;
}
void set_tile (tile_t *tile,char fg,char *fg_c,char bg,char *bg_c)
{
	set_fg(tile,fg,fg_c);
	set_bg(tile,bg,bg_c);
}
tile_t *random_tile(tile_t *zone)
{
	return &zone[rand()%AREA];
}
tile_t *random_empty_tile(tile_t *zone)
{
	tile_t *tile=random_tile(zone);
	if (!tile->fg)
		return tile;
	return random_empty_tile(zone);
}
tile_t *random_grass(tile_t *zone)
{
	tile_t *tile=random_empty_tile(zone);
	if (char_in_string(tile->bg,grass_chars))
		return tile;
	return (random_grass(zone));
}
tile_t *random_floor(tile_t *zone)
{
	tile_t *tile=random_empty_tile(zone);
	if (tile->bg=='#')
		return tile;
	return (random_floor(zone));
}
int abs(int n)
{
	return n<0?-n:n;
}
char move_tile(tile_t *zone,int pos,char dir)
{
	int dest=pos+dir_offset(dir);
	if (abs(dest%WIDTH-pos%WIDTH)==WIDTH-1||0>dest||dest>AREA-1)
		return '\0';
	tile_t *from=&zone[pos],*to=&zone[dest];
	// Note: Describes only cases where movement is disallowed
	switch (from->fg) {
		case '@': // Player
			if (char_in_string(to->fg,"%$"))
				return '\0';
			break;
		case '&': // Monster
			if (char_in_string(to->fg,"%&"))
				return '\0';
			break;
		case '$': // Soldier
			if (char_in_string(to->fg,"%$@A"))
				return '\0';
			break;
		case 'A': // Animal
			if (char_in_string(to->fg,"%@$&A+"))
				return '\0';
			break;
		case 'Z': // Zombie
			if (char_in_string(to->fg,"%Z@+"))
				return '\0';
			break;
		default:
			return '\0';
	}
	// If the destination is not the source
	if (dest!=pos) {
		char killed='\''; // (Represents grass)
		// If the destination is a door
		if (to->fg=='+') {
			// Open it, not moving the creature
			set_fg(to,'\0',NULL);
			draw_pos(zone,dest);
			// Report failure to move
			return '\0';
		}
		// If there's something else at the destination:
		if (to->fg) {
			killed=to->fg; // Remember what was captured
			fprintf(debug_log,"%c moved into %c at [%i]\n"
					,from->fg,to->fg,dest);
			// If it's a creature not on stairs, place a corpse
			if (char_in_string(to->fg,creatures)
					&&!char_in_string(to->bg,"<>"))
				set_bg(to,to->fg,blood);
		}
		// Move the symbol and color
		set_fg(to,from->fg,from->fg_c);
		set_fg(from,'\0',NULL);
		// Redraw the changed positions
		draw_pos(zone,pos);
		draw_pos(zone,dest);
		// Return the value of what was captured
		return killed;
	} else
		return '\0';
}
void update (tile_t *zone)
{
	// Make a matrix of the foreground characters
	char fgcopies[AREA],dir;
	for (int i=0;i<AREA;i++)
		fgcopies[i]=zone[i].fg;
	// For each occupied space, if it hasn't moved yet, move it
	for (int i=0;i<AREA;i++)
		if (fgcopies[i]&&fgcopies[i]==zone[i].fg&&zone[i].fg!='@')
			// Act based on collision
			switch (move_tile(zone,i,dir=rand()%9+'1')) {
				case 'I':
					zone[i+dir_offset(dir)].fg_c=TERM_COLORS_40M[PURPLE];
					draw_pos(zone,i+dir_offset(dir));
					break;
				case 'O':
					set_fg(&zone[i+dir_offset(dir)]
							,'\0',NULL);
					draw_pos(zone,i+dir_offset(dir));
			}
}
bool try_summon(tile_t *tile,char fg,char *fg_c)
{
	if (!tile->fg) {
		tile->fg=fg;
		tile->fg_c=fg_c;
		return true;
	}
	return false;
}
void spawn_player(tile_t *zone,int *pc)
{
	*pc=rand()%AREA;
	if (!try_summon(&zone[*pc],'@',TERM_COLORS_40M[LBLUE]))
		spawn_player(zone,pc);
}
char move_player(tile_t *zone,char dir,int *pc)
{
	char result='\0';
	if (zone[*pc].fg!='@')
		return '\0';
	result=move_tile(zone,*pc,dir);
	if (result)
		*pc+=dir_offset(dir);
	return result;
}
void set_wall(tile_t *tile,int pos)
{
	set_tile(tile,'%',wall_colors[CHECKER(pos)]
			,'%',wall_colors[CHECKER(pos)]);
}
void set_floor(tile_t *tile,int pos)
{
	set_tile(tile,'\0',NULL,'#',floor_colors[CHECKER(pos)]);
}
void set_door(tile_t *tile)
{
	set_tile(tile,'+',TERM_COLORS_40M[BROWN],'-',TERM_COLORS_40M[BROWN]);
}
void make_building(tile_t *zone,int pos,int w,int h)
{
	// Floors
	for (int y=pos;y<=pos+h*WIDTH;y+=WIDTH)
		for (int x=y;x<=y+w;x++)
			set_floor(&zone[x],x);
	// Horizontal walls
	for (int i=pos;i<=pos+w;i++) {
		set_wall(&zone[i],i);
		set_wall(&zone[i+h*WIDTH],i+h*WIDTH);
	}
	// Vertical walls
	for (int i=pos;i<=pos+h*WIDTH;i+=WIDTH) {
		set_wall(&zone[i],i);
		set_wall(&zone[i+w],i+w);
	}
	// Door
	switch (rand()%4) {
		case 0: // North
			set_door(&zone[pos+w/2]);
			break;
		case 1: // South
			set_door(&zone[pos+h*WIDTH+w/2]);
			break;
		case 2: // West
			set_door(&zone[pos+h/2*WIDTH]);
			break;
		case 3: // East
			set_door(&zone[pos+w+h/2*WIDTH]);
	}
}
void make_random_building(tile_t *zone)
{
	int w=3+rand()%(WIDTH/4),h=3+rand()%(HEIGHT/4);
	int pos=(1+rand()%(WIDTH-w-2))+(1+rand()%(HEIGHT-h-2))*WIDTH;
	make_building(zone,pos,w,h);
}
void cull_walls(tile_t *zone)
{
	// Beware: ugly formatting ahead
	int walls_removed;
	do {
		walls_removed=0;
		for (int i=0;i<AREA;i++)
			if (char_in_string(zone[i].fg,"%+")
					&&((zone[i+1].bg=='#'
					&&zone[i-1].bg=='#')
					||(zone[i+WIDTH].bg=='#'
					&&zone[i-WIDTH].bg=='#'))) {
				set_floor(&zone[i],i);
				walls_removed++;
			}
	} while (walls_removed>0);
}
void create_field(tile_t *field,int b,int m,int a,int s)
{
	fprintf(debug_log,"Growing grass...\n");
	for (int i=0;i<AREA;i++)
		set_tile(&field[i],'\0',NULL,grass_chars[rand()%5]
				,grass_colors[rand()%2]);
	fprintf(debug_log,"Building buildings...\n");
	for (int i=0;i<b;i++)
		make_random_building(field);
	fprintf(debug_log,"Culling walls...\n");
	cull_walls(field);
	fprintf(debug_log,"Creating monsters...\n");
	for (int i=0;i<m;i++)
		set_fg(random_floor(field),'&',TERM_COLORS_40M[GRAY]);
	fprintf(debug_log,"Breeding animals...\n");
	for (int i=0;i<a;i++)
		set_fg(random_grass(field),'A',TERM_COLORS_40M[YELLOW]);
	fprintf(debug_log,"Training soldiers...\n");
	for (int i=0;i<s;i++)
		set_fg(random_floor(field),'$',TERM_COLORS_40M[BLUE]);
	fprintf(debug_log,"Digging stairwell...\n");
	set_bg(random_floor(field),'>',TERM_COLORS_40M[BROWN]);
	fprintf(debug_log,"Opening portal...");
	set_fg(random_grass(field),'O',TERM_COLORS_40M[PURPLE]);
	fprintf(debug_log,"Done!\n");
}
void create_dungeon(tile_t *dungeon,int b,int m)
{
	fprintf(debug_log,"Placing rocks...\n");
	for (int i=0;i<AREA;i++)
		set_tile(&dungeon[i],'%',rock_colors[rand()%2],'\0',NULL);
	fprintf(debug_log,"Carving rooms...\n");
	for (int i=0;i<b;i++)
		make_random_building(dungeon);
	fprintf(debug_log,"Culling walls...\n");
	cull_walls(dungeon);
	fprintf(debug_log,"Creating monsters...\n");
	for (int i=0;i<m;i++)
		set_fg(random_floor(dungeon),'&',TERM_COLORS_40M[GRAY]);
	fprintf(debug_log,"Crafting scepter...\n");
	set_fg(random_empty_tile(dungeon),'I',TERM_COLORS_40M[PURPLE]);
	fprintf(debug_log,"Digging stairwell...\n");
	set_bg(random_floor(dungeon),'<',TERM_COLORS_40M[BROWN]);
	if (b>1) {
		fprintf(debug_log,"Mapping tunnels...\n");
		for (int i=0;i<TUNNELS;i++)
			while (!make_path(dungeon,rand()%AREA));
	}
	fprintf(debug_log,"Done!\n");
}
int dist_to_wall(tile_t *zone,int pos,char dir)
{
	int dist=0,dest=pos;
	if (dir_offset(dir)==0)
		return -1;
	for (;zone[pos].bg!='%';pos=dest) {
		dest=pos+dir_offset(dir);
		dist++;
		if (abs(pos%WIDTH-dest%WIDTH)==WIDTH-1
				||dest<0||dest>AREA)
			return -1;
	}
	return dist;
}
bool make_path(tile_t *zone,int pos)
{
	fprintf(debug_log,"Trying to make a path at %i.\n",pos);
	if (!char_in_string(zone[pos].bg,grass_chars)&&zone[pos].bg) {
		fprintf(debug_log,"Invalid path origin.\n");
		return false;
	}
	fprintf(debug_log,"Start position looks okay...\n");
	fprintf(debug_log,"Trying to determining path direction...\n");
	int dist[2]={AREA,AREA};
	char dirs[2]={'\0','\0'};
	for (int i=1;i<=4;i++) {
		int d=dist_to_wall(zone,pos,2*i+'0');
		if (d>0&&d<dist[1]) {
			if (d<dist[0]) {
				dist[1]=dist[0];
				dist[0]=d;
				dirs[1]=dirs[0];
				dirs[0]=2*i+'0';
			}
			else {
				dist[1]=d;
				dirs[1]=2*i+'0';
			}
		}
	}
	if (dist[1]==AREA||!dirs[1]) {
		fprintf(debug_log,"No valid path direction. Stopping.\n");
		return false;
	}
	fprintf(debug_log,"Placing path tiles...\n");
	for (int i=0;i<2;i++) {
		int d=pos+dist[i]*dir_offset(dirs[i]);
		for (int j=0;j<dist[i];j++) {
			int p=pos+j*dir_offset(dirs[i]);
			set_floor(&zone[p],p);
		}
		set_door(&zone[d]);
	}
	fprintf(debug_log,"Finished making a path.\n");
	return true;
}
