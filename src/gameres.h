// resource ids
#define IDD_STARTWIN                1000
#define IDC_STARTWIN_TABCTL         1001
#define IDC_STARTWIN_BITMAP         1002
#define IDC_ALWAYSSHOW              1003
#define IDC_STARTWIN_APPTITLE       1004

#define IDD_PAGE_CONFIG             1100
#define IDC_FULLSCREEN              1101
#define IDC_VMODE3D                 1102
#define IDC_SINGLEPLAYER            1103
#define IDC_JOINMULTIPLAYER         1104
#define IDC_HOSTMULTIPLAYER         1105
#define IDC_HOSTFIELD               1106
#define IDC_NUMPLAYERS              1107
#define IDC_NUMPLAYERSUD            1108
#define IDC_SOUNDQUALITY            1109
#define IDC_USEMOUSE                1110
#define IDC_USEJOYSTICK             1111

#define IDD_PAGE_MESSAGES           1200
#define IDC_MESSAGES                1201

#define IDD_PAGE_GAME               1300
#define IDC_GAMELIST                1301

#define IDI_ICON       100
#define IDB_BMP        200

#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif

#define Y_POS(origin,index) (origin + (index * 15))
