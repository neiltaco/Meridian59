// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * config.c:  This file deals with user configuration of the game, such as 
 *   graphics and sound settings.  The settings are grouped together in a 
 *   global variable called "config".  The settings are all stored in a private
 *   INI file.
 * XXX Should check for errors in these routines
 */

#include "client.h"

#define MAX_INTSTR 12     // Maximum # of digits in an integer

/* Miscellaneous game settings */
Config config;
char inihost[MAXHOST];

// We use two different INI files: meridian.ini (for original preferences) 
// and config.ini (for key/mouse bindings and graphic settings)

// Full pathname of meridian.ini
static char ini_filename[MAX_PATH + FILENAME_MAX];
char *ini_file;  // Pointer to ini_filename

// Full pathname of config.ini
static char config_ini[MAX_PATH + FILENAME_MAX];

// If version doesn't match that in INI file, restore default colors and fonts (used to change
// color and font settings in old clients).
#define INI_VERSION 3

static bool is_steam_install = false;

/* meridian.ini file entries (preferences) */
static char misc_section[]   = "Miscellaneous";  /* Section of INI file for config stuff */
static char INISaveOnExit[]  = "SaveOnExit";
static char INIPlayMusic[]   = "PlayMusic";
static char INIPlaySound[]   = "PlaySound";
static char INIPlayLoopSounds[]   = "PlayLoopSounds";
static char INIPlayRandomSounds[]   = "PlayRandomSounds";
static char INITimeout[]     = "Timeout";
static char INIUserName[]    = "UserName";
static char INIArea[]        = "Area";
static char INIDownload[]    = "Download";
static char INIBrowser[]     = "Browser";
static char INIDefaultBrowser[] = "DefaultBrowser";
static char INIVersion[]     = "INIVersion";
static char INILastPass[]    = "Sentinel";
static char INISoundLibrary[] = "SoundLibrary";
static char INICacheBalance[] = "CacheBalance";
static char INIObjectCacheMin[] = "ObjectCacheMin";
static char INIGridCacheMin[] = "GridCacheMin";
static char INIMusicVolume[]  = "MusicVolume";
static char INISoundVolume[]  = "SoundVolume";
static char INIAmbientVolume[] = "AmbientVolume";

static char interface_section[]= "Interface";
static char INIDrawMap[]     = "DrawMap";
static char INIScrollLock[]  = "ScrollLock";
static char INITooltips[]    = "Tooltips";
static char INIInventory[]   = "InventoryNum";
static char INIAggressive[]  = "Aggressive";
static char INIBounce[]      = "Bounce";
static char INIToolbar[]     = "Toolbar";
static char INIPainFX[]      = "Pain";
static char INIWeatherFX[]   = "Weather";
static char INIOldProfane[]  = "AntiProfane";
static char INIIgnoreProfane[]= "IgnoreProfane";
static char INIAntiProfane[] = "ProfanityFilter";
static char INIExtraProfane[]= "ExtraProfaneSearch";
static char INILagbox[]      = "LatencyMeter";
static char INISpinningCube[]= "SpinningCube";
static char INIHaloColor[]   = "HaloColor";
static char INIColorCodes[]  = "ColorCodes";
static char INIMapAnnotations[] = "MapAnnotations";

static char window_section[] = "Window";         /* Section in INI file for window info */
static char INILeft[]        = "NormalLeft";
static char INIRight[]       = "NormalRight";
static char INITop[]         = "NormalTop";
static char INIBottom[]      = "NormalBottom";
static char INIShow[]        = "Show";
static char INIMaxX[]        = "MaxX";
static char INIMaxY[]        = "MaxY";

static char comm_section[]   = "Comm";  /* Section for comm stuff in INI file */
static char INIPort[]        = "Port";
static char INIRedialDelay[] = "RedialDelay";
static char INIHostname[]    = "Hostname";
static char INISockPort[]    = "SocketPort";
static char INIServerNum[]   = "ServerNumber";
static char INIDomainFormat[] = "Domain";

static char users_section[]  = "Users";  /* Section for dealing with other users */
static char INIDrawNames[]   = "DrawNames";
static char INIIgnoreAll[]   = "IgnoreAll";
static char ININoBroadcast[] = "NoBroadcast";
static char INIIgnoreList[]  = "IgnoreList";

