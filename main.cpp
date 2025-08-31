#include <cctype>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>

enum tokens {
    RETURN,
    INT_LIT,
    SEMI,
    INVALID,
    EQUAL,
    STRING,
    LET,
    PRINT,
    L_PAREN,
    R_PAREN,
    QUOTES
};

class Token {
    public:
        tokens type;
        int value;
        std::string val;

        Token(const tokens& n) : type(n), value(0), val("") {}
        Token(const tokens& n, int a) : type(n), value(a), val("") {}
        Token(const tokens& n, std::string s) : type(n), value(0), val(s) {}
        Token(const tokens& n, int a, std::string s) : type(n), value(a), val(s) {}
};

std::vector<Token> getTokens(std::string str) {
    std::vector<Token> token_list;
    std::string buf;
    const size_t n = str.length();

    for (size_t i = 0; i < n; ++i) { // O(n)
        unsigned char c = static_cast<unsigned char>(str[i]);
        if (std::isalpha(c)) {
            buf.clear();
            size_t j = i;
            while (j < n && std::isalnum(static_cast<unsigned char>(str[j]))) {
                buf += str[j];
                ++j;
            }
            if(buf == "return") {
                token_list.push_back({ RETURN });
            } else if( buf == "let") {
                token_list.push_back({ LET });
            } else if(buf == "print") {
                token_list.push_back({ PRINT });
            } else {
                token_list.push_back({ STRING, buf });
            }
            i = j - 1;
            buf.clear();
        } else if (std::isdigit(c)) {
            buf.clear();
            size_t j = i;
            while (j < n && std::isdigit(static_cast<unsigned char>(str[j]))) {
                buf += str[j];
                ++j;
            }
            token_list.push_back({ INT_LIT, std::stoi(buf) });
            buf.clear();
            i = j - 1;
        } else if(str[i] == ';') {
            token_list.push_back({ SEMI });
        } else if(str[i] == '=') {
            token_list.push_back({ EQUAL });
        } else if(str[i] == '(') {
            token_list.push_back({ L_PAREN });
        } else if(str[i] == ')') {
            token_list.push_back({ R_PAREN });
        } else if(str[i] == '"') {
            token_list.push_back({ QUOTES });
        } else if(std::isspace(c)) {
            continue;
        } else {
            token_list.push_back({ INVALID });
            std::cerr << "FOUND AN INVALID, at i = " << i << std::endl;
            break;
        }
    }

    return token_list;
}

std::string sysVars;
int sysVarC = 0;

std::unordered_map<std::string, size_t> varLengths;

std::string addSysVar(std::string s) {
    std::string name = "sysvar" + std::to_string(sysVarC);
    sysVars += name + " db \"" + s + "\", 0\n";
    sysVarC++;
    return name;
}

