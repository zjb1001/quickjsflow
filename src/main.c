#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quickjsflow/lexer.h"
#include "quickjsflow/parser.h"
#include "quickjsflow/ast.h"
#include "quickjsflow/cfg.h"
#include "quickjsflow/codegen.h"
#include "quickjsflow/scope.h"
#include "quickjsflow/plugin.h"

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

static int cmd_generate(const char *path) {
    size_t len = 0;
    char *src = read_file(path, &len);
    if (!src) {
        fprintf(stderr, "Failed to read file: %s\n", path);
        return 2;
    }
    
    // Parse the input as JavaScript (for now, could be extended to parse JSON AST)
    Parser p;
    parser_init(&p, src, len);
    AstNode *prog = parse_program(&p);
    
    if (!prog) {
        fprintf(stderr, "Failed to parse program\n");
        free(src);
        return 1;
    }
    
    // Generate code
    CodegenOptions opts = {
        .indent_width = 2,
        .indent_char = ' ',
        .emit_source_map = 0,
        .source_name = path
    };
    
    CodegenResult result = codegen_generate(prog, &opts);
    
    if (result.code) {
        printf("%s", result.code);
        codegen_result_free(&result);
    } else {
        fprintf(stderr, "Code generation failed\n");
        ast_free(prog);
        free(src);
        return 1;
    }
    
    ast_free(prog);
    free(src);
    return 0;
}

static int cmd_check(const char *path) {
    size_t len = 0;
    char *src = read_file(path, &len);
    if (!src) {
        fprintf(stderr, "Error: Failed to read file: %s\n", path);
        return 2;
    }
    
    Parser p;
    parser_init(&p, src, len);
    AstNode *prog = parse_program(&p);
    
    if (!prog) {
        fprintf(stderr, "Error: Failed to parse program\n");
        free(src);
        return 1;
    }
    
    // Perform scope analysis
    ScopeManager sm;
    scope_manager_init(&sm);
    int scope_ok = scope_analyze(&sm, prog, 0);
    
    if (scope_ok != 0) {
        fprintf(stderr, "Error: Scope analysis failed\n");
        scope_manager_free(&sm);
        ast_free(prog);
        free(src);
        return 1;
    }
    
    printf("✓ %s: No errors found\n", path);
    
    scope_manager_free(&sm);
    ast_free(prog);
    free(src);
    return 0;
}

static int cmd_run(const char *path, const char *plugin_name) {
    size_t len = 0;
    char *src = read_file(path, &len);
    if (!src) {
        fprintf(stderr, "Failed to read file: %s\n", path);
        return 2;
    }
    
    Parser p;
    parser_init(&p, src, len);
    AstNode *prog = parse_program(&p);
    
    if (!prog) {
        fprintf(stderr, "Failed to parse program\n");
        free(src);
        return 1;
    }
    
    // Apply plugin if specified
    if (plugin_name) {
        Plugin *plugin = NULL;
        
        if (strcmp(plugin_name, "remove-console") == 0) {
            plugin = plugin_remove_console_log();
        } else if (strcmp(plugin_name, "remove-debugger") == 0) {
            plugin = plugin_remove_debugger();
        } else {
            fprintf(stderr, "Unknown plugin: %s\n", plugin_name);
            ast_free(prog);
            free(src);
            return 1;
        }
        
        fprintf(stderr, "Applying plugin: %s\n", plugin->name);
        prog = plugin_apply(plugin, prog, NULL);
    }
    
    // Generate code
    CodegenOptions opts = {
        .indent_width = 2,
        .indent_char = ' ',
        .emit_source_map = 0,
        .source_name = path
    };
    
    CodegenResult result = codegen_generate(prog, &opts);
    
    if (result.code) {
        printf("%s", result.code);
        codegen_result_free(&result);
    } else {
        fprintf(stderr, "Code generation failed\n");
        ast_free(prog);
        free(src);
        return 1;
    }
    
    ast_free(prog);
    free(src);
    return 0;
}