static char special_section[] = "Special";  /* Section for hidden stuff in INI file */
static char INIDebug[]        = "Debug";
static char INISecurity[]     = "Security";
static char INITechnical[]    = "Technical";

/* config.ini file entries (preferences) */
static char config_section[] = "config";  /* Section for configuration stuff */
static char INIGpuEfficiency[] = "gpuefficiency";
static char INIGpuEfficiencyOneTimeFlip[] = "gpuefficiencyonetimeflip";

static char INITextAreaSize[] = "TextAreaSize";

static char INIActiveStatGroup[] = "ActiveStatGroup";

#ifndef NODPRINTFS
static char INIShowMapBlocking[]= "ShowMapBlocking";
static char INIShowFPS[]     = "ShowFPS";
static char INIShowUnseenWalls[] = "ShowUnseenWalls";
static char INIShowUnseenMonsters[] = "ShowUnseenMonsters";
static char INIAvoidDownloadAskDialog[] = "AvoidDownloadAskDialog";
static char INIMaxFPS[]	  = "MaxFPS";
static char INIClearCache[]	  = "ClearCache";
static char INIQuickStart[]   = "QuickStart";
#endif // NODPRINTFS

static int   DefaultRedialDelay   = 60;
static char  DefaultHostname[]    = "cheater";
static char  DefaultDomainFormat[] = "meridian%d.meridian59.com"; // MUST have a %d in it somewhere.
static char  DefaultSockPortFormat[] = "59%.2d";
static int   DefaultServerNum     = -1;
static int   DefaultTimeout       = 20;

/************************************************************************/
/* 
 * LoadSettings:  Load all user's settings from INI file.
 */
void LoadSettings(void)
{
   FontsCreate(False);
   ColorsCreate(False);
   CommLoadSettings();
   ConfigLoad();
   LoadProfaneTerms();

   // Restore defaults if they've changed from previous version
   if (config.ini_version != INI_VERSION)
   {
     ColorsRestoreDefaults();
     FontsRestoreDefaults();
     config.ini_version = INI_VERSION;
   }
}
/************************************************************************/
/*
 * SaveSettings:  Save all user's settings (fonts, etc.)
 */
void SaveSettings(void)
{
   WindowSettingsSave();
   FontsSave();
   ColorsSave();
   CommSaveSettings();
   ConfigSave();
   SaveProfaneTerms();
}

/****************************************************************************/
/*
 * ConfigInit:  Find filenames of INI files.  This function must be called before
 *   using the ini_file/config_ini variables to read from or write to the files.
 */
void ConfigInit(void)
{
   char dir[MAX_PATH];

   GetGamePath( dir );

   // Retrieve the filenames for meridian.ini (preferences) and config.ini (configuration)
   snprintf(ini_filename, sizeof(ini_filename), "%s%s", dir, GetString(hInst, IDS_INIFILE));
   ini_file = ini_filename;

   snprintf(config_ini, sizeof(config_ini), "%s%s", dir, GetString(hInst, IDS_INIFILECONFIG));
}
/****************************************************************************/
/*
 * ConfigLoad:  Load user configuration from INI file.  If entries not present
 *   in INI file, assign defaults.
 */
