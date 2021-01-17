/* fk.h */

#define FN		0x0100
#define MFN		0x0200
#define ES		0x0400
#define VT		0x0800
#define LEFT_CLICK	(MFN | 1)
#define RIGHT_CLICK	(MFN | 2)
#define CTRL_A		1
#define CTRL_B		2
#define CTRL_C		3
#define CTRL_D		4
#define CTRL_E		5
#define CTRL_F		6
#define CTRL_H		8
#define CTRL_N		14
#define CTRL_O		15
#define CTRL_P		16
#define ESC		27

#ifdef WIN32
void    mouse_position(int *, int *);
#endif // WIN32