static void usage(void) {
    fprintf(stderr, "Usage: quickjsflow <command> [options] <file>\n\n");
    fprintf(stderr, "Commands:\n");
    fprintf(stderr, "  lex <file>              Tokenize file and output JSON tokens\n");
    fprintf(stderr, "  parse <file>            Parse file and output AST in JSON format\n");
    fprintf(stderr, "  generate <file>         Generate code from AST (expects JSON AST)\n");
    fprintf(stderr, "  check <file>            Parse and check for errors\n");
    fprintf(stderr, "  cfg <file> [format]     Build control flow graph\n");
    fprintf(stderr, "                          format: json (default), dot, mermaid\n");
    fprintf(stderr, "  run <file> [--plugin]   Parse, transform with plugin, and output code\n");
    fprintf(stderr, "                          --plugin remove-console    Remove console.log calls\n");
    fprintf(stderr, "                          --plugin remove-debugger   Remove debugger statements\n");
    fprintf(stderr, "\nExamples:\n");
    fprintf(stderr, "  quickjsflow parse input.js\n");
    fprintf(stderr, "  quickjsflow generate ast.json\n");
    fprintf(stderr, "  quickjsflow check input.js\n");
    fprintf(stderr, "  quickjsflow run input.js --plugin remove-console\n");
}

int main(int argc, char **argv) {
    if (argc < 2) { usage(); return 1; }
    const char *cmd = argv[1];
    
    if (strcmp(cmd, "lex") == 0) {
        if (argc < 3) { usage(); return 1; }
        return cmd_lex(argv[2]);
    }
    if (strcmp(cmd, "parse") == 0) {
        if (argc < 3) { usage(); return 1; }
        size_t len = 0; char *src = read_file(argv[2], &len);
        if (!src) { fprintf(stderr, "Failed to read file: %s\n", argv[2]); return 2; }
        Parser p; parser_init(&p, src, len);
        AstNode *prog = parse_program(&p);
        ast_print_json(prog);
        ast_free(prog);
        free(src);
        return 0;
    }
    if (strcmp(cmd, "generate") == 0) {
        if (argc < 3) { usage(); return 1; }
        return cmd_generate(argv[2]);
    }
    if (strcmp(cmd, "check") == 0) {
        if (argc < 3) { usage(); return 1; }
        return cmd_check(argv[2]);
    }
    if (strcmp(cmd, "run") == 0) {
        if (argc < 3) { usage(); return 1; }
        const char *plugin_name = NULL;
        if (argc >= 4 && strcmp(argv[3], "--plugin") == 0) {
            if (argc < 5) {
                fprintf(stderr, "Error: --plugin requires a plugin name\n");
                usage();
                return 1;
            }
            plugin_name = argv[4];
        }
        return cmd_run(argv[2], plugin_name);
    }
    if (strcmp(cmd, "cfg") == 0) {
        if (argc < 3) { usage(); return 1; }
        size_t len = 0;
        char *src = read_file(argv[2], &len);
        if (!src) {
            fprintf(stderr, "Failed to read file: %s\n", argv[2]);
            return 2;
        }
        
        Parser p;
        parser_init(&p, src, len);
        AstNode *prog = parse_program(&p);
        
        if (!prog || prog->type != AST_Program) {
            fprintf(stderr, "Failed to parse program\n");
            free(src);
            return 1;
        }
        
        Program *program = (Program *)prog->data;
        
        // 查找第一个函数声明
        AstNode *func = NULL;
        for (size_t i = 0; i < program->body.count; i++) {
            if (program->body.items[i]->type == AST_FunctionDeclaration) {
                func = program->body.items[i];
                break;
            }
        }
        
        if (!func) {
            fprintf(stderr, "No function declaration found\n");
            ast_free(prog);
            free(src);
            return 1;
        }
        
        // 构建 CFG
        CFG *cfg = qjs_build_cfg(func, NULL, NULL);
        if (!cfg) {
            fprintf(stderr, "Failed to build CFG\n");
            ast_free(prog);
            free(src);
            return 1;
        }
        
        // 确定输出格式
        const char *format = (argc > 3) ? argv[3] : "json";
        char *output = NULL;
        
        if (strcmp(format, "dot") == 0) {
            output = qjs_cfg_to_dot(cfg);
        } else if (strcmp(format, "mermaid") == 0) {
            output = qjs_cfg_to_mermaid(cfg);
        } else {
            // 默认为 json
            output = qjs_cfg_to_json(cfg);
        }
        
        if (output) {
            printf("%s", output);
            free(output);
        }
        
        qjs_cfg_free(cfg);
        ast_free(prog);
        free(src);
        return 0;
    }
    usage();
    return 1;
}