void ConfigLoad(void)
{
   int index = 0;
   char *start, *ptr;
   char ignore_string[MAX_IGNORE_LIST * (MAX_CHARNAME + 1) + 1];
   
   config.save_settings = GetConfigInt(misc_section, INISaveOnExit, True, ini_file);
   config.play_music    = GetConfigInt(misc_section, INIPlayMusic, True, ini_file);
   config.play_sound    = GetConfigInt(misc_section, INIPlaySound, True, ini_file);
   config.music_volume    = GetConfigInt(misc_section, INIMusicVolume, 100, ini_file);
   config.sound_volume    = GetConfigInt(misc_section, INISoundVolume, 100, ini_file);
   config.ambient_volume    = GetConfigInt(misc_section, INIAmbientVolume, 100, ini_file);
   config.play_loop_sounds    = GetConfigInt(misc_section, INIPlayLoopSounds, True, ini_file);
   config.play_random_sounds    = GetConfigInt(misc_section, INIPlayRandomSounds, True, ini_file);

   config.ini_version   = GetConfigInt(misc_section, INIVersion, 0, ini_file);
   config.default_browser = GetConfigInt(misc_section, INIDefaultBrowser, True, ini_file);
   GetPrivateProfileString(misc_section, INIUserName, "", 
			   config.username, MAXUSERNAME, ini_file); 
   GetPrivateProfileString(misc_section, INIBrowser, "", 
			   config.browser, MAX_PATH, ini_file); 
   
   config.draw_names   = GetConfigInt(users_section, INIDrawNames, True, ini_file);
   config.ignore_all   = GetConfigInt(users_section, INIIgnoreAll, False, ini_file);
   config.no_broadcast = GetConfigInt(users_section, ININoBroadcast, False, ini_file);

   GetPrivateProfileString(users_section, INIIgnoreList, "", 
                           ignore_string, sizeof(ignore_string), ini_file);
   memset(config.ignore_list, 0, sizeof(config.ignore_list));
   // Parse ignore string: usernames separated by commas
   start = ignore_string;
   ptr = start;
   while (index < MAX_IGNORE_LIST && *ptr != 0) {
     if (*ptr == ',') {
       *ptr = 0;
       strncpy(config.ignore_list[index], start, MAX_CHARNAME);
       start = ptr + 1;
       ++index;
     }
     ++ptr;
   }
   
   config.scroll_lock  = GetConfigInt(interface_section, INIScrollLock, False, ini_file);
   config.drawmap      = GetConfigInt(interface_section, INIDrawMap, True, ini_file);
   config.tooltips     = GetConfigInt(interface_section, INITooltips, True, ini_file);
   config.inventory_num= GetConfigInt(interface_section, INIInventory, True, ini_file);
   config.aggressive   = GetConfigInt(interface_section, INIAggressive, False, ini_file);
   config.bounce       = GetConfigInt(interface_section, INIBounce, True, ini_file);
   config.toolbar      = GetConfigInt(interface_section, INIToolbar, True, ini_file);
   config.pain         = GetConfigInt(interface_section, INIPainFX, True, ini_file);
   config.weather      = GetConfigInt(interface_section, INIWeatherFX, False, ini_file);
   config.antiprofane  = GetConfigInt(interface_section, INIAntiProfane, True, ini_file);
   config.ignoreprofane = GetConfigInt(interface_section, INIIgnoreProfane, False, ini_file);
   config.extraprofane = GetConfigInt(interface_section, INIExtraProfane, False, ini_file);
   config.lagbox       = GetConfigInt(interface_section, INILagbox, True, ini_file);
   config.spinning_cube= GetConfigInt(interface_section, INISpinningCube, False, ini_file);
   config.halocolor    = GetConfigInt(interface_section, INIHaloColor, 0, ini_file);
   config.colorcodes   = GetConfigInt(interface_section, INIColorCodes, True, ini_file);
   config.map_annotations = GetConfigInt(interface_section, INIMapAnnotations, True, ini_file);

   config.lastPasswordChange = GetConfigInt(misc_section, INILastPass, 0, ini_file);

   /* charlie: 
		This works like this , the balance is a % between 10%-90% which controls how much of the memory
		goes to which cache, so a value of 70% means that 70% of the cache memory goes to the 
		object cache, the remaining 30% to the grid cache anything over 90% is clamped, as is
		anything below 10%
	*/

   config.CacheBalance   = GetConfigInt(misc_section, INICacheBalance,        70, ini_file);
   config.ObjectCacheMin = GetConfigInt(misc_section, INIObjectCacheMin, 6000000, ini_file);
   config.GridCacheMin = GetConfigInt(misc_section, INIGridCacheMin,   4000000, ini_file);

   if( config.CacheBalance < 10 ) config.CacheBalance = 10 ;
   if( config.CacheBalance > 90 ) config.CacheBalance = 90 ;

   config.soundLibrary = GetConfigInt(misc_section, INISoundLibrary, LIBRARY_MSS, ini_file);

#ifdef NODPRINTFS
   config.debug    = False;
   config.security = True;
   config.timeout  = DefaultTimeout;
#else
   config.debug = 
      GetConfigInt(special_section, INIDebug, False, ini_file);
   config.security = 
      GetConfigInt(special_section, INISecurity, True, ini_file);
   config.timeout = GetConfigInt(misc_section, INITimeout, DefaultTimeout, ini_file);
#endif

   config.technical = GetConfigInt(special_section, INITechnical, False, ini_file);

#ifndef NODPRINTFS
   config.showMapBlocking = GetConfigInt(special_section, INIShowMapBlocking, 0, ini_file);
   config.showFPS      = GetConfigInt(special_section, INIShowFPS, 0, ini_file);
   config.showUnseenWalls = GetConfigInt(special_section, INIShowUnseenWalls, 0, ini_file);
   config.showUnseenMonsters = GetConfigInt(special_section, INIShowUnseenMonsters, 0, ini_file);
   config.avoidDownloadAskDialog = GetConfigInt(special_section, INIAvoidDownloadAskDialog, 0, ini_file);
   config.maxFPS = GetConfigInt(special_section, INIMaxFPS, 120, ini_file);
   config.clearCache = GetConfigInt(special_section, INIClearCache, False, ini_file);
   //config.quickstart = GetConfigInt(special_section, INIQuickStart, 0, ini_file);
#else
   config.showMapBlocking = FALSE;
   config.showFPS = FALSE;
   config.showUnseenWalls = FALSE;
   config.showUnseenMonsters = FALSE;
   config.avoidDownloadAskDialog = FALSE;
   config.maxFPS = FALSE;
   config.clearCache = FALSE;
#endif // NODPRINTFS

   config.text_area_size = GetConfigInt(misc_section, INITextAreaSize, TEXT_AREA_HEIGHT, ini_file);

   // Default to stat group 5 (INVENTORY)
   config.active_stat_group = GetConfigInt(misc_section, INIActiveStatGroup, 5, ini_file);

   // Determine if we should be using gpu efficiency mode or not.
   auto one_time_gpu_efficiency_enablement = GetConfigInt(config_section, 
	   "gpuefficiencyonetimeflip", False, config_ini);
   if (one_time_gpu_efficiency_enablement)
   {
      char config_value[10];
      GetPrivateProfileString(config_section, INIGpuEfficiency, "true", config_value, 
		  sizeof(config_value), config_ini);
      config.gpuEfficiency = (0 == strcmp(config_value, "true"));
   }
   else
   {
      // TODO: Added February 2025 - remove after the next update when everyone will have been reset.
      // Also remove the `one_time_gpu_efficiency_enablement` variable and associated check above.

      // Perform one-time flip to enable GPU efficiency mode regardless of current preference.
      // This is to ensure players start from a clean slate with maximum performance.
      // After this one-time flip, future changes to gpu efficiency preferences will be respected.
      config.gpuEfficiency = true;
      WriteConfigInt(config_section, INIGpuEfficiencyOneTimeFlip, True, config_ini);
      WritePrivateProfileString(config_section, INIGpuEfficiency, "true", config_ini);
   }

   TimeSettingsLoad();
}
/****************************************************************************/
void ConfigSave(void)
{
  int i;
  char ignore_string[MAX_IGNORE_LIST * (MAX_CHARNAME + 1) + 1];
  
   WriteConfigInt(misc_section, INISaveOnExit, config.save_settings, ini_file);
   WriteConfigInt(misc_section, INIPlayMusic, config.play_music, ini_file);
   WriteConfigInt(misc_section, INIPlaySound, config.play_sound, ini_file);
   WriteConfigInt(misc_section, INIMusicVolume, config.music_volume, ini_file);
   WriteConfigInt(misc_section, INISoundVolume, config.sound_volume, ini_file);
   WriteConfigInt(misc_section, INIAmbientVolume, config.ambient_volume, ini_file);
   WriteConfigInt(misc_section, INIPlayLoopSounds, config.play_loop_sounds, ini_file);
   WriteConfigInt(misc_section, INIPlayRandomSounds, config.play_random_sounds, ini_file);
   WriteConfigInt(misc_section, INITimeout, config.timeout, ini_file);
   WriteConfigInt(misc_section, INIVersion, config.ini_version, ini_file);
   WriteConfigInt(misc_section, INIDefaultBrowser, config.default_browser, ini_file);
   WritePrivateProfileString(misc_section, INIBrowser, config.browser, ini_file);

   WriteConfigInt(misc_section, INICacheBalance, config.CacheBalance, ini_file);
   WriteConfigInt(misc_section, INIObjectCacheMin, config.ObjectCacheMin, ini_file);
   WriteConfigInt(misc_section, INIGridCacheMin, config.GridCacheMin, ini_file);

   WriteConfigInt(users_section, INIDrawNames, config.draw_names, ini_file);
   WriteConfigInt(users_section, INIIgnoreAll, config.ignore_all, ini_file);
   WriteConfigInt(users_section, ININoBroadcast, config.no_broadcast, ini_file);

   // Build up comma-separated list of character names
   ignore_string[0] = 0;
   for (i = 0; i < MAX_IGNORE_LIST; ++i) {
     if (config.ignore_list[i][0] != 0) {
       strcat(ignore_string, config.ignore_list[i]);
       strcat(ignore_string, ",");
     }
   }
   WritePrivateProfileString(users_section, INIIgnoreList, ignore_string, ini_file);

   WritePrivateProfileString(misc_section, INIUserName, config.username, ini_file);

   WriteConfigInt(interface_section, INIScrollLock, config.scroll_lock, ini_file);
   WriteConfigInt(interface_section, INITooltips, config.tooltips, ini_file);
   WriteConfigInt(interface_section, INIInventory, config.inventory_num, ini_file);
   WriteConfigInt(interface_section, INIAggressive, config.aggressive, ini_file);
   WriteConfigInt(interface_section, INIBounce, config.bounce, ini_file);
   WriteConfigInt(interface_section, INIToolbar, config.toolbar, ini_file);
   WriteConfigInt(interface_section, INIDrawMap, config.drawmap, ini_file);

   WriteConfigInt(interface_section, INIPainFX, config.pain, ini_file);
   WriteConfigInt(interface_section, INIWeatherFX, config.weather, ini_file);
   WriteConfigInt(interface_section, INIAntiProfane, config.antiprofane, ini_file);
   WriteConfigInt(interface_section, INIIgnoreProfane, config.ignoreprofane, ini_file);
   WriteConfigInt(interface_section, INIExtraProfane, config.extraprofane, ini_file);
   WriteConfigInt(interface_section, INILagbox, config.lagbox, ini_file);
   WriteConfigInt(interface_section, INISpinningCube, config.spinning_cube, ini_file);
   WriteConfigInt(interface_section, INIHaloColor, config.halocolor, ini_file);
   WriteConfigInt(interface_section, INIColorCodes, config.colorcodes, ini_file);
   WriteConfigInt(interface_section, INIMapAnnotations, config.map_annotations, ini_file);
   
   WriteConfigInt(misc_section, INILastPass, config.lastPasswordChange, ini_file);

   WriteConfigInt(misc_section, INITextAreaSize, config.text_area_size, ini_file);

   WriteConfigInt(misc_section, INIActiveStatGroup, config.active_stat_group, ini_file);

   // "Special" section options NOT saved, so that they're not normally visible

   WritePrivateProfileString(interface_section, INIOldProfane, NULL, ini_file); // remove old string
}
/************************************************************************/
void ConfigOverride(LPCTSTR pszCmdLine)
{
   int argp;
   for (argp=1; argp < __argc; argp++)
   {
      char* p = __argv[argp];
      if (p && (*p == '-' || *p == '/'))
      {
	 char ch;
	 p++;
	 ch = *p++;
	 if (*p == ':')
	    p++;
	 switch (ch)
	 {
	 case 'h':
	 case 'H':
	    debug(("/H: \"%s\"\n", p));
	    strcpy(config.comm.hostname, p);
	    break;

	 case 'p':
	 case 'P':
	    debug(("/P: \"%s\"\n", p));
	    if (0 == atoi(p))
	    {
	       debug(("  ignoring invalid port number\n"));
	       break;
	    }
	    config.comm.sockport = atoi(p);
	    break;

	 case 'u':
	 case 'U':
	    debug(("/U: \"%s\"\n", p));
	    strcpy(config.username, p);
	    break;

	 case 'w':
	 case 'W':
	    debug(("/W: option got something\n"));
	    strcpy(config.password, p);
	    break;

   case 's':
   case 'S':
     // We had a snafu during an update, and now we need to be able to
     // distinguish between Steam and non-Steam installs.  Specifying -s
     // means this is a Steam install.
     debug(("/S: Steam install\n"));
     is_steam_install = true;
     break;

	 case 'q':
	 case 'Q':
	    debug(("/Q: will try to start quick\n"));
	    config.quickstart = TRUE;
	    break;
	 }
      }
   }

   // We had a problem where the Web (non-Steam) client was accidentally
   // published with a download time of 9999.  To undo this, we
   // interpret 9999 as the then-current download time of 195.  However,
   // the Steam version also was set to 9999, and we don't want to
   // have the Steam version accidentally trying to do a download.  So
   // we specify a command line flag from Steam to avoid it.
   // TODO: Remove if/when Steam-only
   if (!is_steam_install && config.download_time == 9999) {
     config.download_time = 195;
   }
}
/************************************************************************/
void WindowSettingsSave(void)
{
   RECT *r;
   WINDOWPLACEMENT w;
   
   w.length = sizeof(WINDOWPLACEMENT);
   GetWindowPlacement(hMain, &w);

   r = &w.rcNormalPosition;

   w.ptMaxPosition.x = min(w.ptMaxPosition.x, - GetSystemMetrics(SM_CXFRAME));
   w.ptMaxPosition.y = min(w.ptMaxPosition.y, - GetSystemMetrics(SM_CYFRAME));

   WriteConfigInt(window_section, INILeft, r->left, ini_file);
   WriteConfigInt(window_section, INIRight, r->right, ini_file);
   WriteConfigInt(window_section, INITop, r->top, ini_file);
   WriteConfigInt(window_section, INIBottom, r->bottom, ini_file);
   WriteConfigInt(window_section, INIMaxX, w.ptMaxPosition.x, ini_file);
   WriteConfigInt(window_section, INIMaxY, w.ptMaxPosition.y, ini_file);
   WriteConfigInt(window_section, INIShow, w.showCmd, ini_file);
}
/************************************************************************/
void WindowSettingsLoad(WINDOWPLACEMENT *w)
{
   RECT *r = &w->rcNormalPosition;
   int def_x, def_y, def_width, def_height;

   // When no INI file exists, we need to give a default position for the maximized window.
   // We need to place the window so that its border is off the screen; this is the
   // default Windows behavior.
   def_x = - GetSystemMetrics(SM_CXFRAME);
   def_y = - GetSystemMetrics(SM_CYFRAME);

   // Try to make client window optimally sized, but also fit to screen
   def_width  = min(MAIN_DEF_WIDTH, GetSystemMetrics(SM_CXSCREEN));
   def_height = min(MAIN_DEF_HEIGHT, GetSystemMetrics(SM_CYSCREEN));

   r->left   = GetConfigInt(window_section, INILeft, MAIN_DEF_LEFT, ini_file);
   r->right  = GetConfigInt(window_section, INIRight, MAIN_DEF_LEFT + def_width, ini_file);
   r->top    = GetConfigInt(window_section, INITop, MAIN_DEF_TOP, ini_file);
   r->bottom = GetConfigInt(window_section, INIBottom, MAIN_DEF_TOP + def_height, ini_file);
   w->ptMaxPosition.x = GetConfigInt(window_section, INIMaxX, def_x, ini_file);
   w->ptMaxPosition.y = GetConfigInt(window_section, INIMaxY, def_y, ini_file);
   w->showCmd = GetConfigInt(window_section, INIShow, SW_SHOWNORMAL, ini_file);
}

