#include "wcleaner.h"

#include "crc.h"
#include "texopt.h"
#include "util.h"
#include "wad.h"

/// Path to external IWAD
char *ExternalIWAD;
char *WorkPWAD;
char *OutPWAD;
bool SupressDebug;


void WC_Exit(int code)
{
    exit(code);
}

static void Usage()
{
    fprintf(stdout, "Usage: wcleaner [OPTIONS] pwad\n");
    fprintf(stdout, "\t-o outwad\t\tOutput optimized PWAD into outwad\n");
    fprintf(stdout, "\t-s \t\t\tSupress debug output\n");
}

void WC_ParseCmd(int argc, char **argv)
{
    fprintf(stdout, "WCleaner 0.1 by cybermind - a tool to clean out your PWAD from garbage.\n");
    if (argc < 2) {
        Usage();
        WC_Exit(1);
    }
    for (;;) {
		switch (getopt(argc, argv, "ho:s?")) {
		    case 'o': {
                OutPWAD = optarg;
                continue;
		    }
            case 's': {
                SupressDebug = true;
                continue;
		    }
		    case -1:
				break;
			case '?':
			case 'h':
			default:
				Usage();
				WC_Exit(1);
		}
		break;
    }

    if (!(argc - optind)) {
		DebugPrint("Specify a PWAD to process.");
		WC_Exit(1);
	}
    char *index;
    WorkPWAD = argv[optind];
    // replace '\' to '/'
    while (index = strchr(WorkPWAD, '\\')) {
        *index = '/';
    }
}

int WC_Main(int argc, char **argv)
{
    int beforeOpt, afterOpt;

    // open output files
    my_stdout = freopen ("stdout.txt", "wb" /*or "wt"*/, stdout);
	my_stderr = freopen ("stderr.txt", "wb" /*or "wt"*/, stderr);

    WC_ParseCmd(argc, argv);  // get command line options
	CRC_Init();
	wFile_t *file = Wad_Open(WorkPWAD);
	if (!file) {
        WC_Exit(1);
	}
	beforeOpt = file->size;
	if (TexOpt_Optimize(file)) {
        Wad_Output(file, OutPWAD);
        afterOpt = file->size;
        int64_t save = (beforeOpt - afterOpt) * 100L / beforeOpt;
        printf("Results:\nOriginal size - %i bytes\nOptimized size - %i bytes\nSave - %" PRId64 "%%",
            beforeOpt, afterOpt, save);
	} else {
	    printf("No optimizations were made\n");
	}
	Wad_Close(file);

	return 0;
}

int main(int argc, char **argv)
{
    return WC_Main(argc, argv);
}
