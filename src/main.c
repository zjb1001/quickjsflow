#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quickjsflow/lexer.h"

static char *read_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return NULL; }
    rewind(f);
    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t readn = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[readn] = '\0';
    if (out_len) *out_len = readn;
    return buf;
}

static const char *tok_name(TokenType t) {
    switch (t) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_IDENTIFIER: return "Identifier";
        case TOKEN_NUMBER: return "Number";
        case TOKEN_STRING: return "String";
        case TOKEN_PUNCTUATOR: return "Punctuator";
        case TOKEN_COMMENT_LINE: return "LineComment";
        case TOKEN_COMMENT_BLOCK: return "BlockComment";
        case TOKEN_ERROR: return "Error";
        default: return "Unknown";
    }
}

static int cmd_lex(const char *path) {
    size_t len = 0;
    char *src = read_file(path, &len);
    if (!src) {
        fprintf(stderr, "Failed to read file: %s\n", path);
        return 2;
    }
    Lexer lx;
    lexer_init(&lx, src, len);
    for (;;) {
        Token t = lexer_next(&lx);
            printf("{\"type\":\"%s\",\"start\":{\"line\":%d,\"column\":%d},\"end\":{\"line\":%d,\"column\":%d},\"error\":%d,",
                   tok_name(t.type), t.start_line, t.start_col, t.end_line, t.end_col, t.error);
            if (t.error_kind) {
                printf("\"kind\":\"%s\",", t.error_kind);
            } else {
                printf("\"kind\":null,");
            }
            printf("\"lexeme\":\"");
        if (t.lexeme) {
            // naive escaping of quotes and backslashes
            for (char *p = t.lexeme; *p; ++p) {
                    if (*p == '"' || *p == '\\') putchar('\\');
                putchar(*p);
            }
        }
        printf("\"}\n");
        if (t.type == TOKEN_EOF) { token_free(&t); break; }
        token_free(&t);
    }
    free(src);
    return 0;
}

static void usage(void) {
    fprintf(stderr, "Usage: quickjsflow lex <file>\n");
}

int main(int argc, char **argv) {
    if (argc < 3) { usage(); return 1; }
    const char *cmd = argv[1];
    if (strcmp(cmd, "lex") == 0) {
        return cmd_lex(argv[2]);
    }
    usage();
    return 1;
}