/********************************************************************/
/*
 * CommLoadSettings:  Load comm parameters from INI file.
 *  If use_defaults is False, try to load fonts from INI file.
 *  Otherwise use default comm settings.
 */
void CommLoadSettings(void)
{
   config.comm.timeout = GetConfigInt(comm_section, INIRedialDelay, DefaultRedialDelay, ini_file);

   GetPrivateProfileString(comm_section, INIHostname, DefaultHostname, 
			   inihost, MAXHOST, ini_file);
   GetPrivateProfileString(comm_section, INIDomainFormat, DefaultDomainFormat, 
			   config.comm.domainformat, MAXHOST, ini_file);
   if (!strstr(config.comm.domainformat, "%d"))
      strcpy(config.comm.domainformat, DefaultDomainFormat);
   config.comm.server_num = GetConfigInt(comm_section, INIServerNum, DefaultServerNum, ini_file);

   // We read the DomainFormat INI value (which we don't write into the INI)
   // and we set the hostname from that and the specified server number.
   //
   // A Meridian Service Provider other than NDS can supply another
   // value for DomainFormat in either the code (DefaultDomainFormat)
   // or by setting Domain=msp%d.foobar.co.sw in the INI.
   // We require there to be one %d in the format somewhere.
   //
   ConfigSetServerNameByNumber(config.comm.server_num);

   // What's the default port number we should connect to?  This varies by server number.
   config.comm.sockport = GetConfigInt(comm_section, INISockPort, -1, ini_file);

   if (config.comm.sockport == -1)
   {
      // We have no socket number set in the .ini file.
      config.comm.constant_port = False;
      ConfigSetSocketPortByNumber(config.comm.server_num);
   }
   else
   {
      config.comm.constant_port = True;
   }

}
/********************************************************************/
/*
 * CommSaveSettings:  Save comm settings to INI file.
 */
