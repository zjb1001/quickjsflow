#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "quickjsflow/parser.h"

static Token next(Parser *p) {
    if (p->has_lookahead) {
        p->has_lookahead = 0;
        return p->lookahead;
    }
    return lexer_next(&p->lx);
}

static Token peek(Parser *p) {
    if (!p->has_lookahead) {
        p->lookahead = lexer_next(&p->lx);
        p->has_lookahead = 1;
    }
    return p->lookahead;
}

static int is_punct(Token *t, const char *s) {
    return t->type == TOKEN_PUNCTUATOR && t->lexeme && strcmp(t->lexeme, s) == 0;
}

static Position pos_start(Token *t) { Position p = { t->start_line, t->start_col }; return p; }
static Position pos_end(Token *t) { Position p = { t->end_line, t->end_col }; return p; }

void parser_init(Parser *p, const char *input, size_t length) {
    lexer_init(&p->lx, input, length);
    p->has_lookahead = 0;
}

static AstNode *parse_primary(Parser *p) {
    Token t = next(p);
    if (t.type == TOKEN_IDENTIFIER) {
        AstNode *id = ast_identifier(t.lexeme, pos_start(&t), pos_end(&t));
        token_free(&t);
        return id;
    }
    if (t.type == TOKEN_NUMBER) {
        AstNode *lit = ast_literal(LIT_Number, t.lexeme, pos_start(&t), pos_end(&t));
        token_free(&t);
        return lit;
    }
    if (t.type == TOKEN_STRING) {
        AstNode *lit = ast_literal(LIT_String, t.lexeme, pos_start(&t), pos_end(&t));
        token_free(&t);
        return lit;
    }
    if (t.type == TOKEN_ERROR) {
        AstNode *err = ast_error(t.error_kind ? t.error_kind : "LexerError", pos_start(&t), pos_end(&t));
        token_free(&t);
        return err;
    }
    AstNode *err = ast_error("UnexpectedToken", pos_start(&t), pos_end(&t));
    token_free(&t);
    return err;
}

static AstNode *parse_update_expr(Parser *p) {
    Token t = peek(p);
    if (t.type == TOKEN_IDENTIFIER) {
        // check for postfix ++ or --
        Token idt = next(p);
        Token pt = peek(p);
        if (pt.type == TOKEN_PUNCTUATOR && pt.lexeme && (strcmp(pt.lexeme, "++") == 0 || strcmp(pt.lexeme, "--") == 0)) {
            Token op = next(p);
            Position s = pos_start(&idt);
            Position e = pos_end(&op);
            AstNode *id = ast_identifier(idt.lexeme, pos_start(&idt), pos_end(&idt));
            AstNode *ue = ast_update_expression(op.lexeme, 0, id, s, e);
            token_free(&idt); token_free(&op);
            return ue;
        }
        // fallback: identifier as primary
        AstNode *id = ast_identifier(idt.lexeme, pos_start(&idt), pos_end(&idt));
        token_free(&idt);
        return id;
    }
    return parse_primary(p);
}

static AstNode *parse_expression(Parser *p) {
    // MVP: support identifier, number, string, postfix update; ignore binary for simplicity
    return parse_update_expr(p);
}

static AstNode *parse_variable_declaration(Parser *p, VarKind kind) {
    AstNode *decl = ast_variable_declaration(kind);
    VariableDeclaration *vd = (VariableDeclaration *)decl->data;
    // id
    Token t = peek(p);
    if (t.type != TOKEN_IDENTIFIER) {
        AstNode *err = ast_error("ExpectedIdentifier", pos_start(&t), pos_end(&t));
        astvec_push(&vd->declarations, err);
        return decl;
    }
    Token idt = next(p);
    AstNode *id = ast_identifier(idt.lexeme, pos_start(&idt), pos_end(&idt));
    token_free(&idt);
    AstNode *init = NULL;
    Token pt = peek(p);
    if (is_punct(&pt, "=")) {
        next(p);
        init = parse_expression(p);
    } else {
        if (pt.type != TOKEN_PUNCTUATOR || !pt.lexeme || strcmp(pt.lexeme, ";") != 0) {
            // allow missing init; continue
        }
    }
    AstNode *vdtr = ast_variable_declarator(id, init);
    astvec_push(&vd->declarations, vdtr);
    // consume optional ';'
    Token semi = peek(p);
    if (semi.type == TOKEN_PUNCTUATOR && semi.lexeme && strcmp(semi.lexeme, ";") == 0) { next(p); }
    return decl;
}

static AstNode *parse_statement(Parser *p) {
    Token t = peek(p);
    // skip comments as trivia
    if (t.type == TOKEN_COMMENT_LINE || t.type == TOKEN_COMMENT_BLOCK) { next(p); return parse_statement(p); }
    if (t.type == TOKEN_IDENTIFIER && t.lexeme && strcmp(t.lexeme, "var") == 0) { next(p); return parse_variable_declaration(p, VD_Var); }
    if (t.type == TOKEN_IDENTIFIER && t.lexeme && strcmp(t.lexeme, "let") == 0) { next(p); return parse_variable_declaration(p, VD_Let); }
    if (t.type == TOKEN_IDENTIFIER && t.lexeme && strcmp(t.lexeme, "const") == 0) { next(p); return parse_variable_declaration(p, VD_Const); }
    if (t.type == TOKEN_EOF) { return NULL; }
    // expression statement
    Position s = { t.start_line, t.start_col };
    AstNode *expr = parse_expression(p);
    Token endt = peek(p);
    Position e = { endt.start_line, endt.start_col };
    // consume optional ';'
    if (endt.type == TOKEN_PUNCTUATOR && endt.lexeme && strcmp(endt.lexeme, ";") == 0) { next(p); endt = peek(p); }
    AstNode *stmt = ast_expression_statement(expr, s, e);
    return stmt;
}

AstNode *parse_program(Parser *p) {
    AstNode *prog = ast_program();
    Program *pr = (Program *)prog->data;
    for (;;) {
        Token t = peek(p);
        // skip trivia tokens
        if (t.type == TOKEN_COMMENT_LINE || t.type == TOKEN_COMMENT_BLOCK) { next(p); continue; }
        if (t.type == TOKEN_EOF) { break; }
        AstNode *stmt = parse_statement(p);
        if (!stmt) break;
        astvec_push(&pr->body, stmt);
    }
    return prog;
}
