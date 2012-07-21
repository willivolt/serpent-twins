void tcp_init();
void tcp_init_addresses();
void tcp_put_pixels(byte address, byte* pixels, int n);
void tcp_put_pixels_multi(byte** pixel_ptrs, int num_channels, int num_pixels);