void CommSaveSettings(void)
{
   WriteConfigInt(comm_section, INIServerNum, config.comm.server_num, ini_file);
}
/********************************************************************/
/* 
 * TimeSettingsLoad:  Load last download time from INI file
 */
void TimeSettingsLoad(void)
{
   config.download_time = GetConfigInt(misc_section, INIDownload, 0, ini_file);
}

int GetTimeSettings(void)
{
 return GetConfigInt(misc_section, INIDownload, 0, ini_file);

}
/********************************************************************/
/* 
 * TimeSettingsSave:  Set last download time to given value and save
 *   to INI file.
 */
void TimeSettingsSave(int download_time)
{
   config.download_time = download_time;
   WriteConfigInt(misc_section, INIDownload, config.download_time, ini_file);
}

/* 32 bit replacements for 16-bit Windows functions */
/********************************************************************/
/*
 * GetConfigInt:  32-bit version of GetPrivateProfileInt
 */
int GetConfigInt(char *section, char *key, int default_value, char *fname)
{
   char buf[MAX_INTSTR], temp[MAX_INTSTR];

   snprintf(temp, sizeof(temp), "%d", default_value);

   if (GetPrivateProfileString(section, key, temp, buf, MAX_INTSTR, fname) == 0)
      return default_value;
   
   return atoi(buf);
}
/********************************************************************/
/*
 * WriteConfigInt:  32-bit version of WritePrivateProfileInt
 */
