#ifndef TOKEN_H
#define TOKEN_H

struct Token {
    Token() : flag(0), messages(0) {}
    Token(int flag, int messages) : flag(flag), messages(messages) {}

    int flag = 0;
    int messages = 0;
};

#endif // TOKEN_H
