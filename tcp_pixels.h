int tcp_init(char* ipaddr, int port);
void tcp_put_pixels(byte address, byte* pixels, int n);
void tcp_put_pixels_multi(byte** pixel_ptrs, int num_channels, int num_pixels);