std::string code(std::vector<Token> t) {
    std::string code;
    for(int i = 0; i < (int)t.size(); i++) {
        if(t[i].type == RETURN){
            //let hello = 70;
            //reutrn hello;
            if(i + 2 < (int)t.size()) {
                if(t[i + 1].type == INT_LIT && t[i + 2].type == SEMI) {
                    //std::cout << i << " " << t.size() << std::endl;
                    code += "    mov rax, 60\n    mov rdi, ";
                    code += std::to_string(t[i + 1].value);
                    code += "\n    syscall";
                    i+=2;
                } else if (t[i + 1].type == STRING && t[i + 2].type == SEMI) {
                    //std::cout << i << " " << t.size() << std::endl;
                    code += "    mov rax, 60\n    mov edi, dword [";
                    code += t[i + 1].val;
                    code += "]\n    syscall";
                    i+=2;
                }
            } else {
                std::cerr << "return used withut an error return value" << std::endl;
                break;
            }
        }
        if (t[i].type == PRINT) {
            std::cout << i << " " << t.size() << std::endl;
            // pattern: print("literal");
            if (i + 6 < (int)t.size()) {
                if(t[i+1].type == L_PAREN && t[i+2].type == QUOTES && t[i+3].type == STRING &&
                        t[i+4].type == QUOTES && t[i+5].type == R_PAREN && t[i+6].type == SEMI) {
                    std::string name = addSysVar(t[i+3].val);
                    // use lea [rel ...] and correct length (no +1)
                    code += ("    mov rax, 1\n    mov rdi, 1\n    lea rsi, [rel " + name + "]\n    mov rdx, " + std::to_string(t[i+3].val.length()) + "\n    syscall\n\n");
                    i += 6; // skip PRINT ( " literal " ) ;
                }
            }
            // pattern: print(var);
            if(i + 4 < (int)t.size()) {
                if(t[i+1].type == L_PAREN && t[i+2].type == STRING && t[i+3].type == R_PAREN && t[i+4].type == SEMI) {
                    std::string varName = t[i+2].val;
                    size_t len = 0;
                    auto it = varLengths.find(varName);
                    if (it != varLengths.end()) len = it->second;
                    code += ("    mov rax, 1\n    mov rdi, 1\n    lea rsi, [rel " + varName + "]\n    mov rdx, " + std::to_string(len) + "\n    syscall\n\n");
                    i+=4;
                }
            }
        }
    }
    return code;
}


std::string variables(std::vector<Token> t) {
    std::string code;
    for(int i = 0; i < (int)t.size(); i++) {
        if(t[i].type == LET) {
            //let myvar = 50;
            if(i + 4 < (int)t.size()) {
                if(t[i+1].type == STRING && t[i+2].type == EQUAL && t[i+3].type == INT_LIT && t[i+4].type == SEMI) {
                    code += (t[i+1].val + " dd " + std::to_string(t[i+3].value) + "\n"); // <-- add newline
                    i += 4;
                    continue;
                }
            }
            //let myvar = "hello";
            if(i + 6 < (int)t.size()) {
                if(t[i+1].type == STRING && t[i+2].type == EQUAL && t[i+3].type == QUOTES && t[i+4].type == STRING
                        && t[i+5].type == QUOTES && t[i+6].type == SEMI) {
                    code += (t[i+1].val + " db \"" + t[i+4].val + "\", 0\n"); // <-- add newline
                    // store length of the string (without terminating 0) for use when printing the variable
                    varLengths[t[i+1].val] = t[i+4].val.length(); // <-- remember length
                    i+=6;
                    continue;
                }
            }
        }
    }
    return code;
}

std::string usingTokens(std::vector<Token> t) {
    std::string asmCode;
    asmCode = "section .data\n";
    asmCode += variables(t);
    asmCode += "\n\nsection .text\n";
    asmCode += "global _start\n\n";
    asmCode += "_start:\n";
    asmCode += code(t);

    return asmCode;
}

int main(int argc, char* argv[]) {
    std::cout << argv[1] << std::endl;

    std::ifstream file(argv[1]);
    std::string code((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));;

    std::vector<Token> found = getTokens(code);
    for(int i = 0; i < (int)found.size(); i++) {
          std::cout << "type: " << found[i].type << " value: " << found[i].value << " val: " << found[i].val << " i: " << i << std::endl;
    }

    {
        std::ofstream asmFile("./asm/main1.asm");
        asmFile << usingTokens(found);
    }

    {
        std::cout << std::endl << std::endl << sysVars;
        std::ifstream asmFile("./asm/main1.asm");
        std::ostringstream oss;

        std::string line;
        bool foundLine = false;

        while (std::getline(asmFile, line)) {
            oss << line << std::endl;
            if (line.find("section .data") != std::string::npos) {
                oss << sysVars << std::endl;
                foundLine = true;
            }
        }
        asmFile.close();

        std::ofstream asmFile1("./asm/main1.asm");

        asmFile1 << oss.str();
    }

    return 0;
}

