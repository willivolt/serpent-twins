COPTS=-framework OpenGL -framework GLUT -std=c99

serpent: serpent.c
	gcc $(COPTS) serpent.c -o serpent

