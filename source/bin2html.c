#include <stdio.h>

char *cc[] = {
	"#000000",
	"#0000AA",
	"#00AA00",
	"#00AAAA",
	"#AA0000",

	"#AA00AA",
	"#AA5500",
	"#AAAAAA",
	"#555555",
	"#5555FF",

	"#55AA55",
	"#55FFFF",
	"#FFAAAA",
	"#FFAAFF",
	"#FFFFAA",

	"#FFFFFF"
};

int main(int argc, char **argv)
{
	FILE *fp;
	unsigned char ch,at;
	int i;

	fp = fopen("DUKE3D.BIN","rb");
	if (!fp) {
		return -1;
	}

	printf("<html><head><title>DUKE3D.BIN</title></head><body><pre style=\"font-family:HyperFont,monospace;line-height:1\">");
	
	for (i=0;i<2000;i++) {
		ch = (unsigned char)fgetc(fp);
		at = (unsigned char)fgetc(fp);

		printf("<span style=\"color:%s;background-color:%s\">",cc[ at&15 ],cc[ (at>>4)&15 ] );
		putchar(ch);
		printf("</span>");
		if (i%80==79) putchar('\n');
	}

	printf("</pre></body></html>");
	
	return 0;
}

