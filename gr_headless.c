/* gr_headless.c - Headless stubs for Python bindings */

int c0 = 0, c1 = 0, bx = 0, by = 0, bw = 0, bh = 0, cw = 0, ch = 0, xpix = 800, ypix = 600, ytop = 0, ybot = 0, ycmd = 0;
int keyevent = 0;

void xw_init(void) {}
void xw_line(int x1, int y1, int x2, int y2, int c) {}
void xw_clrb(int x1, int y1, int x2, int y2) {}
void xw_flush(void) {}
int xw_kbhit(void) { return 0; }
int xw_getch(void) { return 0; }
void xw_close(void) {}

void gr_cinit(void) {}
void gr_line(int x1, int y1, int x2, int y2, int c) {}
void gr_clrb(int x1, int y1, int x2, int y2) {}
void gr_update(int mstatus) {}