BOOL WriteConfigInt(char *section, char *key, int value, char *fname)
{
   char buf[MAX_INTSTR];

   snprintf(buf, sizeof(buf), "%d", value);
   return WritePrivateProfileString(section, key, buf, fname);
}

/********************************************************************/
/*
 * ConfigSetServerNameByNumber:  Set server name given server number.
 */
void ConfigSetServerNameByNumber(int num)
{
   snprintf(config.comm.hostname, sizeof(config.comm.hostname), config.comm.domainformat, num);
}

/********************************************************************/
/*
 * ConfigSetServerPortByNumber:  Set socketport from server number.
 */
void ConfigSetSocketPortByNumber(int num)
{
   char buf[MAX_INTSTR];

   // Do not alter it if we have a constant port set.
   if (config.comm.constant_port)
   {
      return;
   }

   /* We try to connect to the socket of the form 59##, where ## is
   ** the last two numbers of the server number.  So, if the user
   ** specifies server 103, it will try to connect to port 5903 on
   ** the specified server.  This allows us to run multiple server 
   ** instances on a single machine.
   **
   ** There should be no conflicts for most server numbers, since
   ** ports 5900 to 5967 are currently unassigned. If conflicts do
   ** appear, just change DefaultSockPortFormat.
   */

   snprintf(buf, sizeof(buf), DefaultSockPortFormat, (num % 100));

   config.comm.sockport = atoi(buf);
}

bool IsSteamVersion() {
  return is_steam_install;
}
