struct startwin_settings {
    int fullscreen, display;
    int xdim3d, ydim3d, bpp3d;
    int forcesetup;
    int usemouse, usejoy;
    int samplerate, bitspersample, channels;

    struct grpfile const *selectedgrp;

    int numplayers;
    char *joinhost;
    int netoverride;
};
