#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

#define MAXLINES 100
#define MAXLEN 256

char lines[MAXLINES][MAXLEN];
int linecount = 0;
char filename[512];

void print_help() {
    printf("Simple ed editor commands:\n");
    printf("a - append lines\n");
    printf("p - print all lines\n");
    printf("d - delete line\n");
    printf("w - write to file\n");
    printf("q - quit\n");
    printf("h - help\n");
}

// Custom string copy function
void copy_str(char *dest, const char *src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

// Custom string concatenate function
void concat_str(char *dest, const char *src) {
    while (*dest) dest++;
    while (*src) *dest++ = *src++;
    *dest = '\0';
}

void gets_with_backspace(char *buf, int max) {
    int i = 0;
    char c;
    
    while(1) {
        if(read(0, &c, 1) != 1) {
            buf[0] = '\0';
            return;
        }
        
        // Handle backspace (ASCII 8)
        if(c == '\b' || c == 127) {
            if(i > 0) {
                i--;
                write(1, "\b \b", 3); // Erase the character on screen
            }
            continue;
        }
        
        // Handle enter key
        if(c == '\n' || c == '\r') {
            buf[i] = '\n';
            buf[i+1] = '\0';
            write(1, "\n", 1);
            return;
        }
        
        // Normal character
        if(i < max-1) {
            buf[i++] = c;
            write(1, &c, 1); // Echo the character
        }
    }
}

void append_lines() {
    printf("Enter lines (single . on line to end):\n");
    while (1) {
        char buf[MAXLEN];
        gets_with_backspace(buf, MAXLEN);
        if (buf[0] == '.' && buf[1] == '\n') {
            break;
        }
        if (linecount < MAXLINES) {
            copy_str(lines[linecount], buf);
            linecount++;
        } else {
            printf("Buffer full\n");
            break;
        }
    }
}

void print_lines() {
    for (int i = 0; i < linecount; i++) {
        printf("%d: %s", i+1, lines[i]);
    }
}

void delete_line() {
    printf("Enter line number to delete: ");
    char buf[16];
    gets_with_backspace(buf, 16);
    int n = atoi(buf);
    if (n < 1 || n > linecount) {
        printf("Invalid line number\n");
        return;
    }
    for (int i = n-1; i < linecount-1; i++) {
        copy_str(lines[i], lines[i+1]);
    }
    linecount--;
}

void write_file() {
    if (filename[0] == '\0') {
        printf("Enter filename: ");
        gets_with_backspace(filename, sizeof(filename));
        // Remove newline
        filename[strlen(filename)-1] = '\0';
    }

    int fd = open(filename, O_CREATE | O_WRONLY);
    if (fd < 0) {
        printf("Error opening file\n");
        return;
    }

    for (int i = 0; i < linecount; i++) {
        if (write(fd, lines[i], strlen(lines[i])) != strlen(lines[i])) {
            printf("Write error\n");
            close(fd);
            return;
        }
    }
    close(fd);
    printf("%d lines written to %s\n", linecount, filename);
}

void read_file() {
    printf("Enter filename: ");
    gets_with_backspace(filename, sizeof(filename));
    // Remove newline
    filename[strlen(filename)-1] = '\0';

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        printf("Error opening file\n");
        return;
    }

    linecount = 0;
    char buf[MAXLEN];
    int n;
    while ((n = read(fd, buf, MAXLEN-1)) > 0 && linecount < MAXLINES) {
        buf[n] = '\0';
        // Simple split by newlines
        char *p = buf;
        char *q;
        while ((q = strchr(p, '\n')) != 0 && linecount < MAXLINES) {
            *q = '\0';
            copy_str(lines[linecount], p);
            lines[linecount][strlen(p)] = '\n';
            lines[linecount][strlen(p)+1] = '\0';
            linecount++;
            p = q + 1;
        }
    }
    close(fd);
    printf("%d lines read from %s\n", linecount, filename);
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        strcpy(filename, argv[1]);
        read_file();
    }

    printf("Simple ed editor (type 'h' for help)\n");
    
    while (1) {
        printf("> ");
        char cmd[16];
        gets_with_backspace(cmd, sizeof(cmd));
        
        switch (cmd[0]) {
            case 'a':
                append_lines();
                break;
            case 'p':
                print_lines();
                break;
            case 'd':
                delete_line();
                break;
            case 'w':
                write_file();
                break;
            case 'r':
                read_file();
                break;
            case 'q':
                exit(0);
                break;
            case 'h':
                print_help();
                break;
            default:
                printf("Unknown command '%c'. Type 'h' for help.\n", cmd[0]);
                break;
        }
    }

    exit(0);
}
