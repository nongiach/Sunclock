#define N_MENU 28
#define N_ZOOM 14
#define N_OPTION 12
#define N_URBAN 12
#define N_FILESEL 6
#define N_HELP 49

#define STDFORMATS "%H:%M%_%a%_%d%_%b%_%y|%H:%M:%S%_%Z|%a%_%j/%t%_%U/52"

enum {L_POINT=0, L_GMTTIME, L_SOLARTIME, L_LEGALTIME,
      L_DAYLENGTH, L_SUNRISE, L_SUNSET, 

      L_SEC, L_MIN, L_HOUR, L_DAY, L_DAYS, 

      L_CLICKCITY, L_CLICKLOC, L_CLICK2LOC, L_DEGREE,

      L_KEY, L_CONTROLS, L_ESCAPE, L_ESCMENU, L_UNKNOWN, L_SYNCHRO, 
      L_PROGRESS, L_TIMEJUMP,

      L_CITYNAME, L_TIMEZONE, L_LATITUDE, L_LONGITUDE, L_CITYSIZE, 
      L_CITYWARNING1, L_CITYWARNING2, 

      L_OPTION, L_ACTIVATE, L_INCORRECT, L_OPTIONINTRO,
      L_ONCE, L_PERIODIC, L_STARRYSKY, L_BLANKSCREEN,

      L_LISTOPTIONS, L_CONFIG, L_NEWIMAGE, L_SHORTHELP,

      L_END
      };

#ifdef DEFVAR
char	* Label[L_END] = {
	"Point", "GMT time", "Solar time", "Legal time", 
        "Day length", "Sunrise", "Sunset", 

	"seconds", "minute", "hour", "day", "days",

        "Click on a city",
        "Click on a location",
        "Click consecutively on two locations",
        "Double click or strike * for ° ' \"",

	"Key",
        "Key/Mouse controls",
	"Escape",
        "Escape menu",
	"Unknown key binding !!",
        "Synchro (\")",
	"Progress value =",
        "Global time shift =",

        "City name",
	"Timezone",
	"Latitude",
	"Longitude",
	"Size",
	"Warning: %s, lat=%s lon=%s already in list of cities !!",
	"Overriding previous entry for %s",

	"Option",
        "Activating selected option...",
        "Option incorrect or not available at runtime !!",
	"Options: strike <Ctrl><Space> for blank space within an item",
	"(once)",
	"(periodically, period %d seconds)",
	"(with starry sky)",
	"(blank root window)",

	"with the following rather long list of options:",
        "Starting from **, options are runtime configurable.",
	"Calculating new image...",
        "Sunclock has a number of internal procedures which can be accessed\n"
            "through mouse clicks or key controls:"
};

char    MenuKey[2*N_MENU] = 
"H,F,Z,U,O;C,S,D,E,L;A,B,G,J;N,Y,M,P,T;W,K,I,R;>,<,!;X,Q;";

char    ZoomKey[2*N_ZOOM] =
"*,«,#,/;&,+,-,1,.;>,<,!,W,K;";

char    OptionKey[2*N_OPTION] = 
"@,%;=,°;[,];G,J,';!,W,K;";

char    UrbanKey[2*N_URBAN] = 
"U,°,§,%;C,S;~,(,);!,W,K;";

char    CommandKey[N_HELP] = 
"HFZUOCSDELABGJNYMPTWKIR><!XQ*«#/&+-1.\"@%=[]'°§~()";

char  * FileselBanner[N_FILESEL] = { 
    "home", "share", "  /", "  .", "  W", "  !"
};

char    WeakChars[] = "QCDEMPSTUK><!";

char  * Help[N_HELP] = {

/* Menu window help */
"Show help and commands (H or click on strip)",
"Load Earth map file (F or mouse button 2)",
"Zoom (Z or mouse button 3)",
"Urban command window",
"Option command window",
"Use coordinate mode",
"Use solar time mode",
"Use distance mode",
"Use hour extension mode",
"Use legal time mode",
"Modify time forwArd",
"Modify time Backward",
"Adjust proGress value",
"Reset global time shift to Zero",
"Draw/Erase Night area",
"Draw/Erase Sun and Moon",
"Draw/Erase meridians",
"Draw/Erase parallels",
"Draw/Erase Tropics/Equator/Arctic circles",
"Open new map window (W or Mousebutton 3)",
"Close window",
"Iconify window",
"Refresh map window",
"Adjust window width to screen size",
"Back to previous window size",
"Switch clock and map windows",
"Activate command (-command option)",
"Quit program",

/* Zoom window help */
"Activate new zoom settings",
"Return to previous zoom settings",
"Cancel change in zoom settings",
"Set aspect by resizing main window",
"Cycle through zoom modes 0,1,2",
"Zoom in by factor 1.2",
"Zoom out by factor 1/1.2 = 0.833",
"Return to zoom factor = 1 (full map)",
"Center zoom area on selected city or location",
"Synchronize zoom operation",

/* Option window help */
"Activate the selected option",
"Erase the command/parameter line(s)",
"Synchronize windows or not",
"Copy map to root window",
"Erase map from root window",
"Start/stop animation",

/* Urban window help */
"Use degree, minute, seconds, or decimal degrees",
"Search/select city",
"Modify city parameters",
"Create new city location",
"Delete city"

};

char	*ShortHelp = 
"Sunclock has a number of internal procedures which can be accessed\n"
"through mouse clicks or key controls:";

char  	Day_name[7][10] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

char    Month_name[12][10] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
        "Aug", "Sep", "Oct", "Nov", "Dec" 
};

#else

extern char Day_name[7][10];
extern char Month_name[12][10];
extern char MenuKey[N_MENU];
extern char ZoomKey[N_ZOOM];
extern char OptionKey[N_OPTION];
extern char UrbanKey[N_URBAN];
extern char CommandKey[N_HELP];

#endif
